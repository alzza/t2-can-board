// 애플리케이션 진입점 및 초기화/루프 템플릿
#pragma once

#include <memory>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "handlers.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif

#ifndef PIN_LED
#define PIN_LED 2  // 상태 LED 핀 (프레임 수신 시 점멸)
#endif




using SelectedHandler = HW3Handler;
/*
#if defined(NAG_KILLER)
using SelectedHandler = NagHandler;
#elif defined(HW4)
using SelectedHandler = HW4Handler;
#elif defined(HW3)
using SelectedHandler = HW3Handler;
#elif defined(LEGACY)
using SelectedHandler = LegacyHandler;
#else
#error "Define HW4, HW3, LEGACY, or NAG_KILLER in build_flags"
#endif
*/


                                                                                                                                                                                                                            
static std::unique_ptr<CanDriver> appDriver;    // A채널 CAN 드라이버 (MCP2515)
static std::unique_ptr<CarManagerBase> appHandler; // A채널 프레임 핸들러 (HW3Handler)

static volatile bool frameReady = true;  // ISR용 프레임 수신 플래그 (T2CAN은 폴링이라 미사용)
static void canISR() { frameReady = true; }

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
#include "web/web_server.h"
#endif

template <typename Driver>
static void appSetup(std::unique_ptr<Driver> drv, const char *readyMsg)
{
    appHandler = std::make_unique<SelectedHandler>();
    delay(1500);
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 1000)
    {
    }

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    appDriver = std::move(drv);
    if (!appDriver->init())
    {
        Serial.println("CAN init failed");
    }

    appDriver->setFilters(appHandler->filterIds(), appHandler->filterIdCount());
    // ISR 지원 드라이버만 인터럽트 활성화 (T2CAN의 MCP2515는 폴링 전용)
    if constexpr (Driver::kSupportsISR)
    {
        appDriver->enableInterrupt(canISR);
    }

    Serial.println(readyMsg);

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
    // CAN 버스 안정화 대기. 웹 서버는 setup() 마지막에서 시작해
    // 반쯤 부팅된 상태가 외부에 노출되지 않도록 한다.
    delay(2000);
#endif
}

template <typename Driver>
static void appLoop()
{
    aChannelDiag.lastLoopMs = millis();
#if !defined(NATIVE_BUILD)
    aChannelDiag.loopCoreId = xPortGetCoreID();
#endif

    // ── A채널 Hz 계산 (1초 간격) ─────────────────────────────────────────────
    static uint32_t _aHzFrames = 0;
    static uint32_t _aLastHzMs = 0;
    if (_aLastHzMs == 0) _aLastHzMs = millis();

    CanFrame frame;
    // ── A채널 (MCP2515/SPI) 수신 루프 ────────────────────────────────────────
    // appDriver(MCP2515) 에서 프레임을 읽어 핸들러로 처리합니다.
    //  · handleMessage : HW3/HW4/NagHandler 가 프레임을 분석·수정·에코
    // ─────────────────────────────────────────────────────────────────────────
    while (appDriver->read(frame))
    {
        digitalWrite(PIN_LED, LOW);
        appHandler->frameCount++;
        _aHzFrames++;
#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
        aChannelDiag.lastFrameRxMs = millis();
#endif
        appHandler->handleMessage(frame, *appDriver);
    }
    digitalWrite(PIN_LED, HIGH);

    {
        uint32_t _now = millis();
        uint32_t _elapsed = _now - _aLastHzMs;
        if (_elapsed >= 1000) {
            aChannelDiag.frameHz = _aHzFrames * 1000.0f / (float)(_elapsed > 0 ? _elapsed : 1);
            _aHzFrames = 0;
            _aLastHzMs = _now;
        }
    }
    aChannelDiag.lastStatusUpdateMs = millis();

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
    // ── A채널 MCP2515 에러 플래그 폴링 (1초 간격) ─────────────────────────────
    // getErrorFlags()는 Normal Mode를 유지하며 EFLG 레지스터만 읽음 (통신 무중단)
    static uint32_t _lastEflgMs = 0;
    static bool _prevTxBo = false;
    static uint32_t _prevATxFail = 0;
    static uint32_t _aTxBoSinceMs = 0;
    static uint32_t _aLastTxBoRecoverMs = 0;
    {
        uint32_t _nowMs = millis();
        if (_nowMs - _lastEflgMs >= 1000) {
            _lastEflgMs = _nowMs;
            uint8_t eflg = appDriver->getErrorFlags();
            aChannelDiag.mcpEflg = eflg;
            // 누적 OR: 세션 내 최악 상태 보존
            aChannelDiag.mcpEflgPeak = (uint8_t)aChannelDiag.mcpEflgPeak | eflg;

            // ── TEC/REC 실시간 값 + 피크 추적 ────────────────────────────
            uint8_t tec = 0, rec = 0;
            appDriver->getErrorCounters(tec, rec);
            aChannelDiag.aTec = tec;
            aChannelDiag.aRec = rec;
            if (tec > (uint8_t)aChannelDiag.aTecPeak) aChannelDiag.aTecPeak = tec;
            if (rec > (uint8_t)aChannelDiag.aRecPeak) aChannelDiag.aRecPeak = rec;

            // ── MERRF (ACK/Bit/Stuff 메시지 에러) 발생 카운트 + 클리어 ────
            // 발생률 ↑ → 가설 H1(ACK 부재) 또는 H2(동일 ID 충돌) 후보
            if (appDriver->readAndClearMerrf()) {
                aChannelDiag.aMerrfCount = (uint32_t)aChannelDiag.aMerrfCount + 1;
            }

            // ── RX0OVR/RX1OVR sticky 비트 클리어 + 재발 카운트 ────────────
            // sticky이므로 한 번 set되면 SW가 명시 클리어해야 다음 OVR 검출 가능.
            // 클리어 후 다시 카운트가 증가하면 → 진짜 폴링 부족(가설: A루프 지연).
            if (eflg & 0xC0) {
                aChannelDiag.aRxOvrCount = (uint32_t)aChannelDiag.aRxOvrCount + 1;
                appDriver->clearRxOverrun();
            }

            // BUS-OFF 진입 감지 (TXBO 비트, bit5)
            // EFLG 0→비제로 전환: 에러 발생 이벤트 카운트
            static uint8_t _prevEflg = 0;
            if (eflg != 0 && _prevEflg == 0) {
                aChannelDiag.mcpEflgEventCount = (uint32_t)aChannelDiag.mcpEflgEventCount + 1;
            }
            _prevEflg = eflg;

            bool txBo = (eflg & (1 << 5)) != 0;
            if (txBo && _aTxBoSinceMs == 0) {
                _aTxBoSinceMs = _nowMs;
                aChannelDiag.mcpBusOffSinceMs = _aTxBoSinceMs;
            }
            if (txBo && !_prevTxBo) {
                aChannelDiag.mcpTxBoCount = (uint32_t)aChannelDiag.mcpTxBoCount + 1;
                logRing.push("🚨 [A-CH] MCP2515 BUS-OFF 진입 감지 (EFLG.TXBO)", _nowMs);
            }
            if (txBo) {
                aChannelDiag.mcpBusOffSinceMs = _aTxBoSinceMs;
                if (_aLastTxBoRecoverMs == 0 || _nowMs - _aLastTxBoRecoverMs >= kAMcpBusOffRecoverIntervalMs) {
                    _aLastTxBoRecoverMs = _nowMs;
                    aChannelDiag.mcpRecoveryAttemptCount = (uint32_t)aChannelDiag.mcpRecoveryAttemptCount + 1;
                    aChannelDiag.mcpLastRecoveryMs = _nowMs;
                    if (appDriver->recoverBusOff()) {
                        appDriver->setFilters(appHandler->filterIds(), appHandler->filterIdCount());
                        aChannelDiag.mcpRecoverySuccessCount = (uint32_t)aChannelDiag.mcpRecoverySuccessCount + 1;
                        logRing.push("🛠️ [A-CH] MCP2515 BUS-OFF 재초기화 완료", _nowMs);
                    } else {
                        aChannelDiag.mcpRecoveryFailCount = (uint32_t)aChannelDiag.mcpRecoveryFailCount + 1;
                        logRing.push("❌ [A-CH] MCP2515 BUS-OFF 재초기화 실패", _nowMs);
                    }
                }
                if (_nowMs - _aTxBoSinceMs >= kAMcpBusOffRestartFallbackMs) {
                    logRing.push("🧯 [A-CH] MCP2515 BUS-OFF 지속 → ESP32 재시작", _nowMs);
                    delay(100);
                    ESP.restart();
                }
            } else if (_aTxBoSinceMs != 0) {
                _aTxBoSinceMs = 0;
                _aLastTxBoRecoverMs = 0;
                aChannelDiag.mcpBusOffSinceMs = 0;
                logRing.push("✅ [A-CH] MCP2515 BUS-OFF 해제", _nowMs);
            }
            _prevTxBo = txBo;
            // TX 에러 경고 로그 (에러 패시브 이상일 때) — 실제 TEC 값 함께 기록
            if (eflg & (1 << 4)) { // TXEP: TEC ≥ 128
                char buf[80];
                snprintf(buf, sizeof(buf), "⚠️ [A-CH] EFLG=0x%02X TEC=%u/REC=%u TX에러패시브", eflg, tec, rec);
                logRing.push(buf, _nowMs);
            } else if (eflg & (1 << 2)) { // TXWAR: TEC ≥ 96
                char buf[80];
                snprintf(buf, sizeof(buf), "⚠️ [A-CH] EFLG=0x%02X TEC=%u/REC=%u TEC≥96 경고", eflg, tec, rec);
                logRing.push(buf, _nowMs);
            }

            uint8_t guardReason = kATxGuardReasonNone;
            if ((bool)aTxGuardRuntime) {
                if (eflg & ((1 << 5) | (1 << 4) | (1 << 2))) {
                    guardReason = kATxGuardReasonEflg;
                } else if (tec >= kATxGuardTecThreshold) {
                    guardReason = kATxGuardReasonTec;
                } else if ((uint32_t)aChannelDiag.aTxFail != _prevATxFail) {
                    guardReason = kATxGuardReasonTxFail;
                }
            }
            _prevATxFail = (uint32_t)aChannelDiag.aTxFail;

            if (guardReason != kATxGuardReasonNone) {
                bool wasGuardActive = aTxGuardActive(_nowMs);
                aChannelDiag.aTxGuardUntilMs = _nowMs + kATxGuardDurationMs;
                aChannelDiag.aTxGuardLastReason = guardReason;
                if (!wasGuardActive) {
                    aChannelDiag.aTxGuardCount = (uint32_t)aChannelDiag.aTxGuardCount + 1;
                    char buf[96];
                    snprintf(buf, sizeof(buf), "🛡️ [A-CH] TX guard %ums 시작: %s TEC=%u EFLG=0x%02X Fail=%u",
                             (unsigned)kATxGuardDurationMs,
                             aTxGuardReasonName(guardReason),
                             tec,
                             eflg,
                             (unsigned)(uint32_t)aChannelDiag.aTxFail);
                    logRing.push(buf, _nowMs);
                }
            }
        }
    }
#endif
}  
