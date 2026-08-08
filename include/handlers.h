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
        // RXF0~1 → RXB0, RXF2~5 → RXB1. RXF5는 드라이버가 ids[0]
        // (1021)로 채워 두 버퍼에서 1021을 받을 수 있게 한다.
        static constexpr uint32_t ids[] = {1021, 280, 921, 1016, 390};
        return ids;
    }
    uint8_t filterIdCount() const override { return 5; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        const uint32_t nowMs = millis();
        aChannelDiag.framesReceivedTotal++;
        aChannelDiag.lastFrameIdReceived = frame.id;

        if (frame.id == 280) {
            aChannelDiag.frames280++;
            summonHandle280(frame, nowMs);
            trackSummoningState(nowMs);
            return;
        }

        if (frame.id == 390) {
            aChannelDiag.frames390++;
            summonHandle390(frame, nowMs);
            trackSummoningState(nowMs);
            return;
        }

        if (frame.id == 921) {
            aChannelDiag.frames921++;
            summonHandle921(frame, nowMs);
            return;
        }

        if (frame.id == 1016) {
            aChannelDiag.frames1016++;
            summonHandle1016(frame);
            trackSummoningState(nowMs);
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
                const CanTxResult txResult = driver.sendDetailed(frame, CanTxSource::Tsllc);
                if (txResult == CanTxResult::Queued) {
                    const uint32_t sentAtMs = millis();
                    aChannelDiag.aTxOk++;
                    aChannelDiag.tsllcTxOk++;
                    aChannelDiag.lastTxMs = sentAtMs;
                    aChannelDiag.lastTsllcTxMs = sentAtMs;
                } else if (txResult == CanTxResult::Busy) {
                    aChannelDiag.aTxBusy++;
                    aChannelDiag.tsllcTxBusy++;
                } else {
                    aChannelDiag.aTxFail++;
                    aChannelDiag.tsllcTxFail++;
                }
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
            const bool gateOpen = summonGateOpen(nowMs);
            const bool summonInject = summonEnabled && gateOpen;

            if (summonEnabled && !gateOpen) {
                summonGateDiag.blocked = (uint32_t)summonGateDiag.blocked + 1;
                summonGateDiag.lastBlockedMs = nowMs;
            }

            if (summonInject) {
                summonGateDiag.mux1Received = (uint32_t)summonGateDiag.mux1Received + 1;
                if (shouldSkipATx("SummonUnlock")) return;
                if (!canTxPermitBegin()) return;
                setBit(frame, 19, false);  // UI_applyEceR79=0 (ECE R79 적용 해제)
#if defined(HW3)
                setBit(frame, 46, true); // 검증된 HW3 Summon 제한 해제 비트
#elif defined(HW4)
                setBit(frame, 47, true); // HW4 빌드 호환 경로
#endif
                aChannelDiag.summonUnlockModifiedCount++;
                framesSent++;
                const CanTxResult txResult = driver.sendDetailed(frame, CanTxSource::Summon);
                if (txResult == CanTxResult::Queued) {
                    const uint32_t sentAtMs = millis();
                    aChannelDiag.aTxOk++;
                    aChannelDiag.lastTxMs = sentAtMs;
                    summonGateDiag.txOk = (uint32_t)summonGateDiag.txOk + 1;
                    summonGateDiag.lastTxMs = sentAtMs;
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
                } else if (txResult == CanTxResult::Busy) {
                    aChannelDiag.aTxBusy++;
                    summonGateDiag.txBusy = (uint32_t)summonGateDiag.txBusy + 1;
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

private:
    void trackSummoningState(uint32_t nowMs)
    {
        const bool active = (bool)summonGateDiag.summoning;
        if (!summoningStateSeen_) {
            summoningStateSeen_ = true;
            lastSummoningState_ = active;
            if (!active) return;
        } else if (active == lastSummoningState_) {
            return;
        }

        const uint32_t txOk = (uint32_t)summonGateDiag.txOk;
        const uint32_t txFail = (uint32_t)summonGateDiag.txFail;
        const uint32_t blocked = (uint32_t)summonGateDiag.blocked;
        const uint32_t detail = eventSummoningStateDetail(
            active, (bool)summonGateDiag.parked, (bool)summonGateDiag.acaActive,
            (bool)summonGateDiag.sprSeen, summonGateOpen(nowMs),
            summonGateReasonCode(nowMs),
            active ? 0U : txOk - summoningStartTxOk_,
            active ? 0U : txFail - summoningStartTxFail_,
            active ? 0U : blocked - summoningStartBlocked_);
        eventLogPush(EV_SUMMONING_STATE,
                     (uint16_t)(uint8_t)aChannelDiag.aTec,
                     (uint16_t)(uint8_t)aChannelDiag.aRec, detail);
        if (active) {
            summoningStartTxOk_ = txOk;
            summoningStartTxFail_ = txFail;
            summoningStartBlocked_ = blocked;
        }
        lastSummoningState_ = active;
    }

    bool summoningStateSeen_ = false;
    bool lastSummoningState_ = false;
    uint32_t summoningStartTxOk_ = 0;
    uint32_t summoningStartTxFail_ = 0;
    uint32_t summoningStartBlocked_ = 0;
};
 

// ===================================================================
// [B채널] HW3 NagHandler — 검증된 MODE 1/2 이식
// ===================================================================
struct NagHandler : public CarManagerBase
{
    Shared<bool> nagKillerActive{true};
    Shared<uint32_t> nagEchoCount{0};
    Shared<uint8_t> dasHandsOnState{0xFF};

    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {880, 921, 923};
        return ids;
    }

    uint8_t filterIdCount() const override { return 3; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        handleMessageAt(frame, driver, millis(), true, micros());
    }

    void handleMessageAt(CanFrame &frame, CanDriver &driver, uint32_t now,
                         bool transmissionAllowed = true, uint32_t rxUs = 0)
    {
        if (rxUs == 0) rxUs = micros();
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
            storeRawFrame(frame, bChannelDiag.rawDasSeq,
                          bChannelDiag.rawDasLow, bChannelDiag.rawDasHigh);
            updateDasState(frame, now);
            return;
        }
        if (frame.id != 880 || frame.dlc < 8)
            return;

        bChannelDiag.last880RxMs = now;
        uint8_t handsOn = static_cast<uint8_t>((frame.data[4] >> 6) & 0x03);
        uint16_t torqueRaw = static_cast<uint16_t>(((frame.data[2] & 0x0F) << 8) |
                                                    frame.data[3]);

        // 최근 송신 8개와 전체 프레임을 비교한다. 단순히 HO=1/+1.8Nm인
        // 실제 차량 프레임을 자기 에코로 오인하지 않는다.
        uint32_t echoTxUs = 0;
        if (matchesRecentEcho(frame, echoTxUs))
        {
            bChannelDiag.echoConfirmCount++;
            bChannelDiag.lastEchoRxMs = now;
            bChannelDiag.echoLatUs = micros() - echoTxUs;
            return;
        }

        storeRawFrame(frame, bChannelDiag.raw880Seq,
                      bChannelDiag.raw880Low, bChannelDiag.raw880High);
        bChannelDiag.realHo = handsOn;
        bChannelDiag.realTorqueNm = torqueRaw * 0.01f - 20.5f;
        targetFramesSeen_++;
        bChannelDiag.nagWarmupFramesSeen = targetFramesSeen_;
        updateWarmupReadiness(now);

        if (!transmissionAllowed || !nagKillerActive || !nagKillerRuntime)
        {
            bChannelDiag.skipRuntimeOrInactive++;
            bChannelDiag.nagLastDecision = kNagDecisionRuntimeOff;
            return;
        }

        if (!(bool)bChannelDiag.nagReady)
        {
            bChannelDiag.skipWarmup++;
            bChannelDiag.nagLastDecision = kNagDecisionWarmup;
            return;
        }

        // 실제 운전자가 핸들을 잡은 상태에서는 모든 모드가 감시만 한다.
        if (handsOn != 0)
        {
            bChannelDiag.skipHandsOn++;
            bChannelDiag.nagLastDecision = kNagDecisionHandsOn;
            return;
        }

        // Mode 1/2는 원본 선제 주입 또는 AP 전용을 사용자가 선택한다.
        // AP 전용에서는 오래된 상태를 허용하지 않아 일반 주행 중 주입하지 않는다.
        if ((bool)nagApOnlyRuntime)
        {
            if (!dasStateSeen_ || now - lastDasStateMs_ > kContextFreshMs)
            {
                bChannelDiag.skipApState++;
                bChannelDiag.nagLastDecision = kNagDecisionNo921;
                return;
            }
            if (!nagApStateAllowsInjection(apState_))
            {
                bChannelDiag.skipApState++;
                bChannelDiag.nagLastDecision = kNagDecisionApBlocked;
                return;
            }
        }

        uint16_t torque = kNagTorqueRawMax;
        bool setHandsOn = true;
        if (!decideInjection(selectedMode, now, torque, setHandsOn))
            return;

        sendEcho(frame, driver, now, rxUs, torque, setHandsOn);
    }

    void onCanStarted(uint32_t now)
    {
        canStarted_ = true;
        canStartedMs_ = now;
        targetFramesSeen_ = 0;
        bChannelDiag.nagWarmupStartMs = now;
        bChannelDiag.nagWarmupFramesSeen = 0;
        bChannelDiag.nagReady = false;
        bChannelDiag.nagReadiness = kNagReadinessWarmupTime;
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
    uint32_t lastDasStateMs_ = 0;
    bool dasStateSeen_ = false;
    struct RecentEcho
    {
        CanFrame frame;
        uint32_t txUs;
        bool valid;
    };
    RecentEcho recentEchoes_[kNagRecentEchoSlots] = {};
    uint8_t recentEchoWrite_ = 0;
    bool canStarted_ = false;
    uint32_t canStartedMs_ = 0;
    uint32_t targetFramesSeen_ = 0;

    static uint32_t packFrameWord(const CanFrame &frame, uint8_t offset)
    {
        uint32_t packed = 0;
        for (uint8_t i = 0; i < 4; ++i)
        {
            const uint8_t index = static_cast<uint8_t>(offset + i);
            if (index < frame.dlc)
                packed |= static_cast<uint32_t>(frame.data[index]) << (i * 8);
        }
        return packed;
    }

    static void storeRawFrame(const CanFrame &frame, Shared<uint32_t> &sequence,
                              Shared<uint32_t> &low,
                              Shared<uint32_t> &high)
    {
        const uint32_t next = (uint32_t)sequence + 1U;
        sequence = next;  // 홀수: 갱신 중
        low = packFrameWord(frame, 0);
        high = packFrameWord(frame, 4);
        sequence = next + 1U;  // 짝수: 일관된 스냅샷
    }

    void updateWarmupReadiness(uint32_t now)
    {
        // Native 단위 테스트와 독립 핸들러 사용은 기존 즉시 동작을 유지한다.
        // 실차에서는 TWAI init 성공 직후 onCanStarted()가 반드시 호출된다.
        if (!canStarted_)
        {
            bChannelDiag.nagReady = true;
            bChannelDiag.nagReadiness = kNagReadinessReady;
            return;
        }

        if (now - canStartedMs_ < kNagWarmupMs)
        {
            bChannelDiag.nagReady = false;
            bChannelDiag.nagReadiness = kNagReadinessWarmupTime;
            return;
        }
        if (targetFramesSeen_ < kNagWarmupTargetFrames)
        {
            bChannelDiag.nagReady = false;
            bChannelDiag.nagReadiness = kNagReadinessWarmupFrames;
            return;
        }
        bChannelDiag.nagReady = true;
        bChannelDiag.nagReadiness = kNagReadinessReady;
    }

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
        lastDasStateMs_ = 0;
        dasStateSeen_ = false;
        bChannelDiag.modeBPhase = 0;
        eventLogPush(EV_NAG_MODE,
                     clampU16((uint32_t)bChannelDiag.twaiTxErrNow),
                     clampU16((uint32_t)bChannelDiag.twaiRxErrNow), mode);
    }

    bool matchesRecentEcho(const CanFrame &frame, uint32_t &txUs) const
    {
        const uint32_t nowUs = micros();
        for (uint8_t i = 0; i < kNagRecentEchoSlots; ++i)
        {
            const RecentEcho &entry = recentEchoes_[i];
            if (!entry.valid || frame.id != entry.frame.id || frame.dlc != entry.frame.dlc)
                continue;
            if (nowUs - entry.txUs > kNagEchoMatchMaxAgeUs)
                continue;
            if (memcmp(frame.data, entry.frame.data, frame.dlc) == 0)
            {
                txUs = entry.txUs;
                return true;
            }
        }
        return false;
    }

    void rememberEcho(const CanFrame &frame, uint32_t txUs)
    {
        RecentEcho &entry = recentEchoes_[recentEchoWrite_];
        entry.frame = frame;
        entry.txUs = txUs;
        entry.valid = true;
        recentEchoWrite_ = static_cast<uint8_t>((recentEchoWrite_ + 1) %
                                                kNagRecentEchoSlots);
    }

    void setPhase(uint8_t phase, uint32_t now)
    {
        (void)now;
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

        dasHandsOnState = handsOnState;
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
                bChannelDiag.nagLastDecision = kNagDecisionModePause;
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

        bChannelDiag.nagLastDecision = kNagDecisionRuntimeOff;
        return false;
    }

    bool sendEcho(const CanFrame &frame, CanDriver &driver, uint32_t now,
                  uint32_t rxUs,
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

        const uint32_t elapsedUs = micros() - rxUs;
        if (elapsedUs > kNagEchoDeadlineUs)
        {
            bChannelDiag.echoDroppedLate++;
            bChannelDiag.nagLastDecision = kNagDecisionLateDrop;
            return false;
        }

        if (!canTxPermitBegin())
        {
            bChannelDiag.nagLastDecision = kNagDecisionRuntimeOff;
            return false;
        }

        bChannelDiag.txAttemptCount++;
        const bool sent = driver.sendCheck(echo);
        canTxPermitEnd();
        if (!sent)
        {
            bChannelDiag.nagLastDecision = kNagDecisionNoEcho;
            return false;
        }

        framesSent++;
        nagEchoCount++;
        const uint32_t txDoneUs = micros();
        rememberEcho(echo, txDoneUs);
        bChannelDiag.echoCount++;
        bChannelDiag.txSuccessCount++;
        bChannelDiag.txLatencyUs = txDoneUs - rxUs;
        bChannelDiag.lastEchoTxMs = now;
        bChannelDiag.modeBInjectCount = (uint32_t)bChannelDiag.modeBInjectCount + 1;
        bChannelDiag.modeBLastTorqueNm = torque * 0.01f - 20.5f;
        bChannelDiag.lastTxTorqueNm = torque * 0.01f - 20.5f;
        bChannelDiag.lastTxHandsOn =
            static_cast<uint8_t>((echo.data[4] >> 6) & 0x03);
        if (!dasStateSeen_)
            bChannelDiag.nagFiredNoDas++;
        bChannelDiag.nagLastDecision = kNagDecisionEcho;
        return true;
    }
};
