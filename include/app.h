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
    const bool driverInitOk = appDriver->init();
    aChannelDiag.driverInitialized = driverInitOk;
    if (!driverInitOk)
    {
        Serial.println("CAN init failed");
    }
    else
    {
        appDriver->setFilters(appHandler->filterIds(), appHandler->filterIdCount());
    }
    // ISR 지원 드라이버만 인터럽트 활성화 (T2CAN의 MCP2515는 폴링 전용)
    if constexpr (Driver::kSupportsISR)
    {
        appDriver->enableInterrupt(canISR);
    }

    logRing.push(readyMsg, millis());

    // MCP2515 초기화 직후 수신 폴링을 시작한다. 여기서 2초를 멈추면
    // RX0/RX1 두 버퍼가 즉시 가득 차 OTA 재부팅 직후 EFLG RXOVR가 발생한다.
}

template <typename Driver>
static uint16_t appLoop()
{
    const uint32_t appLoopNowUs = micros();
    static uint32_t previousAppLoopUs = 0;
    const uint8_t phaseBeforeLoop = (uint8_t)aChannelDiag.canTaskPhase;
    if (previousAppLoopUs != 0) {
        const uint32_t gapUs = appLoopNowUs - previousAppLoopUs;
        aChannelDiag.loopGapLastUs = gapUs;
        if (gapUs > (uint32_t)aChannelDiag.loopGapPeakUs)
            aChannelDiag.loopGapPeakUs = gapUs;
        if (gapUs > (uint32_t)aChannelDiag.loopGapWindowPeakUs) {
            aChannelDiag.loopGapWindowPeakUs = gapUs;
            aChannelDiag.loopGapWindowPeakPhase = phaseBeforeLoop;
        }
        if (gapUs > 250U)
            aChannelDiag.loopGapOver250usCount =
                (uint32_t)aChannelDiag.loopGapOver250usCount + 1U;
        if (gapUs > 500U)
            aChannelDiag.loopGapOver500usCount =
                (uint32_t)aChannelDiag.loopGapOver500usCount + 1U;
        if (gapUs > 1000U)
            aChannelDiag.loopGapOver1msCount =
                (uint32_t)aChannelDiag.loopGapOver1msCount + 1U;
        if (gapUs > 2000U)
            aChannelDiag.loopGapOver2msCount =
                (uint32_t)aChannelDiag.loopGapOver2msCount + 1U;
    }
    previousAppLoopUs = appLoopNowUs;
    const uint32_t appLoopNowMs = millis();
    aChannelDiag.lastLoopMs = appLoopNowMs;
    summonGateMaintain(appLoopNowMs);
#if !defined(NATIVE_BUILD)
    aChannelDiag.loopCoreId = xPortGetCoreID();
#endif

    // ── A채널 Hz 계산 (1초 간격) ─────────────────────────────────────────────
    static uint32_t _aHzFrames = 0;
    static uint32_t _aLastHzMs = 0;
    if (_aLastHzMs == 0) _aLastHzMs = millis();

    // ── A채널 RX 선회수 큐 ───────────────────────────────────────────────────
    // MCP2515 RXB0/RXB1을 먼저 고정 RAM 큐로 비운다. 프레임 분석·1021 수정
    // 송신 뒤에도 즉시 다시 비워, 핸들러 실행 중 도착한 동기 버스트를 받는다.
    constexpr uint8_t kRxQueueCapacity = 32;
    constexpr uint8_t kRxDrainBurst = 4;
    CanFrame rxQueue[kRxQueueCapacity];
    uint8_t rxHead = 0;
    uint8_t rxCount = 0;
    uint16_t processedFrames = 0;

    auto drainHardwareRx = [&]() {
        aChannelDiag.canTaskPhase = kACanPhaseRxDrain;
        while (rxCount < kRxQueueCapacity) {
            CanFrame burst[kRxDrainBurst];
            const uint8_t room = (uint8_t)(kRxQueueCapacity - rxCount);
            const uint8_t request = room < kRxDrainBurst ? room : kRxDrainBurst;
            const uint8_t got = appDriver->drainReceived(burst, request);
            if (got == 0) break;
            // 빈 폴링 횟수가 아니라 실제 RX를 회수한 배치 수를 누적한다.
            // 32비트 카운터가 며칠 만에 순환하는 일을 피하면서 진단 의미도 선명해진다.
            aChannelDiag.aRxDrainCalls = (uint32_t)aChannelDiag.aRxDrainCalls + 1U;
            aChannelDiag.aRxDrainFrames = (uint32_t)aChannelDiag.aRxDrainFrames + got;
            for (uint8_t i = 0; i < got; ++i) {
                const uint8_t tail = (uint8_t)((rxHead + rxCount) % kRxQueueCapacity);
                rxQueue[tail] = burst[i];
                ++rxCount;
            }
            if (rxCount > (uint32_t)aChannelDiag.aRxQueueHighWater)
                aChannelDiag.aRxQueueHighWater = rxCount;
            if (got < request) break;
        }
    };

    drainHardwareRx();
    if (rxCount > 0) digitalWrite(PIN_LED, LOW);
    while (rxCount > 0) {
        CanFrame frame = rxQueue[rxHead];
        rxHead = (uint8_t)((rxHead + 1U) % kRxQueueCapacity);
        --rxCount;
        appHandler->frameCount++;
        _aHzFrames++;
        ++processedFrames;
#if !defined(NATIVE_BUILD)
        // A채널은 T-2CAN 빌드에서 MCP2515를 사용한다. DRIVER_TWAI로 제한하면
        // 실제 수신 중에도 lastFrameRxMs가 0으로 남아 NO_FRAMES/65.535초 포화값과
        // 약 49일 전 수신처럼 잘못 표시된다. 드라이버 종류와 무관하게 실제로
        // 회수한 A 프레임에서 수신 시각과 wake 구간을 갱신한다.
        const uint32_t frameNowMs = millis();
        const uint32_t previousFrameMs = (uint32_t)aChannelDiag.lastFrameRxMs;
        if (previousFrameMs > 0 && frameNowMs - previousFrameMs > kAChannelWakeGapMs) {
            aChannelDiag.wakeCount = (uint32_t)aChannelDiag.wakeCount + 1;
            aChannelDiag.lastWakeRxMs = frameNowMs;
            aChannelDiag.wakeAwaitingSummonTx = true;
            aChannelDiag.wakeToSummonTxMs = 0;
            logRing.push("[A-CH] CAN 재수신 시작: 첫 Summon TX 지연 측정", frameNowMs);
        }
        aChannelDiag.lastFrameRxMs = frameNowMs;
#endif
        aChannelDiag.canTaskPhase = kACanPhaseFrameHandle;
        appHandler->handleMessage(frame, *appDriver);
        drainHardwareRx();
    }
    digitalWrite(PIN_LED, HIGH);

    // RX 버퍼가 빈 뒤에만 비동기 TX 결과 레지스터를 확인한다.
    aChannelDiag.canTaskPhase = kACanPhaseTxResult;
    appDriver->pollTransmitResults();

    {
        uint32_t _now = millis();
        uint32_t _elapsed = _now - _aLastHzMs;
        if (_elapsed >= 1000) {
            aChannelDiag.frameHz = _aHzFrames * 1000.0f / (float)(_elapsed > 0 ? _elapsed : 1);
            _aHzFrames = 0;
            _aLastHzMs = _now;
        }
    }
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
            aChannelDiag.canTaskPhase = kACanPhaseErrorPoll;
            uint8_t eflg = appDriver->getErrorFlags();
            const uint32_t loopGapWindowPeakUs =
                (uint32_t)aChannelDiag.loopGapWindowPeakUs;
            const uint8_t loopGapWindowPeakPhase =
                (uint8_t)aChannelDiag.loopGapWindowPeakPhase;
            aChannelDiag.loopGapWindowPeakUs = 0;
            aChannelDiag.loopGapWindowPeakPhase = kACanPhaseIdle;
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
                if (eflg & 0x40U)
                    aChannelDiag.aRx0OvrCount = (uint32_t)aChannelDiag.aRx0OvrCount + 1U;
                if (eflg & 0x80U)
                    aChannelDiag.aRx1OvrCount = (uint32_t)aChannelDiag.aRx1OvrCount + 1U;
                aChannelDiag.lastOverrunPhase = loopGapWindowPeakPhase;
                appDriver->clearRxOverrun();
            }

            // BUS-OFF 진입 감지 (TXBO 비트, bit5)
            // EFLG 0→비제로 전환: 에러 발생 이벤트 카운트
            static uint8_t _prevEflg = 0;
            static bool _prevGuardActive = false;
            const bool guardActiveBeforeUpdate = aTxGuardActive(_nowMs);
            if (_prevGuardActive && !guardActiveBeforeUpdate) {
                const uint32_t guardDetail =
                    (uint32_t)(uint8_t)aChannelDiag.aTxGuardLastReason |
                    ((uint32_t)(uint8_t)aChannelDiag.aTxGuardTriggerSourceMask << 8);
                eventLogPush(EV_A_TX_GUARD_CLEAR, tec, rec, guardDetail);
            }
            if (eflg != 0 && _prevEflg == 0) {
                aChannelDiag.mcpEflgEventCount = (uint32_t)aChannelDiag.mcpEflgEventCount + 1;
                eventLogPush(EV_A_EFLG_SET, tec, rec, eflg);
                char buf[96];
                snprintf(buf, sizeof(buf), "⚠️ [A-CH] EFLG 진입: 0x%02X %s TEC=%u REC=%u",
                         eflg, aMcpEflgStateName(eflg), tec, rec);
                logRing.push(buf, _nowMs);
            } else if (eflg == 0 && _prevEflg != 0) {
                eventLogPush(EV_A_EFLG_CLEAR, tec, rec, _prevEflg);
                char buf[88];
                snprintf(buf, sizeof(buf), "✅ [A-CH] EFLG 해제: 이전=0x%02X TEC=%u REC=%u",
                         _prevEflg, tec, rec);
                logRing.push(buf, _nowMs);
            }
            if (eflg & 0xC0U) {
                const uint32_t rawGapUs = loopGapWindowPeakUs;
                const uint32_t gapUs = rawGapUs > 0x00FFFFFFUL ? 0x00FFFFFFUL : rawGapUs;
                eventLogPush(EV_A_RX_OVERRUN, tec, rec, eflg | (gapUs << 8));
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

            const uint32_t currentATxFail = (uint32_t)aChannelDiag.aTxFail;
            const uint32_t currentATxFailSummon = (uint32_t)aChannelDiag.aTxFailSummon;
            const uint32_t currentATxFailTsllc = (uint32_t)aChannelDiag.aTxFailTsllc;
            const uint32_t currentATxFailOther = (uint32_t)aChannelDiag.aTxFailOther;
            const uint32_t rawATxFailDelta = currentATxFail - _prevATxFail;
            const uint8_t aTxFailDelta =
                (uint8_t)(rawATxFailDelta > 255U ? 255U : rawATxFailDelta);
            aChannelDiag.aTxFailWindowDelta = aTxFailDelta;
            if (aTxFailDelta > (uint8_t)aChannelDiag.aTxFailWindowPeak) {
                aChannelDiag.aTxFailWindowPeak = aTxFailDelta;
            }
            static uint32_t _prevATxFailSummon = 0;
            static uint32_t _prevATxFailTsllc = 0;
            static uint32_t _prevATxFailOther = 0;
            const uint32_t summonFailDelta = currentATxFailSummon - _prevATxFailSummon;
            const uint32_t tsllcFailDelta = currentATxFailTsllc - _prevATxFailTsllc;
            const uint32_t otherFailDelta = currentATxFailOther - _prevATxFailOther;
            uint8_t txFailSourceMask = kATxSourceMaskNone;
            if (summonFailDelta > 0) txFailSourceMask |= kATxSourceMaskSummon;
            if (tsllcFailDelta > 0) txFailSourceMask |= kATxSourceMaskTsllc;
            if (otherFailDelta > 0) txFailSourceMask |= kATxSourceMaskOther;
            _prevATxFailSummon = currentATxFailSummon;
            _prevATxFailTsllc = currentATxFailTsllc;
            _prevATxFailOther = currentATxFailOther;

            uint8_t guardReason = kATxGuardReasonNone;
            if ((bool)aTxGuardRuntime) {
                if (eflg & ((1 << 5) | (1 << 4) | (1 << 2))) {
                    guardReason = kATxGuardReasonEflg;
                } else if (tec >= kATxGuardTecThreshold) {
                    guardReason = kATxGuardReasonTec;
                } else if (aTxFailDelta >= kATxGuardTxFailBurstThreshold) {
                    guardReason = kATxGuardReasonTxFail;
                }
            }
            _prevATxFail = currentATxFail;

            if (guardReason != kATxGuardReasonNone) {
                bool wasGuardActive = aTxGuardActive(_nowMs);
                aChannelDiag.aTxGuardUntilMs = _nowMs + kATxGuardDurationMs;
                aChannelDiag.aTxGuardLastReason = guardReason;
                aChannelDiag.aTxGuardTriggerSourceMask =
                    guardReason == kATxGuardReasonTxFail ? txFailSourceMask : kATxSourceMaskNone;
                if (!wasGuardActive) {
                    aChannelDiag.aTxGuardCount = (uint32_t)aChannelDiag.aTxGuardCount + 1;
                    const uint32_t guardDetail = (uint32_t)guardReason |
                        ((uint32_t)(uint8_t)aChannelDiag.aTxGuardTriggerSourceMask << 8);
                    eventLogPush(EV_A_TX_GUARD_SET, tec, rec, guardDetail);
                    char buf[120];
                    snprintf(buf, sizeof(buf), "🛡️ [A-CH] TX guard %ums 시작: %s(%s) TEC=%u EFLG=0x%02X Fail=%u Fail1s=%u",
                             (unsigned)kATxGuardDurationMs,
                             aTxGuardReasonName(guardReason),
                             aTxSourceMaskName((uint8_t)aChannelDiag.aTxGuardTriggerSourceMask),
                             tec,
                             eflg,
                             (unsigned)currentATxFail,
                             (unsigned)aTxFailDelta);
                    logRing.push(buf, _nowMs);
                }
            }
            _prevGuardActive = aTxGuardActive(_nowMs);
        }
    }
#endif
    aChannelDiag.canTaskPhase = kACanPhaseIdle;
    return processedFrames;
}  
