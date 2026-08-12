// 5초 간격 시계열 통계 수집 (최근 20분, RAM wrap-around)
// 통합 로그 [4] 섹션에 포함되며, /api/timeseries.csv는 디버그용 보조 엔드포인트다.
#pragma once
#include "can_helpers.h"
#include "event_log.h"
#include "version.h"

#ifndef NATIVE_BUILD
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>
#include <esp_http_server.h>
#include <Arduino.h>
#include <stdio.h>

struct TsSample {
    uint32_t t_ms;
    uint8_t  captureMode; // 0=자동 최근 20분, 1=사용자 수동 기록
    uint32_t busoff;
    uint32_t tec;
    uint32_t rec;
    uint32_t arbLost;
    uint32_t busErr;
    uint32_t txFail;
    uint32_t echoCnt;
    uint32_t f880;
    uint32_t f921;
    uint32_t f923;
    uint32_t echoDrop;
    uint32_t skipRuntime;
    uint32_t skipAp;
    uint32_t skipHandsOn;
    uint32_t skipDas;
    uint32_t noDasEcho;
    // 사용자가 "지금 경고가 떴다" 버튼을 누른 횟수. dUserMark가 해당 5초 구간 기준점이다.
    uint32_t userMark;
    uint16_t d880;
    uint16_t d921;
    uint16_t d923;
    uint16_t dEcho;
    uint16_t dDrop;
    uint16_t dSkipRuntime;
    uint16_t dSkipAp;
    uint16_t dSkipHandsOn;
    uint16_t dSkipDas;
    uint16_t dNoDasEcho;
    uint16_t dUserMark;
    uint8_t  handsOn;
    uint8_t  dasState;
    uint8_t  nagMode;
    uint8_t  nagModeDefault;
    uint16_t dasSourceId;
    uint8_t  lastDecision;
    // intervalDecision은 5초 구간 요약, lastDecision은 마지막 880 처리 분기다.
    uint8_t  intervalDecision;
    uint32_t modeBInject;
    uint16_t dModeBInject;
    uint8_t  apState;
    uint8_t  modeBPhase;
    float    realTorqueNm;
    float    modeBLastNm;
    uint16_t age880Ms;
    uint16_t ageDasMs;
    uint16_t ageEchoMs;
    // A채널 MCP2515 상태. 간헐 EFLG 경고와 RX 오버런의 원인 추적용.
    uint32_t aFrames;
    uint32_t aTxOk;
    uint32_t aTxBusy;
    uint32_t aTxFail;
    uint32_t aTxCompleted;
    uint32_t aTxArbitrationLost;
    uint32_t aTxAborted;
    uint32_t aMerrf;
    uint32_t aRxOvr;
    uint32_t aRx0Ovr;
    uint32_t aRx1Ovr;
    uint32_t aRxBuffer0Frames;
    uint32_t aRxBuffer1Frames;
    uint32_t aRxDrainFrames;
    uint32_t aRxDrainCalls;
    uint32_t aRxQueueHighWater;
    uint32_t aRxQueueDrops;
    uint32_t aEflgEvents;
    uint16_t dAFrames;
    uint16_t dATxOk;
    uint16_t dATxFail;
    uint16_t dAMerrf;
    uint16_t dARxOvr;
    uint16_t dAEflgEvents;
    uint16_t aFrameAgeMs;
    uint16_t aLoopAgeMs;
    uint16_t aGuardRemainingMs;
    float    aFrameHz;
    uint8_t  aEflg;
    uint8_t  aEflgPeak;
    uint8_t  aTec;
    uint8_t  aRec;
    uint8_t  aTecPeak;
    uint8_t  aRecPeak;
    uint8_t  aGuardActive;
    uint8_t  aGuardReason;
    uint32_t aWakeCount;
    uint32_t aWakeToSummonTxMs;
    uint8_t  aWakeAwaitingTx;
    // 샘플 순간의 런타임 플래그. 다운로드 시점의 현재값을 과거 행에 잘못 붙이지 않는다.
    uint8_t  aDriverOk;
    uint8_t  aTxEnabled;
    uint8_t  summonEnabled;
    uint8_t  tsllcEnabled;
    uint8_t  aOneShotEnabled;
    uint8_t  aTxGuardEnabled;
    uint8_t  aSpiMhz;
    uint8_t  aSpiTargetMhz;
    uint8_t  bDriverState;
    uint8_t  nagEnabled;
    // 기능 토글과 실제 주입 결과를 같은 시점에서 검증하기 위한 상태.
    uint8_t  summonGateOpen;
    uint8_t  summonConditionLimit;
    uint8_t  summonApState;
    uint8_t  summonApActive;
    uint8_t  summonParked;
    uint8_t  summoning;
    uint8_t  summonGateReason;
    uint16_t summonApStableMs;
    uint8_t  summonInjectReady;
    uint8_t  tsllcInjectReady;
    uint8_t  nagApOnly;
    uint8_t  nagApActive;
    uint8_t  nagInjecting;
    uint32_t summonTxOk;
    uint32_t summonTxFail;
    uint32_t summonBlocked;
    uint32_t tsllcTxOk;
    uint32_t tsllcTxFail;
    uint32_t summonTxCompleted;
    uint32_t summonTxArbitrationLost;
    uint32_t summonTxAborted;
    uint32_t summonTxError;
    uint32_t tsllcTxCompleted;
    uint32_t tsllcTxArbitrationLost;
    uint32_t tsllcTxAborted;
    uint32_t tsllcTxError;
    uint16_t dSummonTxOk;
    uint16_t dSummonTxFail;
    uint16_t dSummonBlocked;
    uint16_t dTsllcTxOk;
    uint16_t dTsllcTxFail;
    uint32_t aLoopGapLastUs;
    uint32_t aLoopGapPeakUs;
    uint32_t aLoopGapOver250us;
    uint32_t aLoopGapOver500us;
    uint32_t aLoopGapOver1ms;
    uint32_t aLoopGapOver2ms;
    uint16_t dALoopGapOver2ms;
    uint8_t  aLastOverrunPhase;
};

static constexpr size_t TS_CAP = 240;  // 240 × 5s = 20분
inline TsSample tsBuf[TS_CAP];
inline volatile size_t tsHead = 0;
inline volatile size_t tsCount = 0;
inline volatile uint32_t tsResetMs = 0;  // 마지막 리셋 시각 (CSV 메타용, 0=부팅 이후 리셋 없음)
inline volatile uint32_t tsRecStartMs = 0;  // 사용자 '기록시작' 시각 (0=정지/미시작)
inline volatile bool tsRecording = false;   // 수동 기록 중 여부
inline portMUX_TYPE tsMux = portMUX_INITIALIZER_UNLOCKED;

inline volatile uint32_t tsBaseBusoff = 0;
inline volatile uint32_t tsBaseArbLost = 0;
inline volatile uint32_t tsBaseBusErr = 0;
inline volatile uint32_t tsBaseTxFail = 0;
inline volatile uint32_t tsBaseEcho = 0;
inline volatile uint32_t tsBaseF880 = 0;
inline volatile uint32_t tsBaseF921 = 0;
inline volatile uint32_t tsBaseF923 = 0;
inline volatile uint32_t tsBaseModeBInject = 0;
inline volatile uint32_t tsBaseEchoDrop = 0;
inline volatile uint32_t tsBaseSkipRuntime = 0;
inline volatile uint32_t tsBaseSkipAp = 0;
inline volatile uint32_t tsBaseSkipHandsOn = 0;
inline volatile uint32_t tsBaseSkipDas = 0;
inline volatile uint32_t tsBaseNoDasEcho = 0;
inline volatile uint32_t tsBaseUserMark = 0;
inline volatile uint32_t tsBaseAFrames = 0;
inline volatile uint32_t tsBaseATxOk = 0;
inline volatile uint32_t tsBaseATxFail = 0;
inline volatile uint32_t tsBaseAMerrf = 0;
inline volatile uint32_t tsBaseARxOvr = 0;
inline volatile uint32_t tsBaseAEflgEvents = 0;
inline volatile uint32_t tsBaseALoopGapOver2ms = 0;
inline volatile uint32_t tsBaseSummonTxOk = 0;
inline volatile uint32_t tsBaseSummonTxFail = 0;
inline volatile uint32_t tsBaseSummonBlocked = 0;
inline volatile uint32_t tsBaseTsllcTxOk = 0;
inline volatile uint32_t tsBaseTsllcTxFail = 0;

inline volatile uint32_t tsPrevEcho = 0;
inline volatile uint32_t tsPrevF880 = 0;
inline volatile uint32_t tsPrevF921 = 0;
inline volatile uint32_t tsPrevF923 = 0;
inline volatile uint32_t tsPrevModeBInject = 0;
inline volatile uint32_t tsPrevEchoDrop = 0;
inline volatile uint32_t tsPrevSkipRuntime = 0;
inline volatile uint32_t tsPrevSkipAp = 0;
inline volatile uint32_t tsPrevSkipHandsOn = 0;
inline volatile uint32_t tsPrevSkipDas = 0;
inline volatile uint32_t tsPrevNoDasEcho = 0;
inline volatile uint32_t tsPrevUserMark = 0;
inline volatile uint32_t tsPrevAFrames = 0;
inline volatile uint32_t tsPrevATxOk = 0;
inline volatile uint32_t tsPrevATxFail = 0;
inline volatile uint32_t tsPrevAMerrf = 0;
inline volatile uint32_t tsPrevARxOvr = 0;
inline volatile uint32_t tsPrevAEflgEvents = 0;
inline volatile uint32_t tsPrevALoopGapOver2ms = 0;
inline volatile uint32_t tsPrevSummonTxOk = 0;
inline volatile uint32_t tsPrevSummonTxFail = 0;
inline volatile uint32_t tsPrevSummonBlocked = 0;
inline volatile uint32_t tsPrevTsllcTxOk = 0;
inline volatile uint32_t tsPrevTsllcTxFail = 0;
inline volatile uint32_t tsPrevSummonTxCompleted = 0;
inline volatile uint32_t tsPrevSummonTxArbitrationLost = 0;
inline volatile uint32_t tsPrevSummonTxAborted = 0;
inline volatile uint32_t tsPrevTsllcTxCompleted = 0;
inline volatile uint32_t tsPrevTsllcTxArbitrationLost = 0;
inline volatile uint32_t tsPrevTsllcTxAborted = 0;
inline volatile bool tsFeatureActivitySeen = false;
inline volatile uint32_t tsFeatureActivityLast = 0;

inline uint32_t tsDelta(uint32_t current, uint32_t base) {
    return current - base;
}

inline uint16_t tsDelta16(uint32_t current, uint32_t base) {
    uint32_t value = current - base;
    return value > 65535U ? 65535U : (uint16_t)value;
}

inline uint16_t tsElapsed16(uint32_t nowMs, uint32_t startMs) {
    if (startMs == 0 || nowMs < startMs) return 0;
    return tsDelta16(nowMs, startMs);
}

using TsTimeFormatter = void(*)(uint32_t, char*, size_t);
inline TsTimeFormatter tsTimeFormatter = nullptr;

inline void timeseriesFormatTime(uint32_t timestampMs, char *out, size_t outLen) {
    if (tsTimeFormatter) {
        tsTimeFormatter(timestampMs, out, outLen);
    } else {
        snprintf(out, outLen, "uptime:%u", (unsigned)timestampMs);
    }
}

static void timeseriesTaskFn(void*) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        uint32_t manualStartMs = 0;
        bool manualRecording = false;
        portENTER_CRITICAL(&tsMux);
        manualStartMs = tsRecStartMs;
        manualRecording = tsRecording;
        portEXIT_CRITICAL(&tsMux);
        // 수동 기록을 정지한 뒤에는 버퍼를 고정한다. 수동 기록을 시작하지 않은
        // 기본 상태에서는 자동 최근 20분 버퍼가 계속 동작한다.
        if (manualStartMs != 0 && !manualRecording) continue;

        TsSample s{};
        s.t_ms     = millis();
        s.captureMode = manualStartMs != 0 ? 1U : 0U;
        s.busoff   = tsDelta((uint32_t)bChannelDiag.busoffCount, (uint32_t)tsBaseBusoff);
        s.tec      = (uint32_t)bChannelDiag.twaiTxErrNow;
        s.rec      = (uint32_t)bChannelDiag.twaiRxErrNow;
        s.arbLost  = tsDelta((uint32_t)bChannelDiag.bArbLost, (uint32_t)tsBaseArbLost);
        s.busErr   = tsDelta((uint32_t)bChannelDiag.bBusError, (uint32_t)tsBaseBusErr);
        s.txFail   = tsDelta((uint32_t)bChannelDiag.txFail, (uint32_t)tsBaseTxFail);
        uint32_t curEcho = (uint32_t)bChannelDiag.echoCount;
        uint32_t cur880 = (uint32_t)bChannelDiag.frames880;
        uint32_t cur921 = (uint32_t)bChannelDiag.frames921;
        uint32_t cur923 = (uint32_t)bChannelDiag.frames923;
        uint32_t curModeBInject = (uint32_t)bChannelDiag.modeBInjectCount;
        uint32_t curDrop = (uint32_t)bChannelDiag.echoDroppedLate;
        uint32_t curSkipRuntime = (uint32_t)bChannelDiag.skipRuntimeOrInactive;
        uint32_t curSkipAp = (uint32_t)bChannelDiag.skipApState;
        uint32_t curSkipHandsOn = (uint32_t)bChannelDiag.skipHandsOn;
        uint32_t curSkipDas = (uint32_t)bChannelDiag.skipDasState;
        uint32_t curNoDas = (uint32_t)bChannelDiag.nagFiredNoDas;
        uint32_t curUserMark = (uint32_t)userMarkerCount;
        uint32_t curAFrames = (uint32_t)aChannelDiag.framesReceivedTotal;
        uint32_t curATxOk = (uint32_t)aChannelDiag.aTxOk;
        uint32_t curATxFail = (uint32_t)aChannelDiag.aTxFail;
        uint32_t curAMerrf = (uint32_t)aChannelDiag.aMerrfCount;
        uint32_t curARxOvr = (uint32_t)aChannelDiag.aRxOvrCount;
        uint32_t curAEflgEvents = (uint32_t)aChannelDiag.mcpEflgEventCount;
        uint32_t curALoopGapOver2ms = (uint32_t)aChannelDiag.loopGapOver2msCount;
        uint32_t curSummonTxOk = (uint32_t)summonGateDiag.txOk;
        uint32_t curSummonTxFail = (uint32_t)summonGateDiag.txFail;
        uint32_t curSummonBlocked = (uint32_t)summonGateDiag.blocked;
        uint32_t curTsllcTxOk = (uint32_t)aChannelDiag.tsllcTxOk;
        uint32_t curTsllcTxFail = (uint32_t)aChannelDiag.tsllcTxFail;
        s.echoCnt  = tsDelta(curEcho, (uint32_t)tsBaseEcho);
        s.f880     = tsDelta(cur880, (uint32_t)tsBaseF880);
        s.f921     = tsDelta(cur921, (uint32_t)tsBaseF921);
        s.f923     = tsDelta(cur923, (uint32_t)tsBaseF923);
        s.modeBInject = tsDelta(curModeBInject, (uint32_t)tsBaseModeBInject);
        s.echoDrop = tsDelta(curDrop, (uint32_t)tsBaseEchoDrop);
        s.skipRuntime = tsDelta(curSkipRuntime, (uint32_t)tsBaseSkipRuntime);
        s.skipAp = tsDelta(curSkipAp, (uint32_t)tsBaseSkipAp);
        s.skipHandsOn = tsDelta(curSkipHandsOn, (uint32_t)tsBaseSkipHandsOn);
        s.skipDas = tsDelta(curSkipDas, (uint32_t)tsBaseSkipDas);
        s.noDasEcho = tsDelta(curNoDas, (uint32_t)tsBaseNoDasEcho);
        s.userMark = tsDelta(curUserMark, (uint32_t)tsBaseUserMark);
        s.d880 = tsDelta16(cur880, (uint32_t)tsPrevF880);
        s.d921 = tsDelta16(cur921, (uint32_t)tsPrevF921);
        s.d923 = tsDelta16(cur923, (uint32_t)tsPrevF923);
        s.dModeBInject = tsDelta16(curModeBInject, (uint32_t)tsPrevModeBInject);
        s.dEcho = tsDelta16(curEcho, (uint32_t)tsPrevEcho);
        s.dDrop = tsDelta16(curDrop, (uint32_t)tsPrevEchoDrop);
        s.dSkipRuntime = tsDelta16(curSkipRuntime, (uint32_t)tsPrevSkipRuntime);
        s.dSkipAp = tsDelta16(curSkipAp, (uint32_t)tsPrevSkipAp);
        s.dSkipHandsOn = tsDelta16(curSkipHandsOn, (uint32_t)tsPrevSkipHandsOn);
        s.dSkipDas = tsDelta16(curSkipDas, (uint32_t)tsPrevSkipDas);
        s.dNoDasEcho = tsDelta16(curNoDas, (uint32_t)tsPrevNoDasEcho);
        s.dUserMark = tsDelta16(curUserMark, (uint32_t)tsPrevUserMark);
        s.aFrames = tsDelta(curAFrames, (uint32_t)tsBaseAFrames);
        s.aTxOk = tsDelta(curATxOk, (uint32_t)tsBaseATxOk);
        s.aTxBusy = (uint32_t)aChannelDiag.aTxBusy;
        s.aTxFail = tsDelta(curATxFail, (uint32_t)tsBaseATxFail);
        s.aTxCompleted = (uint32_t)aChannelDiag.aTxCompleted;
        s.aTxArbitrationLost = (uint32_t)aChannelDiag.aTxArbitrationLost;
        s.aTxAborted = (uint32_t)aChannelDiag.aTxAborted;
        s.aMerrf = tsDelta(curAMerrf, (uint32_t)tsBaseAMerrf);
        s.aRxOvr = tsDelta(curARxOvr, (uint32_t)tsBaseARxOvr);
        s.aRx0Ovr = (uint32_t)aChannelDiag.aRx0OvrCount;
        s.aRx1Ovr = (uint32_t)aChannelDiag.aRx1OvrCount;
        s.aRxBuffer0Frames = (uint32_t)aChannelDiag.aRxBuffer0Frames;
        s.aRxBuffer1Frames = (uint32_t)aChannelDiag.aRxBuffer1Frames;
        s.aRxDrainFrames = (uint32_t)aChannelDiag.aRxDrainFrames;
        s.aRxDrainCalls = (uint32_t)aChannelDiag.aRxDrainCalls;
        s.aRxQueueHighWater = (uint32_t)aChannelDiag.aRxQueueHighWater;
        s.aRxQueueDrops = (uint32_t)aChannelDiag.aRxQueueDropCount;
        s.aEflgEvents = tsDelta(curAEflgEvents, (uint32_t)tsBaseAEflgEvents);
        s.dAFrames = tsDelta16(curAFrames, (uint32_t)tsPrevAFrames);
        s.dATxOk = tsDelta16(curATxOk, (uint32_t)tsPrevATxOk);
        s.dATxFail = tsDelta16(curATxFail, (uint32_t)tsPrevATxFail);
        s.dAMerrf = tsDelta16(curAMerrf, (uint32_t)tsPrevAMerrf);
        s.dARxOvr = tsDelta16(curARxOvr, (uint32_t)tsPrevARxOvr);
        s.dAEflgEvents = tsDelta16(curAEflgEvents, (uint32_t)tsPrevAEflgEvents);
        s.summonTxOk = tsDelta(curSummonTxOk, (uint32_t)tsBaseSummonTxOk);
        s.summonTxFail = tsDelta(curSummonTxFail, (uint32_t)tsBaseSummonTxFail);
        s.summonBlocked = tsDelta(curSummonBlocked, (uint32_t)tsBaseSummonBlocked);
        s.tsllcTxOk = tsDelta(curTsllcTxOk, (uint32_t)tsBaseTsllcTxOk);
        s.tsllcTxFail = tsDelta(curTsllcTxFail, (uint32_t)tsBaseTsllcTxFail);
        s.summonTxCompleted = (uint32_t)aChannelDiag.aTxCompletedSummon;
        s.summonTxArbitrationLost =
            (uint32_t)aChannelDiag.aTxArbitrationLostSummon;
        s.summonTxAborted = (uint32_t)aChannelDiag.aTxAbortedSummon;
        s.summonTxError = (uint32_t)aChannelDiag.aTxFailSummon;
        s.tsllcTxCompleted = (uint32_t)aChannelDiag.aTxCompletedTsllc;
        s.tsllcTxArbitrationLost =
            (uint32_t)aChannelDiag.aTxArbitrationLostTsllc;
        s.tsllcTxAborted = (uint32_t)aChannelDiag.aTxAbortedTsllc;
        s.tsllcTxError = (uint32_t)aChannelDiag.aTxFailTsllc;
        const uint16_t dSummonTxCompleted = tsDelta16(
            s.summonTxCompleted, (uint32_t)tsPrevSummonTxCompleted);
        const uint16_t dSummonTxArbitrationLost = tsDelta16(
            s.summonTxArbitrationLost, (uint32_t)tsPrevSummonTxArbitrationLost);
        const uint16_t dSummonTxAborted = tsDelta16(
            s.summonTxAborted, (uint32_t)tsPrevSummonTxAborted);
        const uint16_t dTsllcTxCompleted = tsDelta16(
            s.tsllcTxCompleted, (uint32_t)tsPrevTsllcTxCompleted);
        const uint16_t dTsllcTxArbitrationLost = tsDelta16(
            s.tsllcTxArbitrationLost, (uint32_t)tsPrevTsllcTxArbitrationLost);
        const uint16_t dTsllcTxAborted = tsDelta16(
            s.tsllcTxAborted, (uint32_t)tsPrevTsllcTxAborted);
        s.dSummonTxOk = tsDelta16(curSummonTxOk, (uint32_t)tsPrevSummonTxOk);
        s.dSummonTxFail = tsDelta16(curSummonTxFail, (uint32_t)tsPrevSummonTxFail);
        s.dSummonBlocked = tsDelta16(curSummonBlocked, (uint32_t)tsPrevSummonBlocked);
        s.dTsllcTxOk = tsDelta16(curTsllcTxOk, (uint32_t)tsPrevTsllcTxOk);
        s.dTsllcTxFail = tsDelta16(curTsllcTxFail, (uint32_t)tsPrevTsllcTxFail);
        s.aLoopGapLastUs = (uint32_t)aChannelDiag.loopGapLastUs;
        s.aLoopGapPeakUs = (uint32_t)aChannelDiag.loopGapPeakUs;
        s.aLoopGapOver250us = (uint32_t)aChannelDiag.loopGapOver250usCount;
        s.aLoopGapOver500us = (uint32_t)aChannelDiag.loopGapOver500usCount;
        s.aLoopGapOver1ms = (uint32_t)aChannelDiag.loopGapOver1msCount;
        s.aLoopGapOver2ms = tsDelta(curALoopGapOver2ms, (uint32_t)tsBaseALoopGapOver2ms);
        s.dALoopGapOver2ms = tsDelta16(curALoopGapOver2ms, (uint32_t)tsPrevALoopGapOver2ms);
        s.aLastOverrunPhase = (uint8_t)aChannelDiag.lastOverrunPhase;
        s.handsOn  = (uint8_t)bChannelDiag.realHo;
        s.dasState = (uint8_t)bChannelDiag.dasHandsOnStateRx;
        s.nagMode = (uint8_t)bChannelDiag.nagMode;
        s.nagModeDefault = s.nagMode == kNagModeDefault ? 1 : 0;
        s.dasSourceId = (uint16_t)(uint32_t)bChannelDiag.dasStatusSourceId;
        s.lastDecision = (uint8_t)bChannelDiag.nagLastDecision;
        s.intervalDecision = nagIntervalDecision(s.d880, s.d921 + s.d923, s.dEcho, s.dDrop,
            s.dSkipRuntime, s.dSkipHandsOn, s.dSkipDas,
            (bool)nagKillerRuntime, s.dSkipAp);
        s.apState = (uint8_t)bChannelDiag.dasAutopilotStateRx;
        s.modeBPhase = (uint8_t)bChannelDiag.modeBPhase;
        s.realTorqueNm = (float)bChannelDiag.realTorqueNm;
        s.modeBLastNm = (float)bChannelDiag.modeBLastTorqueNm;
        s.age880Ms = tsElapsed16(s.t_ms, (uint32_t)bChannelDiag.last880RxMs);
        s.ageDasMs = tsElapsed16(s.t_ms, (uint32_t)bChannelDiag.lastDasStatusRxMs);
        s.ageEchoMs = tsElapsed16(s.t_ms, (uint32_t)bChannelDiag.lastEchoTxMs);
        s.aFrameHz = (float)aChannelDiag.frameHz;
        s.aEflg = (uint8_t)aChannelDiag.mcpEflg;
        s.aEflgPeak = (uint8_t)aChannelDiag.mcpEflgPeak;
        s.aTec = (uint8_t)aChannelDiag.aTec;
        s.aRec = (uint8_t)aChannelDiag.aRec;
        s.aTecPeak = (uint8_t)aChannelDiag.aTecPeak;
        s.aRecPeak = (uint8_t)aChannelDiag.aRecPeak;
        s.aFrameAgeMs = tsElapsed16(s.t_ms, (uint32_t)aChannelDiag.lastFrameRxMs);
        s.aLoopAgeMs = tsElapsed16(s.t_ms, (uint32_t)aChannelDiag.lastLoopMs);
        s.aGuardActive = aTxGuardActive(s.t_ms) ? 1 : 0;
        s.aGuardReason = (uint8_t)aChannelDiag.aTxGuardLastReason;
        s.aWakeCount = (uint32_t)aChannelDiag.wakeCount;
        s.aWakeToSummonTxMs = (uint32_t)aChannelDiag.wakeToSummonTxMs;
        s.aWakeAwaitingTx = (bool)aChannelDiag.wakeAwaitingSummonTx ? 1U : 0U;
        s.aDriverOk = (bool)aChannelDiag.driverInitialized ? 1U : 0U;
        s.aTxEnabled = (bool)aChannelTxRuntime ? 1U : 0U;
        s.summonEnabled = (bool)summonUnlockRuntime ? 1U : 0U;
        s.tsllcEnabled = (bool)tsllcRuntime ? 1U : 0U;
        s.aOneShotEnabled = (bool)aMcpOneShotRuntime ? 1U : 0U;
        s.aTxGuardEnabled = (bool)aTxGuardRuntime ? 1U : 0U;
        s.aSpiMhz = (uint8_t)((uint32_t)aMcpSpiFreqHz / 1000000UL);
        s.aSpiTargetMhz = (uint8_t)((uint32_t)aMcpRequestedSpiFreqHz / 1000000UL);
        s.bDriverState = (uint8_t)bChannelDiag.twaiStateCode;
        s.nagEnabled = (bool)nagKillerRuntime ? 1U : 0U;
        s.summonGateOpen = summonGateOpen(s.t_ms) ? 1U : 0U;
        s.summonConditionLimit = (bool)summonConditionLimitRuntime ? 1U : 0U;
        s.summonApState = (uint8_t)summonGateDiag.apState;
        s.summonApActive = (bool)summonGateDiag.apActive ? 1U : 0U;
        s.summonParked = (bool)summonGateDiag.parked ? 1U : 0U;
        s.summoning = (bool)summonGateDiag.summoning ? 1U : 0U;
        s.summonGateReason = summonGateReasonCode(s.t_ms);
        s.summonApStableMs = tsDelta16(summonApStableMs(s.t_ms), 0);
        s.summonInjectReady =
            s.summonEnabled && s.aTxEnabled && s.summonGateOpen && !s.aGuardActive;
        s.tsllcInjectReady = s.tsllcEnabled && s.aTxEnabled && !s.aGuardActive;
        s.nagApOnly = (bool)nagApOnlyRuntime ? 1U : 0U;
        s.nagApActive = nagApStateAllowsInjection(s.apState) ? 1U : 0U;
        s.nagInjecting = s.dModeBInject > 0 ? 1U : 0U;
        const uint32_t aGuardUntil = (uint32_t)aChannelDiag.aTxGuardUntilMs;
        s.aGuardRemainingMs = aGuardUntil > s.t_ms ? tsDelta16(aGuardUntil, s.t_ms) : 0;
        const uint32_t featureActivity = eventFeatureActivityDetail(
            s.dSummonTxOk > 0, s.dTsllcTxOk > 0, s.dModeBInject > 0,
            s.summonGateOpen != 0, s.aGuardActive != 0, s.nagApActive != 0);
        const bool emitSummonQuality = eventATxQualityIsWarning(
            dSummonTxCompleted, dSummonTxArbitrationLost, dSummonTxAborted);
        const bool emitTsllcQuality = eventATxQualityIsWarning(
            dTsllcTxCompleted, dTsllcTxArbitrationLost, dTsllcTxAborted);
        const uint32_t summonQualityDetail = eventATxQualityDetail(
            (uint8_t)CanTxSource::Summon, dSummonTxCompleted,
            dSummonTxArbitrationLost, dSummonTxAborted);
        const uint32_t tsllcQualityDetail = eventATxQualityDetail(
            (uint8_t)CanTxSource::Tsllc, dTsllcTxCompleted,
            dTsllcTxArbitrationLost, dTsllcTxAborted);
        bool emitFeatureActivity = false;
        portENTER_CRITICAL(&tsMux);
        tsBuf[tsHead] = s;
        tsPrevEcho = curEcho;
        tsPrevF880 = cur880;
        tsPrevF921 = cur921;
        tsPrevF923 = cur923;
        tsPrevModeBInject = curModeBInject;
        tsPrevEchoDrop = curDrop;
        tsPrevSkipRuntime = curSkipRuntime;
        tsPrevSkipAp = curSkipAp;
        tsPrevSkipHandsOn = curSkipHandsOn;
        tsPrevSkipDas = curSkipDas;
        tsPrevNoDasEcho = curNoDas;
        tsPrevUserMark = curUserMark;
        tsPrevAFrames = curAFrames;
        tsPrevATxOk = curATxOk;
        tsPrevATxFail = curATxFail;
        tsPrevAMerrf = curAMerrf;
        tsPrevARxOvr = curARxOvr;
        tsPrevAEflgEvents = curAEflgEvents;
        tsPrevALoopGapOver2ms = curALoopGapOver2ms;
        tsPrevSummonTxOk = curSummonTxOk;
        tsPrevSummonTxFail = curSummonTxFail;
        tsPrevSummonBlocked = curSummonBlocked;
        tsPrevTsllcTxOk = curTsllcTxOk;
        tsPrevTsllcTxFail = curTsllcTxFail;
        tsPrevSummonTxCompleted = s.summonTxCompleted;
        tsPrevSummonTxArbitrationLost = s.summonTxArbitrationLost;
        tsPrevSummonTxAborted = s.summonTxAborted;
        tsPrevTsllcTxCompleted = s.tsllcTxCompleted;
        tsPrevTsllcTxArbitrationLost = s.tsllcTxArbitrationLost;
        tsPrevTsllcTxAborted = s.tsllcTxAborted;
        if (!tsFeatureActivitySeen || tsFeatureActivityLast != featureActivity) {
            tsFeatureActivitySeen = true;
            tsFeatureActivityLast = featureActivity;
            emitFeatureActivity = true;
        }
        tsHead = (tsHead + 1) % TS_CAP;
        if (tsCount < TS_CAP) ++tsCount;
        portEXIT_CRITICAL(&tsMux);
        if (emitFeatureActivity) {
            eventLogPush(EV_FEATURE_ACTIVITY,
                         (uint16_t)bChannelDiag.twaiTxErrNow,
                         (uint16_t)bChannelDiag.twaiRxErrNow,
                         featureActivity);
        }
        if (emitSummonQuality) {
            eventLogPush(EV_A_TX_QUALITY,
                         (uint16_t)aChannelDiag.aTec, (uint16_t)aChannelDiag.aRec,
                         summonQualityDetail);
        }
        if (emitTsllcQuality) {
            eventLogPush(EV_A_TX_QUALITY,
                         (uint16_t)aChannelDiag.aTec, (uint16_t)aChannelDiag.aRec,
                         tsllcQualityDetail);
        }
    }
}

inline void timeseriesStart() {
    xTaskCreatePinnedToCore(timeseriesTaskFn, "ts", 2048, nullptr, 1, nullptr, 0);
}

// 시계열 버퍼 + 기준점을 리셋. 드라이버 누적 카운터는 건드리지 않는다.
inline void timeseriesReset(bool beginManualRecording = false) {
    uint32_t now = millis();
    portENTER_CRITICAL(&tsMux);
    tsHead = 0;
    tsCount = 0;
    tsResetMs = now;
    tsRecStartMs = beginManualRecording ? now : 0;
    tsRecording = beginManualRecording;
    tsBaseBusoff = (uint32_t)bChannelDiag.busoffCount;
    tsBaseArbLost = (uint32_t)bChannelDiag.bArbLost;
    tsBaseBusErr = (uint32_t)bChannelDiag.bBusError;
    tsBaseTxFail = (uint32_t)bChannelDiag.txFail;
    tsBaseEcho = (uint32_t)bChannelDiag.echoCount;
    tsBaseF880 = (uint32_t)bChannelDiag.frames880;
    tsBaseF921 = (uint32_t)bChannelDiag.frames921;
    tsBaseF923 = (uint32_t)bChannelDiag.frames923;
    tsBaseModeBInject = (uint32_t)bChannelDiag.modeBInjectCount;
    tsBaseEchoDrop = (uint32_t)bChannelDiag.echoDroppedLate;
    tsBaseSkipRuntime = (uint32_t)bChannelDiag.skipRuntimeOrInactive;
    tsBaseSkipAp = (uint32_t)bChannelDiag.skipApState;
    tsBaseSkipHandsOn = (uint32_t)bChannelDiag.skipHandsOn;
    tsBaseSkipDas = (uint32_t)bChannelDiag.skipDasState;
    tsBaseNoDasEcho = (uint32_t)bChannelDiag.nagFiredNoDas;
    tsBaseUserMark = (uint32_t)userMarkerCount;
    tsBaseAFrames = (uint32_t)aChannelDiag.framesReceivedTotal;
    tsBaseATxOk = (uint32_t)aChannelDiag.aTxOk;
    tsBaseATxFail = (uint32_t)aChannelDiag.aTxFail;
    tsBaseAMerrf = (uint32_t)aChannelDiag.aMerrfCount;
    tsBaseARxOvr = (uint32_t)aChannelDiag.aRxOvrCount;
    tsBaseAEflgEvents = (uint32_t)aChannelDiag.mcpEflgEventCount;
    tsBaseALoopGapOver2ms = (uint32_t)aChannelDiag.loopGapOver2msCount;
    tsBaseSummonTxOk = (uint32_t)summonGateDiag.txOk;
    tsBaseSummonTxFail = (uint32_t)summonGateDiag.txFail;
    tsBaseSummonBlocked = (uint32_t)summonGateDiag.blocked;
    tsBaseTsllcTxOk = (uint32_t)aChannelDiag.tsllcTxOk;
    tsBaseTsllcTxFail = (uint32_t)aChannelDiag.tsllcTxFail;
    tsPrevEcho = (uint32_t)bChannelDiag.echoCount;
    tsPrevF880 = (uint32_t)bChannelDiag.frames880;
    tsPrevF921 = (uint32_t)bChannelDiag.frames921;
    tsPrevF923 = (uint32_t)bChannelDiag.frames923;
    tsPrevModeBInject = (uint32_t)bChannelDiag.modeBInjectCount;
    tsPrevEchoDrop = (uint32_t)bChannelDiag.echoDroppedLate;
    tsPrevSkipRuntime = (uint32_t)bChannelDiag.skipRuntimeOrInactive;
    tsPrevSkipAp = (uint32_t)bChannelDiag.skipApState;
    tsPrevSkipHandsOn = (uint32_t)bChannelDiag.skipHandsOn;
    tsPrevSkipDas = (uint32_t)bChannelDiag.skipDasState;
    tsPrevNoDasEcho = (uint32_t)bChannelDiag.nagFiredNoDas;
    tsPrevUserMark = (uint32_t)userMarkerCount;
    tsPrevAFrames = (uint32_t)aChannelDiag.framesReceivedTotal;
    tsPrevATxOk = (uint32_t)aChannelDiag.aTxOk;
    tsPrevATxFail = (uint32_t)aChannelDiag.aTxFail;
    tsPrevAMerrf = (uint32_t)aChannelDiag.aMerrfCount;
    tsPrevARxOvr = (uint32_t)aChannelDiag.aRxOvrCount;
    tsPrevAEflgEvents = (uint32_t)aChannelDiag.mcpEflgEventCount;
    tsPrevALoopGapOver2ms = (uint32_t)aChannelDiag.loopGapOver2msCount;
    tsPrevSummonTxOk = (uint32_t)summonGateDiag.txOk;
    tsPrevSummonTxFail = (uint32_t)summonGateDiag.txFail;
    tsPrevSummonBlocked = (uint32_t)summonGateDiag.blocked;
    tsPrevTsllcTxOk = (uint32_t)aChannelDiag.tsllcTxOk;
    tsPrevTsllcTxFail = (uint32_t)aChannelDiag.tsllcTxFail;
    tsPrevSummonTxCompleted = (uint32_t)aChannelDiag.aTxCompletedSummon;
    tsPrevSummonTxArbitrationLost = (uint32_t)aChannelDiag.aTxArbitrationLostSummon;
    tsPrevSummonTxAborted = (uint32_t)aChannelDiag.aTxAbortedSummon;
    tsPrevTsllcTxCompleted = (uint32_t)aChannelDiag.aTxCompletedTsllc;
    tsPrevTsllcTxArbitrationLost = (uint32_t)aChannelDiag.aTxArbitrationLostTsllc;
    tsPrevTsllcTxAborted = (uint32_t)aChannelDiag.aTxAbortedTsllc;
    tsFeatureActivitySeen = false;
    tsFeatureActivityLast = 0;
    portEXIT_CRITICAL(&tsMux);
    // USER_MARK는 부팅 세션 전체의 분석 기준점이다. 로그 초기화나 새 기록
    // 시작으로 지우지 않으며 RAM 상태이므로 보드 재부팅 때만 초기화된다.
    eventLogReset();
    busOffLog.clear();
}

inline void timeseriesSnapshot(size_t& count, size_t& head, uint32_t& resetMs, uint32_t& recStartMs, bool& recording) {
    portENTER_CRITICAL(&tsMux);
    count = tsCount;
    head = tsHead;
    resetMs = tsResetMs;
    recStartMs = tsRecStartMs;
    recording = tsRecording;
    portEXIT_CRITICAL(&tsMux);
}

inline void timeseriesCopyAt(size_t idx, TsSample& out) {
    portENTER_CRITICAL(&tsMux);
    out = tsBuf[idx % TS_CAP];
    portEXIT_CRITICAL(&tsMux);
}

inline esp_err_t timeseriesCsvHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/csv; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"can_timeseries_ab.csv\"");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    size_t n = 0;
    size_t head = 0;
    uint32_t resetMs = 0;
    uint32_t recStartMs = 0;
    bool recording = false;
    timeseriesSnapshot(n, head, resetMs, recStartMs, recording);
    (void)resetMs;
    (void)recStartMs;
    (void)recording;
    httpd_resp_sendstr_chunk(req, "\xEF\xBB\xBF");
    const char* hdr =
        "schema_version,firmware_version,firmware_build_id,wall_time,uptime_ms,capture_mode,"
        "b_busoff,b_tec,b_rec,b_arb_lost,b_bus_error,b_tx_fail,b_echo,b_frames_880,b_frames_921,b_frames_923,"
        "b_hands_on,b_das_state,b_das_state_name,b_das_state_group,b_das_warn_level,b_das_warning,"
        "b_nag_mode,b_nag_mode_default,b_das_source,b_echo_drop,b_skip_off,b_skip_ap,b_skip_hands_on,b_skip_das,b_no_das,"
        "system_user_mark,b_d880,b_d921,b_d923,b_d_echo,b_d_drop,b_d_skip_off,b_d_skip_ap,b_d_skip_hands_on,b_d_skip_das,"
        "b_d_no_das,system_d_user_mark,b_last_decision,b_interval_decision,b_ap_state,b_nag_phase,"
        "b_real_torque_nm,b_nag_inject,b_nag_last_nm,b_age_880_ms,b_age_das_ms,b_age_echo_ms,b_d_nag_inject,"
        "a_frames,a_frame_hz,a_eflg,a_eflg_state,a_eflg_peak,a_tec,a_rec,a_tec_peak,a_rec_peak,"
        "a_tx_queued,a_tx_busy,a_tx_hard_error,a_tx_completed,a_tx_arbitration_lost,a_tx_aborted,a_merrf,"
        "a_rx_overrun,a_rx0_overrun,a_rx1_overrun,a_rx_buffer0_frames,a_rx_buffer1_frames,"
        "a_rx_drain_frames,a_rx_drain_calls,a_rx_queue_high_water,a_rx_queue_drops,"
        "a_eflg_events,a_frame_age_ms,a_loop_age_ms,a_guard_active,a_guard_reason,a_guard_remaining_ms,"
        "a_wake_count,a_wake_to_summon_tx_ms,a_wake_awaiting_tx,a_d_frames,a_d_tx_ok,a_d_tx_fail,a_d_merrf,a_d_rx_overrun,a_d_eflg_events,"
        "a_driver_ok,a_tx_enabled,a_summon_enabled,a_tsllc_enabled,a_one_shot_enabled,a_tx_guard_enabled,a_spi_mhz,a_spi_target_mhz,"
        "b_driver_state,b_nag_enabled,a_loop_gap_last_us,a_loop_gap_peak_us,a_loop_gap_over_250us,a_loop_gap_over_500us,"
        "a_loop_gap_over_1ms,a_loop_gap_over_2ms,a_d_loop_gap_over_2ms,a_last_overrun_phase,"
        "a_summon_gate_open,a_summon_condition_limit,a_summon_ap_state,a_summon_ap_active,a_summon_parked,a_summoning,"
        "a_summon_ap_stable_ms,a_summon_gate_reason,"
        "a_summon_inject_ready,a_summon_tx_ok,a_summon_tx_fail,a_summon_blocked,"
        "a_d_summon_tx_ok,a_d_summon_tx_fail,a_d_summon_blocked,a_tsllc_inject_ready,a_tsllc_tx_ok,a_tsllc_tx_fail,"
        "a_d_tsllc_tx_ok,a_d_tsllc_tx_fail,b_nag_ap_only,b_nag_ap_active,b_nag_injecting,b_nag_tx_ok,b_d_nag_tx_ok,"
        "a_summon_tx_completed,a_summon_tx_arbitration_lost,a_summon_tx_aborted,a_summon_tx_error,"
        "a_tsllc_tx_completed,a_tsllc_tx_arbitration_lost,a_tsllc_tx_aborted,a_tsllc_tx_error\r\n";
    httpd_resp_sendstr_chunk(req, hdr);
    char line[1792];
    char wallTime[40];
    size_t start = (n < TS_CAP) ? 0 : head;  // oldest first
    for (size_t i = 0; i < n; ++i) {
        TsSample s;
        timeseriesCopyAt(start + i, s);
        timeseriesFormatTime(s.t_ms, wallTime, sizeof(wallTime));
        int used = snprintf(line, sizeof(line),
            "4,%s,%s,%s,%u,%s,"
            "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
            "%u,%u,%s,%s,%u,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,"
            "%.2f,%u,%.2f,"
            "%u,%u,%u,%u",
            FIRMWARE_VERSION, FIRMWARE_BUILD_ID, wallTime, (unsigned)s.t_ms, s.captureMode ? "MANUAL" : "AUTO",
            (unsigned)s.busoff, (unsigned)s.tec, (unsigned)s.rec,
            (unsigned)s.arbLost, (unsigned)s.busErr, (unsigned)s.txFail,
            (unsigned)s.echoCnt, (unsigned)s.f880, (unsigned)s.f921, (unsigned)s.f923,
            (unsigned)s.handsOn, (unsigned)s.dasState,
            dasHandsOnStateName(s.dasState), dasHandsOnStateGroup(s.dasState),
            (unsigned)dasHandsOnWarningLevel(s.dasState), dasHandsOnStateIsWarning(s.dasState) ? 1U : 0U,
            (unsigned)s.nagMode, (unsigned)s.nagModeDefault, (unsigned)s.dasSourceId,
            (unsigned)s.echoDrop, (unsigned)s.skipRuntime,
            (unsigned)s.skipAp, (unsigned)s.skipHandsOn, (unsigned)s.skipDas,
            (unsigned)s.noDasEcho, (unsigned)s.userMark,
            (unsigned)s.d880, (unsigned)s.d921, (unsigned)s.d923,
            (unsigned)s.dEcho, (unsigned)s.dDrop, (unsigned)s.dSkipRuntime,
            (unsigned)s.dSkipAp, (unsigned)s.dSkipHandsOn, (unsigned)s.dSkipDas,
            (unsigned)s.dNoDasEcho, (unsigned)s.dUserMark, (unsigned)s.lastDecision,
            (unsigned)s.intervalDecision,
            (unsigned)s.apState, (unsigned)s.modeBPhase,
            (double)s.realTorqueNm,
            (unsigned)s.modeBInject, (double)s.modeBLastNm,
            (unsigned)s.age880Ms, (unsigned)s.ageDasMs,
            (unsigned)s.ageEchoMs, (unsigned)s.dModeBInject);
        if (used < 0 || (size_t)used >= sizeof(line)) continue;
        snprintf(line + used, sizeof(line) - (size_t)used,
            ",%u,%.1f,%u,%s,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%s,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u,%u,%s,"
            "%u,%u,%u,%u,%u,%u,%u,%s,"
            "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u\r\n",
            (unsigned)s.aFrames, (double)s.aFrameHz,
            (unsigned)s.aEflg, aMcpEflgStateName(s.aEflg), (unsigned)s.aEflgPeak,
            (unsigned)s.aTec, (unsigned)s.aRec, (unsigned)s.aTecPeak, (unsigned)s.aRecPeak,
            (unsigned)s.aTxOk, (unsigned)s.aTxBusy, (unsigned)s.aTxFail,
            (unsigned)s.aTxCompleted, (unsigned)s.aTxArbitrationLost,
            (unsigned)s.aTxAborted, (unsigned)s.aMerrf,
            (unsigned)s.aRxOvr, (unsigned)s.aRx0Ovr, (unsigned)s.aRx1Ovr,
            (unsigned)s.aRxBuffer0Frames, (unsigned)s.aRxBuffer1Frames,
            (unsigned)s.aRxDrainFrames, (unsigned)s.aRxDrainCalls,
            (unsigned)s.aRxQueueHighWater, (unsigned)s.aRxQueueDrops,
            (unsigned)s.aEflgEvents,
            (unsigned)s.aFrameAgeMs, (unsigned)s.aLoopAgeMs,
            (unsigned)s.aGuardActive, aTxGuardReasonName(s.aGuardReason),
            (unsigned)s.aGuardRemainingMs,
            (unsigned)s.aWakeCount, (unsigned)s.aWakeToSummonTxMs, (unsigned)s.aWakeAwaitingTx,
            (unsigned)s.dAFrames, (unsigned)s.dATxOk, (unsigned)s.dATxFail,
            (unsigned)s.dAMerrf, (unsigned)s.dARxOvr, (unsigned)s.dAEflgEvents,
            (unsigned)s.aDriverOk, (unsigned)s.aTxEnabled,
            (unsigned)s.summonEnabled, (unsigned)s.tsllcEnabled,
            (unsigned)s.aOneShotEnabled, (unsigned)s.aTxGuardEnabled,
            (unsigned)s.aSpiMhz, (unsigned)s.aSpiTargetMhz,
            (unsigned)s.bDriverState, (unsigned)s.nagEnabled,
            (unsigned)s.aLoopGapLastUs, (unsigned)s.aLoopGapPeakUs,
            (unsigned)s.aLoopGapOver250us, (unsigned)s.aLoopGapOver500us,
            (unsigned)s.aLoopGapOver1ms,
            (unsigned)s.aLoopGapOver2ms, (unsigned)s.dALoopGapOver2ms,
            aCanPhaseName(s.aLastOverrunPhase),
            (unsigned)s.summonGateOpen, (unsigned)s.summonConditionLimit,
            (unsigned)s.summonApState, (unsigned)s.summonApActive,
            (unsigned)s.summonParked, (unsigned)s.summoning,
            (unsigned)s.summonApStableMs,
            summonGateReasonNameFromCode(s.summonGateReason),
            (unsigned)s.summonInjectReady,
            (unsigned)s.summonTxOk, (unsigned)s.summonTxFail,
            (unsigned)s.summonBlocked, (unsigned)s.dSummonTxOk,
            (unsigned)s.dSummonTxFail, (unsigned)s.dSummonBlocked,
            (unsigned)s.tsllcInjectReady, (unsigned)s.tsllcTxOk,
            (unsigned)s.tsllcTxFail, (unsigned)s.dTsllcTxOk,
            (unsigned)s.dTsllcTxFail, (unsigned)s.nagApOnly,
            (unsigned)s.nagApActive, (unsigned)s.nagInjecting,
            (unsigned)s.modeBInject, (unsigned)s.dModeBInject,
            (unsigned)s.summonTxCompleted,
            (unsigned)s.summonTxArbitrationLost,
            (unsigned)s.summonTxAborted, (unsigned)s.summonTxError,
            (unsigned)s.tsllcTxCompleted,
            (unsigned)s.tsllcTxArbitrationLost,
            (unsigned)s.tsllcTxAborted, (unsigned)s.tsllcTxError);
        if (httpd_resp_sendstr_chunk(req, line) != ESP_OK) return ESP_FAIL;
    }
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

// POST /api/timeseries/reset
inline esp_err_t timeseriesResetHandler(httpd_req_t* req) {
    timeseriesReset();
    eventLogPush(EV_CAPTURE_RESET,
                 (uint16_t)bChannelDiag.twaiTxErrNow,
                 (uint16_t)bChannelDiag.twaiRxErrNow, 0);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// POST /api/timeseries/rec  (body: {"start":true|false})
inline esp_err_t timeseriesRecHandler(httpd_req_t* req) {
    char buf[32] = {0};
    int len = req->content_len < (int)sizeof(buf)-1 ? req->content_len : (int)sizeof(buf)-1;
    if (len > 0) httpd_req_recv(req, buf, len);
    bool start = (strstr(buf, "true") != nullptr);
    if (start) {
        timeseriesReset(true);      // 깨끗한 구간 + 원자적으로 수동 기록 시작
        eventLogPush(EV_CAPTURE_START,
                     (uint16_t)bChannelDiag.twaiTxErrNow,
                     (uint16_t)bChannelDiag.twaiRxErrNow, 0);
    } else {
        eventLogPush(EV_CAPTURE_STOP,
                     (uint16_t)bChannelDiag.twaiTxErrNow,
                     (uint16_t)bChannelDiag.twaiRxErrNow, 0);
        portENTER_CRITICAL(&tsMux);
        tsRecording = false;        // 수동 기록 버퍼 고정
        portEXIT_CRITICAL(&tsMux);
    }
    httpd_resp_set_type(req, "application/json");
    char out[80];
    size_t count = 0, head = 0;
    uint32_t resetMs = 0, recStartMs = 0;
    bool recording = false;
    timeseriesSnapshot(count, head, resetMs, recStartMs, recording);
    (void)count; (void)head; (void)resetMs;
    snprintf(out, sizeof(out), "{\"ok\":true,\"rec\":%s,\"start_ms\":%u}",
        recording?"true":"false", (unsigned)recStartMs);
    httpd_resp_sendstr(req, out);
    return ESP_OK;
}

// GET /api/timeseries/status  → REC 상태 + 경과 시간
inline esp_err_t timeseriesStatusHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "application/json");
    char out[128];
    size_t count = 0, head = 0;
    uint32_t resetMs = 0, recStartMs = 0;
    bool recording = false;
    timeseriesSnapshot(count, head, resetMs, recStartMs, recording);
    (void)head; (void)resetMs;
    uint32_t elapsed = (recStartMs && recording) ? (millis() - recStartMs) : 0;
    snprintf(out, sizeof(out),
        "{\"rec\":%s,\"start_ms\":%u,\"elapsed_ms\":%u,\"samples\":%u}",
        recording?"true":"false", (unsigned)recStartMs,
        (unsigned)elapsed, (unsigned)count);
    httpd_resp_sendstr(req, out);
    return ESP_OK;
}
#endif  // NATIVE_BUILD
