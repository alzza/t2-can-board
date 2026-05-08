// A채널(HW3Handler) 및 B채널(NagHandler) CAN 프레임 핸들러 정의
#pragma once

#include <memory>
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "event_log.h"
#include "shared_types.h"
#include "log_buffer.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#else
#include <chrono>
inline unsigned long millis() {
    return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void delayMicroseconds(unsigned int) {}
inline unsigned long micros() {
    return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
#endif

inline LogRingBuffer logRing;

// 모든 CAN 핸들러의 기본 클래스
struct CarManagerBase
{
    Shared<int> speedProfile{1};        // (미사용) 속도 프로필
    Shared<bool> FSDEnabled{false};     // (미사용) FSD 활성화 상태
    Shared<bool> enablePrint{true};     // 시리얼 출력 활성화 여부
    Shared<uint32_t> frameCount{0};     // 수신 프레임 카운터
    Shared<uint32_t> framesSent{0};     // 전송 프레임 카운터
    Shared<int> speedOffset{0};         // (미사용) 속도 오프셋

    virtual void handleMessage(CanFrame &frame, CanDriver &driver) = 0;  // 프레임 처리 핵심 메소드
    virtual const uint32_t *filterIds() const = 0;   // 수신 필터 ID 배열
    virtual uint8_t filterIdCount() const = 0;        // 수신 필터 ID 개수
    virtual ~CarManagerBase() = default;
};

// ===================================================================
// [A채널] HW3Handler (EAP)
// ===================================================================
struct HW3Handler : public CarManagerBase
{
    bool shouldSkipATx(const char *featureName)
    {
        uint32_t nowMs = millis();
        if (!aChannelTxRuntime) {
            static unsigned long lastAOffLog = 0;
            if (nowMs - lastAOffLog > 5000) {
                char buf[80];
                snprintf(buf, sizeof(buf), "[A-CH] %s 주입 보류: A TX OFF", featureName);
                logRing.push(buf, nowMs);
                lastAOffLog = nowMs;
            }
            return true;
        }
        if (!aTxGuardActive(nowMs)) return false;

        aChannelDiag.aTxGuardSkipCount = (uint32_t)aChannelDiag.aTxGuardSkipCount + 1;
        static unsigned long lastGuardSkipLog = 0;
        if (nowMs - lastGuardSkipLog > 5000) {
            char buf[96];
            snprintf(buf, sizeof(buf), "🛡️ [A-CH] %s 주입 보류: TX guard active (%s)",
                     featureName, aTxGuardReasonName((uint8_t)aChannelDiag.aTxGuardLastReason));
            logRing.push(buf, nowMs);
            lastGuardSkipLog = nowMs;
        }
        return true;
    }

    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {1021};
        return ids;
    }
    uint8_t filterIdCount() const override { return 1; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        aChannelDiag.framesReceivedTotal++;
        aChannelDiag.lastFrameIdReceived = frame.id;

        aChannelDiag.frames1021++;

        if (readMuxID(frame) == 0) {
#if defined(ENHANCED_AUTOPILOT)
            if (tsllcRuntime) {
                if (shouldSkipATx("TSLLC")) return;
                setBit(frame, 38, true);  // UI_fsdStopsControlEnabled: 스톱사인/신호등 자동 정지 제어 활성화 (TSLLC 검증)
                setBit(frame, 39, true);  // UI_fsdContinueOnGreenWithCIPV: 앞차 있을 때 녹색신호 자동 출발 활성화 (TSLLC 검증)
                aChannelDiag.tsllcModifiedCount++;
                framesSent++;
                // sendCheck: ERROR_OK=true → aTxOk++, ALLTXBUSY/FAILTX=false → aTxFail++
                if (driver.sendCheck(frame)) { aChannelDiag.aTxOk++; aChannelDiag.lastTxMs = millis(); }
                else                           aChannelDiag.aTxFail++;

                static unsigned long lastTsllcAction = 0;
                if (millis() - lastTsllcAction > 5000) {
                    logRing.push("🟢⚡ [A-CH] TSLLC 주입 완료: 정지/출발 제어 활성", millis());
                    lastTsllcAction = millis();
                }
            }
#endif
        }

        if (readMuxID(frame) == 1) {
#if defined(ENHANCED_AUTOPILOT)
            if (enhancedAutopilotRuntime) {
                if (shouldSkipATx("EAP")) return;
                setBit(frame, 19, false);  // UI_applyEceR79=0 (ECE R79 적용 해제, HW3/HW4 공통)
#if defined(HW3)
                setBit(frame, 46, true);   // UI_hardCoreSummon HW3용 스마트 서먼 해제 비트
#elif defined(HW4)
                setBit(frame, 47, true);   // UI_hardCoreSummon HW4 전용 비트
#endif
                aChannelDiag.eapModifiedCount++;
                framesSent++;
                if (driver.sendCheck(frame)) { aChannelDiag.aTxOk++; aChannelDiag.lastTxMs = millis(); }
                else                           aChannelDiag.aTxFail++;

                static unsigned long lastAAction = 0;
                if (millis() - lastAAction > 5000) {
                    logRing.push("🔵⚡ [A-CH] 작동 OK: EAP 규제 완화 주입 완료", millis());
                    lastAAction = millis();
                }
            }
#endif
        }
    }
};
 

// ===================================================================
// [B채널] NagHandler (유기적 스텔스 나그 킬러)
// ===================================================================

struct NagHandler : public CarManagerBase
{
    Shared<bool> nagKillerActive{true};
    Shared<uint32_t> nagEchoCount{0};
    Shared<uint8_t> dasHandsOnState{0xFF};

    uint32_t _prngState = 0xDEADBEEF;
    int16_t _torqWalk = 2230;
    uint8_t _excFrames = 0;
    uint16_t _framesUntilExc = 175;

    // ── Mode B 상태머신 변수 ────────────────────────────────────────────────
    uint8_t  _mbApState    = 0;       // DAS_autopilotState (ID 921/923 data[0]&0x0F)
    float    _mbAngleDeg   = 0.0f;    // SCCM_steeringAngle (ID 297)
    uint8_t  _mbPrevHoSt   = 0xFF;    // 직전 HandsOnState (전이 감지용)

    uint32_t _mbState1EnterMs  = 0;   // 상태1 진입 시각 (500ms grace)
    uint32_t _mbState1HoldTorq = 2048;
    uint8_t  _mbState1HoldHo   = 0;

    uint32_t _mbState2EnterMs  = 0;   // 상태2 진입 시각 (2s 딜레이)
    int16_t  _mbMildWalkRaw    = 2098;// 상태2 random-walk 현재값
    uint32_t _mbS2HoldUntilMs  = 0;
    uint32_t _mbS2HoldTorqRaw  = 2048;
    uint8_t  _mbS2HoldHo       = 0;
    bool     _mbS2L2WasActive  = false;

    uint32_t _mbStrongEnterMs  = 0;   // 상태3-5 진입 시각 (1s 딜레이)
    uint32_t _mbStrongActiveMs = 0;   // 강 토크 패턴 시작 시각 (ramp)

    uint32_t _mbLastGeneratedTorqRaw = 2048;
    uint8_t  _mbLastSpoofedHo        = 0;

    const uint32_t *filterIds() const override {
        // Mode B는 297(SCCM_steeringAngle)도 필요. 923은 921 대체가 아니라 DAS_status 후보다.
        // Mode A에서 921/923/297은 handleMessage 내 조기 반환으로 처리된다.
        static constexpr uint32_t ids[] = {880, 921, 923, 297};
        return ids;
    }

    uint8_t filterIdCount() const override { return 4; }

    // ── 내부 헬퍼 ───────────────────────────────────────────────────────────
    static bool _mbIsStrongState(uint8_t s) { return s == 3 || s == 4 || s == 5; }

    static uint16_t _mbClampU16(uint32_t v) {
        return v > 65535UL ? 65535U : static_cast<uint16_t>(v);
    }

    static uint32_t _mbPackStateDetail(uint8_t apState, uint8_t oldHoState, uint8_t newHoState) {
        return (static_cast<uint32_t>(apState) << 16)
             | (static_cast<uint32_t>(oldHoState) << 8)
             | static_cast<uint32_t>(newHoState);
    }

    static uint32_t _mbPackPhaseDetail(uint8_t phase, uint8_t apState, uint8_t hoState, uint8_t decision) {
        return (static_cast<uint32_t>(phase) << 24)
             | (static_cast<uint32_t>(apState) << 16)
             | (static_cast<uint32_t>(hoState) << 8)
             | static_cast<uint32_t>(decision);
    }

    void _mbPushTimingEvent(uint8_t type, uint32_t detail) {
        eventLogPush(type,
                     _mbClampU16((uint32_t)bChannelDiag.twaiTxErrNow),
                     _mbClampU16((uint32_t)bChannelDiag.twaiRxErrNow),
                     detail);
    }

    void _mbSetPhase(uint8_t phase, uint32_t nowMs, uint8_t decision) {
        uint8_t prevPhase = (uint8_t)bChannelDiag.modeBPhase;
        bChannelDiag.modeBPhase = phase;
        if ((uint32_t)bChannelDiag.modeBPhaseEnterMs == 0 || prevPhase != phase) {
            bChannelDiag.modeBPhaseEnterMs = nowMs;
            if (prevPhase != phase) {
                _mbPushTimingEvent(EV_MODEB_PHASE, _mbPackPhaseDetail(phase, _mbApState, (uint8_t)dasHandsOnState, decision));
            }
        }
        if (phase == 0 && prevPhase != 0) {
            bChannelDiag.modeBFirstEchoDelayMs = 0;
        }
    }

    // Mode B: 수신된 EPAS 880 프레임을 변조해 버스에 주입
    // torqRaw: 12비트 raw (center=2048), hoLevel: 0-3
    void _mbSendEcho(const CanFrame &frame, CanDriver &driver, uint16_t torqRaw, uint8_t hoLevel) {
        uint32_t txMs = millis();
        CanFrame echo = frame;
        echo.data[2] = (frame.data[2] & 0xF0) | static_cast<uint8_t>((torqRaw >> 8) & 0x0F);
        echo.data[3] = static_cast<uint8_t>(torqRaw & 0xFF);
        echo.data[4] = (frame.data[4] & 0x3F) | static_cast<uint8_t>((hoLevel & 0x03) << 6);
        echo.data[5] = frame.data[5];
        uint8_t cnt = ((frame.data[6] & 0x0F) + 1) & 0x0F;
        echo.data[6] = (frame.data[6] & 0xF0) | cnt;
        uint16_t sum = echo.data[0] + echo.data[1] + echo.data[2] + echo.data[3]
                     + echo.data[4] + echo.data[5] + echo.data[6];
        echo.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);
        driver.send(echo);
        framesSent++;
        nagEchoCount++;
        bChannelDiag.echoCount++;
        bChannelDiag.lastEchoTxMs = txMs;
        bChannelDiag.modeBInjectCount = (uint32_t)bChannelDiag.modeBInjectCount + 1;
        // Nm = (raw - 2048) * 0.01
        bChannelDiag.modeBLastTorqueNm = (static_cast<int32_t>(torqRaw) - 2048) * 0.01f;
        uint32_t enterMs = (uint32_t)bChannelDiag.modeBStateEnterMs;
        if (enterMs != 0 && (uint32_t)bChannelDiag.modeBFirstEchoDelayMs == 0) {
            uint32_t delayMs = txMs - enterMs;
            if (delayMs == 0) delayMs = 1;
            bChannelDiag.modeBFirstEchoDelayMs = delayMs;
            _mbPushTimingEvent(EV_MODEB_FIRST_ECHO, delayMs);
        }
    }

    // Mode B 메인 로직: 880 프레임 처리
    void _handleModeB(const CanFrame &frame, CanDriver &driver) {
        uint32_t nowMs = millis();

        // 전역 허용 조건: AP state 3-6 + HandsOnState 활성
        if (_mbApState < 3 || _mbApState > 6) {
            bChannelDiag.skipApState++;
            _mbSetPhase(0, nowMs, kNagDecisionApBlocked);
            bChannelDiag.nagLastDecision = kNagDecisionApBlocked;
            return;
        }
        uint8_t hoSt = (uint8_t)dasHandsOnState;
        if (hoSt == 0 || hoSt == 8 || hoSt == 15 || hoSt == 0xFF) {
            // DAS 미수신(0xFF)은 fallback: 스킵(Mode B는 보수적)
            uint8_t decision = (hoSt == 0xFF) ? kNagDecisionNo921 : kNagDecisionDasIdle;
            _mbSetPhase(0, nowMs, decision);
            bChannelDiag.nagLastDecision = decision;
            return;
        }
        // 실제 핸즈온 감지 시 주입 중단
        uint8_t realHo = (frame.data[4] >> 6) & 0x03;
        if (realHo != 0) {
            bChannelDiag.skipHandsOn++;
            _mbSetPhase(0, nowMs, kNagDecisionHandsOn);
            bChannelDiag.nagLastDecision = kNagDecisionHandsOn;
            return;
        }

        // ── 상태 전이 처리 ────────────────────────────────────────────────
        if (_mbPrevHoSt != hoSt) {
            uint8_t oldHoSt = _mbPrevHoSt;
            bChannelDiag.modeBStateEnterMs = nowMs;
            bChannelDiag.modeBFirstEchoDelayMs = 0;
            _mbPushTimingEvent(EV_MODEB_STATE, _mbPackStateDetail(_mbApState, oldHoSt, hoSt));

            // 상태1 진입
            if (hoSt == 1) {
                _mbState1EnterMs  = nowMs;
                _mbState1HoldTorq = _mbLastGeneratedTorqRaw;
                _mbState1HoldHo   = _mbLastSpoofedHo;
            }
            if (hoSt != 1) { _mbState1EnterMs = 0; _mbState1HoldTorq = 2048; _mbState1HoldHo = 0; }

            // 상태2 진입
            if (hoSt == 2) { _mbState2EnterMs = nowMs; }
            if (hoSt != 2) {
                _mbState2EnterMs = 0; _mbS2HoldUntilMs = 0;
                _mbS2HoldTorqRaw = 2048; _mbS2HoldHo = 0; _mbS2L2WasActive = false;
            }

            // 상태3-5 진입 (그룹 내 이동은 타이머 유지)
            if (!_mbIsStrongState(_mbPrevHoSt) && _mbIsStrongState(hoSt)) {
                _mbStrongEnterMs = nowMs;
                _mbStrongActiveMs = 0;
            }
            if (!_mbIsStrongState(hoSt)) { _mbStrongEnterMs = 0; _mbStrongActiveMs = 0; }
            _mbPrevHoSt = hoSt;
        }

        uint16_t torqRaw = 2048;
        uint8_t  hoLevel = 0;

        // ── 상태1: idle (500ms grace만) ───────────────────────────────────
        if (hoSt == 1) {
            if (_mbState1EnterMs != 0 && (nowMs - _mbState1EnterMs) < kNagModeBState1GraceMs) {
                torqRaw = static_cast<uint16_t>(_mbState1HoldTorq);
                hoLevel = _mbState1HoldHo;
                _mbSetPhase(1, nowMs, kNagDecisionEcho);
            } else {
                // grace 종료 — 주입 없음
                _mbLastGeneratedTorqRaw = 2048;
                _mbLastSpoofedHo = 0;
                _mbSetPhase(0, nowMs, kNagDecisionNoEcho);
                bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
                return;
            }
        }
        // ── 상태2: mild random-walk ──────────────────────────────────────
        else if (hoSt == 2) {
            // 2초 딜레이
            if (_mbState2EnterMs != 0 && (nowMs - _mbState2EnterMs) < kNagModeBState2DelayMs) {
                _mbSetPhase(2, nowMs, kNagDecisionNoEcho);
                bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
                return;
            }
            _mbSetPhase(3, nowMs, kNagDecisionEcho);
            // level-2 hold
            if (nowMs < _mbS2HoldUntilMs) {
                torqRaw = static_cast<uint16_t>(_mbS2HoldTorqRaw);
                hoLevel = _mbS2HoldHo;
            } else {
                // random-walk in direction opposite steering
                int16_t minR = 2098, maxR = 2198; // +0.5 ~ +1.5 Nm
                if (_mbAngleDeg > 0.0f) { minR = 1898; maxR = 1998; } // -1.5 ~ -0.5 Nm
                if (_mbMildWalkRaw < minR || _mbMildWalkRaw > maxR)
                    _mbMildWalkRaw = static_cast<int16_t>((minR + maxR) / 2);
                // xorshift step (재사용)
                uint32_t r = _prngState; r ^= r<<13; r ^= r>>17; r ^= r<<5; _prngState = r;
                _mbMildWalkRaw += static_cast<int16_t>(r % 25) - 12;
                if (_mbMildWalkRaw < minR) _mbMildWalkRaw = minR;
                if (_mbMildWalkRaw > maxR) _mbMildWalkRaw = maxR;
                torqRaw = static_cast<uint16_t>(_mbMildWalkRaw);
                int32_t absR = abs(static_cast<int32_t>(torqRaw) - 2048);
                hoLevel = (absR >= 200) ? 2 : (absR >= 100) ? 1 : 0;
                // level-2 최초 진입 → 1초 hold
                if (hoLevel == 2 && !_mbS2L2WasActive) {
                    _mbS2HoldUntilMs = nowMs + 1000UL;
                    _mbS2HoldTorqRaw = torqRaw;
                    _mbS2HoldHo = 2;
                    _mbS2L2WasActive = true;
                }
            }
        }
        // ── 상태3-5: 강 ramp-and-hold ────────────────────────────────────
        else if (_mbIsStrongState(hoSt)) {
            // 1초 초기 대기
            if (_mbStrongEnterMs != 0 && (nowMs - _mbStrongEnterMs) < kNagModeBStrongDelayMs) {
                _mbSetPhase(4, nowMs, kNagDecisionNoEcho);
                bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
                return;
            }
            if (_mbStrongActiveMs == 0) _mbStrongActiveMs = nowMs;
            uint32_t activeMs = nowMs - _mbStrongActiveMs;
            uint32_t phase = activeMs % 1500UL;
            // 0-500ms: ramp 0→2.1Nm, 500-1500ms: hold 2.1Nm
            uint16_t magnitude; // raw delta from center
            uint8_t strongPhase = 5;
            if (phase < kNagModeBStrongRampMs) {
                magnitude = static_cast<uint16_t>(210UL * phase / kNagModeBStrongRampMs);  // 0→210 (=2.10Nm)
            } else {
                magnitude = 210;
                strongPhase = 6;
            }
            _mbSetPhase(strongPhase, nowMs, kNagDecisionEcho);
            torqRaw = (_mbAngleDeg > 0.0f)
                      ? static_cast<uint16_t>(2048 - magnitude)
                      : static_cast<uint16_t>(2048 + magnitude);
            // 범위 클램프 (abs 최대 2.1Nm = raw ±210)
            if (torqRaw < 1838) torqRaw = 1838;
            if (torqRaw > 2258) torqRaw = 2258;
            int32_t absR2 = abs(static_cast<int32_t>(torqRaw) - 2048);
            hoLevel = (absR2 >= 200) ? 2 : (absR2 >= 100) ? 1 : 0;
        } else {
            _mbSetPhase(0, nowMs, kNagDecisionNoEcho);
            bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
            return;
        }

        _mbLastGeneratedTorqRaw = torqRaw;
        _mbLastSpoofedHo = hoLevel;
        _mbSendEcho(frame, driver, torqRaw, hoLevel);
        bChannelDiag.nagLastDecision = kNagDecisionEcho;
        bChannelDiag.realHo = realHo;

        static uint32_t lastMBLog = 0;
        if (millis() - lastMBLog > 3000) {
            char buf[96];
            snprintf(buf, sizeof(buf), "🎯 [B-CH] Mode B 주입 hoSt=%u torq=%u ho=%u ap=%u",
                     hoSt, torqRaw, hoLevel, _mbApState);
            logRing.push(buf, millis());
            lastMBLog = millis();
        }
    }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        // ── ID 297 SCCM_steeringAngleSensor: 조향각 갱신 (Mode B용) ─────────
        if (frame.id == 297) {
            if (frame.dlc >= 4) {
                // SCCM_steeringAngle: 16|14@1+ (0.1,-819.2) Little-Endian
                uint16_t raw = static_cast<uint16_t>(
                    (frame.data[2] | (static_cast<uint16_t>(frame.data[3]) << 8)) & 0x3FFF);
                _mbAngleDeg = raw * 0.1f - 819.2f;
                bChannelDiag.steeringAngleDeg = _mbAngleDeg;
                bChannelDiag.frames297 = (uint32_t)bChannelDiag.frames297 + 1;
                bChannelDiag.last297RxMs = millis();
            }
            return;
        }

        // ── ID 921/923 DAS_status 후보: AP state + HandsOn state 갱신 ─────
        if (frame.id == 921 || frame.id == 923) {
            if (frame.dlc >= 6) {
                uint8_t apSt = frame.data[0] & 0x0F;
                uint8_t hoSt = (frame.data[5] >> 2) & 0x0F;
                _mbApState = apSt;
                bChannelDiag.dasAutopilotStateRx = apSt;
                dasHandsOnState = hoSt;
                bChannelDiag.dasHandsOnStateRx = hoSt;
                bChannelDiag.dasStatusSourceId = frame.id;
                bChannelDiag.lastDasStatusRxMs = millis();
                if (frame.id == 921) bChannelDiag.last921RxMs = millis();
                else                 bChannelDiag.last923RxMs = millis();
            }
            return;
        }

        // ── ID 880 EPAS3P_sysStatus: 조건부 echo ────────────────────────────
        if (frame.id != 880 || frame.dlc < 8) return;

        uint8_t handsOn = (frame.data[4] >> 6) & 0x03;
        uint16_t tRaw = static_cast<uint16_t>(((frame.data[2] & 0x0F) << 8) | frame.data[3]);
        bChannelDiag.last880RxMs = millis();
        bChannelDiag.realHo = handsOn;
        bChannelDiag.realTorqueNm = tRaw * 0.01f - 20.5f;

        if (!nagKillerActive || !nagKillerRuntime) {
            bChannelDiag.skipRuntimeOrInactive++;
            bChannelDiag.nagLastDecision = kNagDecisionRuntimeOff;
            return;
        }

        // 현재 모드 스냅샷 (portMUX 아래서 읽기)
        uint8_t currentMode;
#ifndef NATIVE_BUILD
        portENTER_CRITICAL(&nagCfgMux);
        currentMode = nagConfig.mode;
        portEXIT_CRITICAL(&nagCfgMux);
#else
        currentMode = nagConfig.mode;
#endif
        bChannelDiag.nagMode = currentMode;

        if (currentMode == kNagModeB) {
            _handleModeB(frame, driver);
            return;
        }

        // ── Mode A: 기존 스텔스 PRNG ────────────────────────────────────────
        if (handsOn != 0) {
            bChannelDiag.skipHandsOn++;
            bChannelDiag.nagLastDecision = kNagDecisionHandsOn;
            return;
        }

        uint8_t dasState = dasHandsOnState;
        if (dasState == 0xFF) {
            bChannelDiag.nagLastDecision = kNagDecisionNo921;
            return;
        }
        if (!nagDasStateRequiresEcho(dasState)) {
            bChannelDiag.skipDasState++;
            bChannelDiag.nagLastDecision = kNagDecisionDasIdle;
            return;
        }

        uint32_t r = _prngState;
        r ^= r << 13; r ^= r >> 17; r ^= r << 5;
        _prngState = r;

        if (_excFrames > 0) {
            _torqWalk = static_cast<int16_t>(2350 + static_cast<int16_t>(r % 41) - 20);
            _excFrames--;
        } else {
            _torqWalk += static_cast<int16_t>(r % 31) - 15;
            if (_torqWalk < 2150) _torqWalk = 2150;
            if (_torqWalk > 2290) _torqWalk = 2290;

            if (_framesUntilExc == 0) {
                _excFrames = 3 + static_cast<uint8_t>(r % 3);
                _framesUntilExc = 125 + static_cast<uint16_t>(r % 101);
            } else {
                _framesUntilExc--;
            }
        }
        uint16_t torqRaw = static_cast<uint16_t>(_torqWalk);

        // --- 에코 프레임 생성 및 데이터 변조 ---
        CanFrame echo = frame;
        echo.data[2] = (frame.data[2] & 0xF0) | static_cast<uint8_t>(torqRaw >> 8);
        echo.data[3] = static_cast<uint8_t>(torqRaw & 0xFF);
        echo.data[4] = frame.data[4] | 0x40;
        echo.data[5] = frame.data[5];
        uint8_t cnt = (frame.data[6] & 0x0F);
        cnt = (cnt + 1) & 0x0F;
        echo.data[6] = (frame.data[6] & 0xF0) | cnt;
        uint16_t sum = echo.data[0] + echo.data[1] + echo.data[2] + echo.data[3] +
                       echo.data[4] + echo.data[5] + echo.data[6];
        echo.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);
        driver.send(echo);
        framesSent++;
        nagEchoCount++;
        bChannelDiag.echoCount++;
        bChannelDiag.lastEchoTxMs = millis();
        bChannelDiag.nagLastDecision = kNagDecisionEcho;

        static unsigned long lastBAction = 0;
        if (millis() - lastBAction > 3000) {
            char buf[80];
            snprintf(buf, sizeof(buf), "🔥 [B-CH] 나그 방어 발사 (토크=%d)", torqRaw);
            logRing.push(buf, millis());
            lastBAction = millis();
        }
    }
};