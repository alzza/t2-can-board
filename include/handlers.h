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
    Shared<bool> enablePrint{true};     // 시리얼 출력 활성화 여부
    Shared<uint32_t> frameCount{0};     // 수신 프레임 카운터
    Shared<uint32_t> framesSent{0};     // 전송 프레임 카운터

    virtual void handleMessage(CanFrame &frame, CanDriver &driver) = 0;  // 프레임 처리 핵심 메소드
    virtual const uint32_t *filterIds() const = 0;   // 수신 필터 ID 배열
    virtual uint8_t filterIdCount() const = 0;        // 수신 필터 ID 개수
    virtual ~CarManagerBase() = default;
};

// ===================================================================
// [A채널] HW3Handler (Summon Unlock/TSLLC)
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
        static uint32_t ids[kSignalObserverMaxAFilterIds] = {};
        signalObserverFillAFilterIds(ids, kSignalObserverMaxAFilterIds);
        return ids;
    }
    uint8_t filterIdCount() const override
    {
        static uint32_t ids[kSignalObserverMaxAFilterIds] = {};
        return signalObserverFillAFilterIds(ids, kSignalObserverMaxAFilterIds);
    }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        const uint32_t nowMs = millis();
        aChannelDiag.framesReceivedTotal++;
        aChannelDiag.lastFrameIdReceived = frame.id;
        signalObserverObserveFrame(kSignalObserverChannelA, frame, nowMs);

        if (frame.id == 280) {
            aChannelDiag.frames280++;
            summonHandle280(frame, nowMs);
            return;
        }

        if (frame.id == 390) {
            aChannelDiag.frames390++;
            summonHandle390(frame, nowMs);
            return;
        }

        if (frame.id == 921) {
            aChannelDiag.frames921++;
            summonHandle921(frame);
            return;
        }

        if (frame.id == 1016) {
            aChannelDiag.frames1016++;
            summonHandle1016(frame);
            return;
        }

        if (frame.id != 1021) return;
        aChannelDiag.frames1021++;
        if (frame.dlc < 8) return;

        if (readMuxID(frame) == 0) {
#if defined(SUMMON_UNLOCK)
            if (tsllcRuntime) {
                if (shouldSkipATx("TSLLC")) return;
                if (!canTxPermitBegin()) return;
                setBit(frame, 38, true);  // UI_fsdStopsControlEnabled: 스톱사인/신호등 자동 정지 제어 활성화 (TSLLC 검증)
                setBit(frame, 39, true);  // UI_fsdContinueOnGreenWithCIPV: 앞차 있을 때 녹색신호 자동 출발 활성화 (TSLLC 검증)
                aChannelDiag.tsllcModifiedCount++;
                framesSent++;
                // sendCheck: ERROR_OK=true → aTxOk++, ALLTXBUSY/FAILTX=false → aTxFail++
                if (driver.sendCheck(frame)) { aChannelDiag.aTxOk++; aChannelDiag.lastTxMs = millis(); }
                else                           aChannelDiag.aTxFail++;
                canTxPermitEnd();

                static unsigned long lastTsllcAction = 0;
                if (millis() - lastTsllcAction > 5000) {
                    logRing.push("🟢⚡ [A-CH] TSLLC 주입 완료: 정지/출발 제어 활성", millis());
                    lastTsllcAction = millis();
                }
            }
#endif
        }

        if (readMuxID(frame) == 1) {
#if defined(SUMMON_UNLOCK)
            const bool summonEnabled = (bool)summonUnlockRuntime;
            const bool gateOpen = summonGateOpen();
            const bool summonInject = summonEnabled && gateOpen;

            if (summonEnabled && !gateOpen) {
                summonGateDiag.blocked = (uint32_t)summonGateDiag.blocked + 1;
            }

            if (summonInject) {
                summonGateDiag.mux1Received = (uint32_t)summonGateDiag.mux1Received + 1;
                if (shouldSkipATx("SummonUnlock")) return;
                if (!canTxPermitBegin()) return;
                setBit(frame, 19, false);  // UI_applyEceR79=0 (ECE R79 적용 해제, HW3/HW4 공통)
#if defined(HW3)
                setBit(frame, 46, true); // 검증된 HW3 Summon Unlock 비트
#elif defined(HW4)
                setBit(frame, 47, true); // HW4 빌드 호환 경로
#endif
                aChannelDiag.summonUnlockModifiedCount++;
                framesSent++;
                const bool sent = driver.sendCheck(frame);
                if (sent) {
                    const uint32_t sentAtMs = millis();
                    aChannelDiag.aTxOk++;
                    aChannelDiag.lastTxMs = sentAtMs;
                    summonGateDiag.txOk = (uint32_t)summonGateDiag.txOk + 1;
                    if ((bool)aChannelDiag.wakeAwaitingSummonTx) {
                        const uint32_t wakeMs = (uint32_t)aChannelDiag.lastWakeRxMs;
                        const uint32_t wakeDelayMs = wakeMs > 0 ? sentAtMs - wakeMs : 0;
                        aChannelDiag.wakeToSummonTxMs = wakeDelayMs;
                        aChannelDiag.wakeAwaitingSummonTx = false;
                        eventLogPush(EV_A_WAKE_FIRST_TX,
                                     (uint16_t)(uint8_t)aChannelDiag.aTec,
                                     (uint16_t)(uint8_t)aChannelDiag.aRec,
                                     wakeDelayMs);
                    }
                } else {
                    aChannelDiag.aTxFail++;
                    summonGateDiag.txFail = (uint32_t)summonGateDiag.txFail + 1;
                }
                canTxPermitEnd();

                static unsigned long lastAAction = 0;
                if (millis() - lastAAction > 5000) {
                    logRing.push("🔵⚡ [A-CH] Summon Unlock 주입 완료: HW3 bit46", millis());
                    lastAAction = millis();
                }
            }
#endif
        }
    }
};
 

// ===================================================================
// [B채널] HW3 NagHandler — 검증된 MODE 1/2/3 이식
// ===================================================================
struct NagHandler : public CarManagerBase
{
    Shared<bool> nagKillerActive{true};
    Shared<uint32_t> nagEchoCount{0};
    Shared<uint8_t> dasHandsOnState{0xFF};

    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {880, 921, 923, 297};
        return ids;
    }

    uint8_t filterIdCount() const override { return 4; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        handleMessageAt(frame, driver, millis());
    }

    void handleMessageAt(CanFrame &frame, CanDriver &driver, uint32_t now,
                         bool transmissionAllowed = true)
    {
        uint8_t selectedMode;
#ifndef NATIVE_BUILD
        portENTER_CRITICAL(&nagCfgMux);
        selectedMode = nagConfig.mode;
        portEXIT_CRITICAL(&nagCfgMux);
#else
        selectedMode = nagConfig.mode;
#endif
        selectedMode = nagModeClamp(selectedMode);
        if (selectedMode != activeMode_)
            resetModeState(selectedMode, now);
        bChannelDiag.nagMode = selectedMode;

        if (frame.id == 921 || frame.id == 923)
        {
            updateDasState(frame, now);
            return;
        }
        if (frame.id == 297)
        {
            updateSteering(frame, now);
            return;
        }
        if (frame.id != 880 || frame.dlc < 8)
            return;

        bChannelDiag.last880RxMs = now;
        uint8_t handsOn = static_cast<uint8_t>((frame.data[4] >> 6) & 0x03);
        uint16_t torqueRaw = static_cast<uint16_t>(((frame.data[2] & 0x0F) << 8) |
                                                    frame.data[3]);
        bChannelDiag.realHo = handsOn;
        bChannelDiag.realTorqueNm = torqueRaw * 0.01f - 20.5f;

        // 12비트 토크 전체와 전체 프레임을 비교해 자기 에코 재처리를 막는다.
        bool isOwnEcho = (handsOn == 1 && torqueRaw == kNagTorqueRawMax) ||
                         matchesLastEcho(frame);
        if (isOwnEcho)
            return;

        if (!transmissionAllowed || !nagKillerActive || !nagKillerRuntime)
        {
            bChannelDiag.skipRuntimeOrInactive++;
            bChannelDiag.nagLastDecision = kNagDecisionRuntimeOff;
            return;
        }

        bool handsOnEligible = handsOn == 0 ||
                               (selectedMode == kNagMode3 && handsOn == 1);
        if (!handsOnEligible)
        {
            bChannelDiag.skipHandsOn++;
            bChannelDiag.nagLastDecision = kNagDecisionHandsOn;
            return;
        }

        uint16_t torque = kNagTorqueRawMax;
        bool setHandsOn = true;
        if (!decideInjection(selectedMode, now, torque, setHandsOn))
            return;

        sendEcho(frame, driver, now, torque, setHandsOn);
    }

private:
    static constexpr uint32_t kContextFreshMs = 1000;
    static constexpr uint32_t kMode2BurstMs = 1000;
    static constexpr uint32_t kMode2PauseMs = 1500;
    static constexpr uint32_t kMode2TorqueStepMs = 200;

    uint8_t activeMode_ = 0xFF;
    uint8_t mode2TorqueIndex_ = 0;
    uint32_t modeEnteredMs_ = 0;
    uint32_t mode2LastStepMs_ = 0;
    uint8_t apState_ = 0;
    uint8_t handsOnState_ = 0;
    float steeringAngleDeg_ = 0.0f;
    uint32_t lastDasStateMs_ = 0;
    uint32_t lastSteeringMs_ = 0;
    uint32_t handsOnStateEnteredMs_ = 0;
    bool dasStateSeen_ = false;
    bool steeringSeen_ = false;
    bool handsOnStateSeen_ = false;
    uint16_t walkSeed_ = 0;
    float lastMode3TorqueNm_ = 0.0f;
    CanFrame lastEcho_{};
    bool lastEchoValid_ = false;

    static uint16_t clampTorqueRaw(uint16_t raw)
    {
        if (raw < kNagTorqueRawMin) return kNagTorqueRawMin;
        if (raw > kNagTorqueRawMax) return kNagTorqueRawMax;
        return raw;
    }

    static uint16_t torqueNmToRaw(float torqueNm)
    {
        if (torqueNm < -1.8f) torqueNm = -1.8f;
        if (torqueNm > 1.8f) torqueNm = 1.8f;
        float scaled = (torqueNm + 20.5f) * 100.0f + 0.5f;
        return clampTorqueRaw(static_cast<uint16_t>(scaled));
    }

    static float absolute(float value)
    {
        return value < 0.0f ? -value : value;
    }

    static uint16_t clampU16(uint32_t value)
    {
        return value > 65535UL ? 65535U : static_cast<uint16_t>(value);
    }

    void resetModeState(uint8_t mode, uint32_t now)
    {
        activeMode_ = mode;
        mode2TorqueIndex_ = 0;
        modeEnteredMs_ = now;
        mode2LastStepMs_ = now;
        apState_ = 0;
        handsOnState_ = 0;
        steeringAngleDeg_ = 0.0f;
        lastDasStateMs_ = 0;
        lastSteeringMs_ = 0;
        handsOnStateEnteredMs_ = 0;
        dasStateSeen_ = false;
        steeringSeen_ = false;
        handsOnStateSeen_ = false;
        walkSeed_ = 0;
        lastMode3TorqueNm_ = 0.0f;
        lastEchoValid_ = false;
        bChannelDiag.modeBPhase = 0;
        bChannelDiag.modeBPhaseEnterMs = now;
        bChannelDiag.modeBStateEnterMs = 0;
        bChannelDiag.modeBFirstEchoDelayMs = 0;
        eventLogPush(EV_NAG_MODE,
                     clampU16((uint32_t)bChannelDiag.twaiTxErrNow),
                     clampU16((uint32_t)bChannelDiag.twaiRxErrNow), mode);
    }

    bool matchesLastEcho(const CanFrame &frame) const
    {
        if (!lastEchoValid_ || frame.id != lastEcho_.id || frame.dlc != lastEcho_.dlc)
            return false;
        return memcmp(frame.data, lastEcho_.data, frame.dlc) == 0;
    }

    void setPhase(uint8_t phase, uint32_t now)
    {
        if ((uint8_t)bChannelDiag.modeBPhase != phase)
            bChannelDiag.modeBPhaseEnterMs = now;
        bChannelDiag.modeBPhase = phase;
    }

    void updateDasState(const CanFrame &frame, uint32_t now)
    {
        if (frame.dlc < 6)
            return;
        uint8_t apState = static_cast<uint8_t>(frame.data[0] & 0x0F);
        uint8_t handsOnState = static_cast<uint8_t>((frame.data[5] >> 2) & 0x0F);
        apState_ = apState;
        lastDasStateMs_ = now;
        dasStateSeen_ = true;
        bChannelDiag.dasAutopilotStateRx = apState;
        bChannelDiag.dasHandsOnStateRx = handsOnState;
        bChannelDiag.dasStatusSourceId = frame.id;
        bChannelDiag.lastDasStatusRxMs = now;
        if (frame.id == 921) bChannelDiag.last921RxMs = now;
        else                 bChannelDiag.last923RxMs = now;

        if (!handsOnStateSeen_ || handsOnState != handsOnState_)
        {
            handsOnState_ = handsOnState;
            dasHandsOnState = handsOnState;
            handsOnStateEnteredMs_ = now;
            handsOnStateSeen_ = true;
            bChannelDiag.modeBStateEnterMs = now;
            bChannelDiag.modeBFirstEchoDelayMs = 0;
        }
    }

    void updateSteering(const CanFrame &frame, uint32_t now)
    {
        bChannelDiag.frames297 = (uint32_t)bChannelDiag.frames297 + 1;
        if (frame.dlc < 4 || ((frame.data[3] >> 6) & 0x03) != 1)
        {
            steeringSeen_ = false;
            bChannelDiag.steeringAngleValid = false;
            return;
        }
        uint16_t raw = static_cast<uint16_t>((frame.data[2] |
                         (static_cast<uint16_t>(frame.data[3]) << 8)) & 0x3FFF);
        steeringAngleDeg_ = raw * 0.1f - 819.2f;
        lastSteeringMs_ = now;
        steeringSeen_ = true;
        bChannelDiag.steeringAngleDeg = steeringAngleDeg_;
        bChannelDiag.steeringAngleValid = true;
        bChannelDiag.last297RxMs = now;
    }

    bool decideInjection(uint8_t selectedMode, uint32_t now, uint16_t &torque,
                         bool &setHandsOn)
    {
        if (selectedMode == kNagMode1)
        {
            setPhase(1, now);
            torque = kNagTorqueRawMax;
            setHandsOn = true;
            return true;
        }

        if (selectedMode == kNagMode2)
        {
            constexpr uint16_t kTorques[] = {0x08B6, 0x0898, 0x076C, 0x074E};
            constexpr uint32_t kCycleMs = kMode2BurstMs + kMode2PauseMs;
            if ((now - modeEnteredMs_) % kCycleMs >= kMode2BurstMs)
            {
                setPhase(0, now);
                bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
                return false;
            }
            if (now - mode2LastStepMs_ >= kMode2TorqueStepMs)
            {
                mode2TorqueIndex_ = static_cast<uint8_t>((mode2TorqueIndex_ + 1) % 4);
                mode2LastStepMs_ = now;
            }
            setPhase(2, now);
            torque = kTorques[mode2TorqueIndex_];
            setHandsOn = true;
            return true;
        }

        if (!dasStateSeen_ || !handsOnStateSeen_ ||
            now - lastDasStateMs_ > kContextFreshMs)
        {
            setPhase(0, now);
            bChannelDiag.nagLastDecision = kNagDecisionNo921;
            return false;
        }
        if (!steeringSeen_ || now - lastSteeringMs_ > kContextFreshMs ||
            steeringAngleDeg_ < -5.0f || steeringAngleDeg_ > 5.0f)
        {
            setPhase(0, now);
            bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
            return false;
        }
        if (apState_ < 3 || apState_ > 6)
        {
            bChannelDiag.skipApState++;
            setPhase(0, now);
            bChannelDiag.nagLastDecision = kNagDecisionApBlocked;
            return false;
        }

        float torqueNm = 0.0f;
        if (handsOnState_ == 2)
        {
            if (now - handsOnStateEnteredMs_ < 2000)
            {
                setPhase(3, now);
                bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
                return false;
            }
            walkSeed_ = static_cast<uint16_t>(walkSeed_ * 1103u + 12345u);
            float delta = (static_cast<int>(walkSeed_ & 0x1F) - 16) * 0.05f;
            float magnitude = absolute(lastMode3TorqueNm_) + delta;
            if (magnitude < 0.5f) magnitude = 0.5f;
            if (magnitude > 1.8f) magnitude = 1.8f;
            torqueNm = steeringAngleDeg_ > 0.0f ? -magnitude : magnitude;
            lastMode3TorqueNm_ = torqueNm;
            setPhase(4, now);
        }
        else if (handsOnState_ == 3)
        {
            if (now - handsOnStateEnteredMs_ < 1000)
            {
                setPhase(5, now);
                bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
                return false;
            }
            uint32_t phase = (now - handsOnStateEnteredMs_ - 1000) % 1000;
            torqueNm = phase < 500 ? -1.8f + (phase / 500.0f) * 3.6f
                                   : 1.8f - ((phase - 500) / 500.0f) * 3.6f;
            setPhase(6, now);
        }
        else
        {
            setPhase(0, now);
            bChannelDiag.nagLastDecision = kNagDecisionDasIdle;
            return false;
        }

        torque = torqueNmToRaw(torqueNm);
        setHandsOn = absolute(torqueNm) >= 1.0f;
        return true;
    }

    bool sendEcho(const CanFrame &frame, CanDriver &driver, uint32_t now,
                  uint16_t torque, bool setHandsOn)
    {
        torque = clampTorqueRaw(torque);
        CanFrame echo = frame;
        echo.data[2] = static_cast<uint8_t>((frame.data[2] & 0xF0) |
                                            ((torque >> 8) & 0x0F));
        echo.data[3] = static_cast<uint8_t>(torque & 0xFF);
        echo.data[4] = setHandsOn ? static_cast<uint8_t>(frame.data[4] | 0x40)
                                  : frame.data[4];
        uint8_t counter = static_cast<uint8_t>(((frame.data[6] & 0x0F) + 1) & 0x0F);
        echo.data[6] = static_cast<uint8_t>((frame.data[6] & 0xF0) | counter);
        uint16_t sum = echo.data[0] + echo.data[1] + echo.data[2] + echo.data[3] +
                        echo.data[4] + echo.data[5] + echo.data[6];
        echo.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);

        if (!canTxPermitBegin())
        {
            bChannelDiag.nagLastDecision = kNagDecisionRuntimeOff;
            return false;
        }

        const bool sent = driver.sendCheck(echo);
        canTxPermitEnd();
        if (!sent)
        {
            bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
            return false;
        }

        framesSent++;
        nagEchoCount++;
        lastEcho_ = echo;
        lastEchoValid_ = true;
        bChannelDiag.echoCount++;
        bChannelDiag.lastEchoTxMs = now;
        bChannelDiag.modeBInjectCount = (uint32_t)bChannelDiag.modeBInjectCount + 1;
        bChannelDiag.modeBLastTorqueNm = torque * 0.01f - 20.5f;
        uint32_t enterMs = (uint32_t)bChannelDiag.modeBStateEnterMs;
        if (enterMs != 0 && (uint32_t)bChannelDiag.modeBFirstEchoDelayMs == 0)
        {
            uint32_t delayMs = now - enterMs;
            bChannelDiag.modeBFirstEchoDelayMs = delayMs == 0 ? 1 : delayMs;
        }
        bChannelDiag.nagLastDecision = kNagDecisionEcho;
        return true;
    }
};
