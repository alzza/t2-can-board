// CAN 프레임 헬퍼 함수 및 빌드 플래그별 기본값/런타임 변수 정의
#pragma once

#include <cstddef>
#include <cstring>
#include "can_frame_types.h"
#include "shared_types.h"

inline constexpr uint8_t kTrackModeRequestOn = 0x01;

#if defined(BYPASS_TLSSC_REQUIREMENT)
inline constexpr bool kBypassTlsscRequirementDefaultEnabled = true;
inline constexpr bool kBypassTlsscRequirementBuildEnabled = true;
#else
inline constexpr bool kBypassTlsscRequirementDefaultEnabled = false;
inline constexpr bool kBypassTlsscRequirementBuildEnabled = false;
#endif

#if defined(ISA_SPEED_CHIME_SUPPRESS)
inline constexpr bool kIsaSpeedChimeSuppressDefaultEnabled = true;
inline constexpr bool kIsaSpeedChimeSuppressBuildEnabled = true;
#else
inline constexpr bool kIsaSpeedChimeSuppressDefaultEnabled = false;
inline constexpr bool kIsaSpeedChimeSuppressBuildEnabled = false;
#endif

#if defined(EMERGENCY_VEHICLE_DETECTION)
inline constexpr bool kEmergencyVehicleDetectionDefaultEnabled = true;
inline constexpr bool kEmergencyVehicleDetectionBuildEnabled = true;
#else
inline constexpr bool kEmergencyVehicleDetectionDefaultEnabled = false;
inline constexpr bool kEmergencyVehicleDetectionBuildEnabled = false;
#endif

#if defined(SUMMON_UNLOCK)
inline constexpr bool kSummonUnlockDefaultEnabled = true;   // 검증된 INO와 동일한 fresh-NVS 기본값
inline constexpr bool kSummonUnlockBuildEnabled = true;
#else
inline constexpr bool kSummonUnlockDefaultEnabled = false;
inline constexpr bool kSummonUnlockBuildEnabled = false;
#endif

#if defined(SUMMON_UNLOCK)
inline constexpr bool kTsllcDefaultEnabled = false;  // 기본값 OFF (스톱사인/신호등 자동 제어)
inline constexpr bool kTsllcBuildEnabled = true;
#else
inline constexpr bool kTsllcDefaultEnabled = false;
inline constexpr bool kTsllcBuildEnabled = false;
#endif

#if defined(NAG_KILLER)
inline constexpr bool kNagKillerDefaultEnabled = true;
inline constexpr bool kNagKillerBuildEnabled = true;
#else
inline constexpr bool kNagKillerDefaultEnabled = false;
inline constexpr bool kNagKillerBuildEnabled = false;
#endif

// 👇 [추가] Nag Killer B채널 디버그 플래그
#if defined(DEBUG_NAG_KILLER)
inline constexpr bool kDebugNagKillerEnabled = true;
#else
inline constexpr bool kDebugNagKillerEnabled = false;
#endif

#if !defined(T2CAN_SPI_FREQ_HZ)
#define T2CAN_SPI_FREQ_HZ 10000000
#endif

#if !defined(T2CAN_MCP2515_ONE_SHOT_DEFAULT)
#define T2CAN_MCP2515_ONE_SHOT_DEFAULT 1
#endif

#if !defined(T2CAN_A_TX_GUARD_DEFAULT)
#define T2CAN_A_TX_GUARD_DEFAULT 1
#endif

inline constexpr uint32_t kAMcpDefaultSpiFreqHz = T2CAN_SPI_FREQ_HZ;
inline constexpr bool kAMcpOneShotDefaultEnabled = (T2CAN_MCP2515_ONE_SHOT_DEFAULT != 0);
inline constexpr bool kATxGuardDefaultEnabled = (T2CAN_A_TX_GUARD_DEFAULT != 0);

inline Shared<bool> bypassTlsscRequirementRuntime{kBypassTlsscRequirementDefaultEnabled};
inline Shared<bool> isaSpeedChimeSuppressRuntime{kIsaSpeedChimeSuppressDefaultEnabled};
inline Shared<bool> emergencyVehicleDetectionRuntime{kEmergencyVehicleDetectionDefaultEnabled};
inline Shared<bool> summonUnlockRuntime{kSummonUnlockDefaultEnabled};
inline Shared<bool> nagKillerRuntime{kNagKillerDefaultEnabled};
inline Shared<bool> tsllcRuntime{kTsllcDefaultEnabled};         // TSLLC 런타임 토글 (스톱사인/초록불 제어)
inline Shared<bool> aChannelTxRuntime{true};     // A채널 1021 수정 송신 마스터 토글
inline Shared<uint32_t> aMcpSpiFreqHz{kAMcpDefaultSpiFreqHz};
inline Shared<uint32_t> aMcpRequestedSpiFreqHz{kAMcpDefaultSpiFreqHz};
inline Shared<bool> aMcpOneShotRuntime{kAMcpOneShotDefaultEnabled};
inline Shared<bool> aTxGuardRuntime{kATxGuardDefaultEnabled};

// Validated SummonUnlock gate state (HW3): inject only while parked or summoning.
inline constexpr uint32_t kSummonParkedTimeoutMs = 5000;

struct SummonGateDiagnostics {
    Shared<bool> apActive{false};
    Shared<bool> parked{true};
    Shared<bool> summoning{false};
    Shared<bool> acaActive{false};
    Shared<bool> sprSeen{false};
    Shared<uint32_t> last280Ms{0};
    Shared<uint32_t> frames280{0};
    Shared<uint32_t> frames390{0};
    Shared<uint32_t> frames921{0};
    Shared<uint32_t> frames1016{0};
    Shared<uint32_t> mux1Received{0};
    Shared<uint32_t> txOk{0};
    Shared<uint32_t> txFail{0};
    Shared<uint32_t> blocked{0};
};

inline SummonGateDiagnostics summonGateDiag;

inline int8_t summonGearState(uint8_t gear) {
    if (gear == 1) return 1;
    if (gear == 2 || gear == 3 || gear == 4) return 0;
    return -1;
}

inline bool summonGateOpen() {
    return (bool)summonGateDiag.parked || (bool)summonGateDiag.summoning;
}

inline void summonRecompute() {
    summonGateDiag.summoning = (bool)summonGateDiag.acaActive && (bool)summonGateDiag.sprSeen;
}

inline void summonClearOnParkIfAcaInactive(uint8_t gear) {
    if (gear == 1 && !(bool)summonGateDiag.acaActive) {
        summonGateDiag.summoning = false;
        summonGateDiag.sprSeen = false;
    }
}

inline void summonHandle280(const CanFrame &frame, uint32_t nowMs) {
    if (frame.dlc < 7) return;
    summonGateDiag.frames280 = (uint32_t)summonGateDiag.frames280 + 1;
    summonGateDiag.last280Ms = nowMs;
    const uint8_t gear = (frame.data[2] >> 5) & 0x07;
    const int8_t gearState = summonGearState(gear);
    if (gearState == 1) summonGateDiag.parked = true;
    if (gearState == 0) summonGateDiag.parked = false;

    const bool aca = (frame.data[6] & 0x04U) != 0;
    if ((bool)summonGateDiag.acaActive && !aca) summonGateDiag.sprSeen = false;
    summonGateDiag.acaActive = aca;
    summonRecompute();
    summonClearOnParkIfAcaInactive(gear);
}

inline void summonHandle390(const CanFrame &frame, uint32_t nowMs) {
    if (frame.dlc < 8) return;
    summonGateDiag.frames390 = (uint32_t)summonGateDiag.frames390 + 1;
    const uint8_t gear = (frame.data[2] >> 5) & 0x07;
    const int8_t gearState = summonGearState(gear);
    if (gearState < 0) return;
    const uint32_t last280Ms = (uint32_t)summonGateDiag.last280Ms;
    if (last280Ms == 0 || nowMs - last280Ms > kSummonParkedTimeoutMs) {
        summonGateDiag.parked = (gearState == 1);
        summonClearOnParkIfAcaInactive(gear);
    }
}

inline void summonHandle921(const CanFrame &frame) {
    if (frame.dlc < 1) return;
    summonGateDiag.frames921 = (uint32_t)summonGateDiag.frames921 + 1;
    const uint8_t status = frame.data[0] & 0x07;
    summonGateDiag.apActive = status == 2 || status == 3 || status == 4;
}

inline void summonHandle1016(const CanFrame &frame) {
    if (frame.dlc < 4) return;
    summonGateDiag.frames1016 = (uint32_t)summonGateDiag.frames1016 + 1;
    const uint8_t spr = (frame.data[3] >> 4) & 0x0F;
    if (spr != 0) summonGateDiag.sprSeen = true;
    summonRecompute();
}

inline void summonGateMaintain(uint32_t nowMs) {
    const uint32_t last280Ms = (uint32_t)summonGateDiag.last280Ms;
    if (last280Ms > 0 && nowMs - last280Ms > kSummonParkedTimeoutMs) {
        summonGateDiag.parked = true;
    }
}

inline constexpr uint8_t kSignalObserverMaxSignals = 10;
inline constexpr uint8_t kSignalObserverMaxAFilterIds = 6;
inline constexpr uint8_t kSignalObserverChannelA = 0x01;
inline constexpr uint8_t kSignalObserverChannelB = 0x02;
inline constexpr uint8_t kSignalObserverChannelBoth = kSignalObserverChannelA | kSignalObserverChannelB;
inline constexpr uint8_t kSignalObserverByteOrderLittle = 0;
inline constexpr uint8_t kSignalObserverByteOrderBig = 1;
inline constexpr uint8_t kSignalObserverNameLen = 40;
inline constexpr size_t kSignalObserverEventCap = 256;

struct SignalObserverDef {
    bool enabled;
    uint8_t channelMask;
    uint8_t byteOrder;
    uint16_t frameId;
    uint8_t startBit;
    uint8_t length;
    uint32_t idleRaw;
    char name[kSignalObserverNameLen];
    uint8_t muxStartBit;
    uint8_t muxLength;
    uint32_t muxValue;
};

struct SignalObserverState {
    bool seen;
    bool active;
    uint32_t frameCount;
    uint32_t activeFrameCount;
    uint32_t changeCount;
    uint32_t burstCount;
    uint32_t currentRunFrames;
    uint32_t lastRunFrames;
    uint32_t maxRunFrames;
    uint32_t firstSeenMs;
    uint32_t lastSeenMs;
    uint32_t lastChangeMs;
    uint32_t lastRaw;
    uint32_t prevRaw;
};

enum SignalObserverEventType : uint8_t {
    SO_EVT_CAPTURE_START = 0,
    SO_EVT_CAPTURE_STOP = 1,
    SO_EVT_RESET = 2,
    SO_EVT_CONFIG_LOADED = 3,
    SO_EVT_FIRST_SEEN = 4,
    SO_EVT_RAW_CHANGE = 5,
    SO_EVT_ACTIVE_START = 6,
    SO_EVT_ACTIVE_END = 7,
};

struct SignalObserverEvent {
    uint32_t tMs;
    uint8_t type;
    uint8_t signalIndex;
    uint8_t channelMask;
    uint8_t byteOrder;
    uint8_t active;
    uint16_t frameId;
    uint8_t startBit;
    uint8_t length;
    uint32_t prevRaw;
    uint32_t raw;
    uint32_t frameCount;
    uint32_t activeFrameCount;
    uint32_t changeCount;
    uint32_t burstCount;
    uint32_t currentRunFrames;
    uint32_t lastRunFrames;
    uint32_t maxRunFrames;
};

inline Shared<bool> signalObserverRuntime{false};  // 부팅 시 정지 상태; 시작 버튼 누를 때까지 카운트 없음
inline Shared<uint8_t> signalObserverCount{0};      // 기본 프리셋 없음; 사용자가 올린 수동 관찰 설정만 사용
inline SignalObserverDef signalObserverDefs[kSignalObserverMaxSignals] = {};
inline SignalObserverState signalObserverStates[kSignalObserverMaxSignals] = {};
inline SignalObserverEvent signalObserverEvents[kSignalObserverEventCap] = {};
inline volatile size_t signalObserverEventHead = 0;
inline volatile size_t signalObserverEventCount = 0;
inline volatile uint32_t signalObserverEventOverwritten = 0;

inline const char* signalObserverEventTypeName(uint8_t type) {
    switch (type) {
    case SO_EVT_CAPTURE_START: return "CAPTURE_START";
    case SO_EVT_CAPTURE_STOP: return "CAPTURE_STOP";
    case SO_EVT_RESET: return "RESET";
    case SO_EVT_CONFIG_LOADED: return "CONFIG_LOADED";
    case SO_EVT_FIRST_SEEN: return "FIRST_SEEN";
    case SO_EVT_RAW_CHANGE: return "RAW_CHANGE";
    case SO_EVT_ACTIVE_START: return "ACTIVE_START";
    case SO_EVT_ACTIVE_END: return "ACTIVE_END";
    default: return "UNKNOWN";
    }
}

inline void signalObserverResetEvents() {
    std::memset(signalObserverEvents, 0, sizeof(signalObserverEvents));
    signalObserverEventHead = 0;
    signalObserverEventCount = 0;
    signalObserverEventOverwritten = 0;
}

inline void signalObserverEventPush(uint8_t type, uint8_t signalIndex, const SignalObserverDef &def,
                                    const SignalObserverState &st, uint32_t nowMs,
                                    uint32_t prevRaw, uint32_t raw, bool active) {
    SignalObserverEvent &ev = signalObserverEvents[signalObserverEventHead];
    ev.tMs = nowMs;
    ev.type = type;
    ev.signalIndex = signalIndex;
    ev.channelMask = def.channelMask;
    ev.byteOrder = def.byteOrder;
    ev.active = active ? 1 : 0;
    ev.frameId = def.frameId;
    ev.startBit = def.startBit;
    ev.length = def.length;
    ev.prevRaw = prevRaw;
    ev.raw = raw;
    ev.frameCount = st.frameCount;
    ev.activeFrameCount = st.activeFrameCount;
    ev.changeCount = st.changeCount;
    ev.burstCount = st.burstCount;
    ev.currentRunFrames = st.currentRunFrames;
    ev.lastRunFrames = st.lastRunFrames;
    ev.maxRunFrames = st.maxRunFrames;

    signalObserverEventHead = (signalObserverEventHead + 1) % kSignalObserverEventCap;
    if (signalObserverEventCount < kSignalObserverEventCap) {
        ++signalObserverEventCount;
    } else {
        ++signalObserverEventOverwritten;
    }
}

inline void signalObserverEventPushSystem(uint8_t type, uint32_t nowMs) {
    SignalObserverDef def = {};
    SignalObserverState st = {};
    signalObserverEventPush(type, 0xFF, def, st, nowMs, 0, 0, false);
}

inline void signalObserverEventSnapshot(size_t &count, size_t &head, uint32_t &overwritten) {
    count = signalObserverEventCount;
    head = signalObserverEventHead;
    overwritten = signalObserverEventOverwritten;
}

inline void signalObserverEventCopyAt(size_t idx, SignalObserverEvent &out) {
    out = signalObserverEvents[idx % kSignalObserverEventCap];
}

inline bool signalObserverFrameIdIsBaseA(uint16_t frameId) {
    return frameId == 659 || frameId == 1016 || frameId == 1021;
}

inline void signalObserverResetStats() {
    std::memset(signalObserverStates, 0, sizeof(signalObserverStates));
}

inline const char* signalObserverChannelName(uint8_t channelMask) {
    switch (channelMask & kSignalObserverChannelBoth) {
    case kSignalObserverChannelA: return "A";
    case kSignalObserverChannelB: return "B";
    case kSignalObserverChannelBoth: return "A+B";
    default: return "OFF";
    }
}

inline const char* signalObserverByteOrderName(uint8_t byteOrder) {
    return byteOrder == kSignalObserverByteOrderBig ? "big" : "little";
}

inline uint8_t signalObserverNextBigEndianBit(uint8_t bitPosition) {
    return (bitPosition % 8U == 0U) ? static_cast<uint8_t>(bitPosition + 15U) : static_cast<uint8_t>(bitPosition - 1U);
}

inline bool signalObserverExtractRawBits(const CanFrame &frame, uint8_t startBit, uint8_t length,
                                         uint8_t byteOrder, uint32_t &rawOut) {
    if (length == 0 || length > 32) return false;
    if (startBit > 63) return false;
    uint8_t dlc = frame.dlc > 8 ? 8 : frame.dlc;
    if (byteOrder == kSignalObserverByteOrderBig) {
        uint32_t raw = 0;
        uint8_t bitPosition = startBit;
        for (uint8_t bitIndex = 0; bitIndex < length; ++bitIndex) {
            if (bitPosition >= (uint8_t)(dlc * 8U)) return false;
            raw = (raw << 1U) | ((frame.data[bitPosition / 8U] >> (bitPosition % 8U)) & 0x01U);
            if (bitIndex + 1U < length) bitPosition = signalObserverNextBigEndianBit(bitPosition);
        }
        rawOut = raw;
        return true;
    }

    if ((uint16_t)startBit + (uint16_t)length > 64) return false;
    uint8_t neededBytes = static_cast<uint8_t>(((uint16_t)startBit + (uint16_t)length + 7U) / 8U);
    if (dlc < neededBytes) return false;

    uint64_t payload = 0;
    for (uint8_t byteIndex = 0; byteIndex < dlc; ++byteIndex) {
        payload |= (static_cast<uint64_t>(frame.data[byteIndex]) << (8U * byteIndex));
    }
    uint64_t mask = (length >= 32) ? 0xFFFFFFFFULL : ((1ULL << length) - 1ULL);
    rawOut = static_cast<uint32_t>((payload >> startBit) & mask);
    return true;
}

inline bool signalObserverExtractRaw(const CanFrame &frame, const SignalObserverDef &def, uint32_t &rawOut) {
    if (!def.enabled) return false;
    if (def.muxLength > 0) {
        uint32_t muxRaw = 0;
        if (!signalObserverExtractRawBits(frame, def.muxStartBit, def.muxLength, def.byteOrder, muxRaw)) {
            return false;
        }
        if (muxRaw != def.muxValue) return false;
    }
    return signalObserverExtractRawBits(frame, def.startBit, def.length, def.byteOrder, rawOut);
}

inline void signalObserverObserveFrame(uint8_t channelMask, const CanFrame &frame, uint32_t nowMs) {
    if (!(bool)signalObserverRuntime) return;
    uint8_t count = (uint8_t)signalObserverCount;
    if (count > kSignalObserverMaxSignals) count = kSignalObserverMaxSignals;

    for (uint8_t i = 0; i < count; ++i) {
        const SignalObserverDef &def = signalObserverDefs[i];
        if (!def.enabled || (def.channelMask & channelMask) == 0 || def.frameId != frame.id) continue;

        uint32_t raw = 0;
        if (!signalObserverExtractRaw(frame, def, raw)) continue;

        SignalObserverState &st = signalObserverStates[i];
        bool wasSeen = st.seen;
        bool wasActive = st.active;
        uint32_t prevRaw = st.lastRaw;
        bool rawChanged = st.seen && raw != st.lastRaw;
        if (!st.seen) {
            st.seen = true;
            st.firstSeenMs = nowMs;
        } else if (rawChanged) {
            st.prevRaw = st.lastRaw;
            st.changeCount++;
            st.lastChangeMs = nowMs;
        }

        bool active = raw != def.idleRaw;
        st.frameCount++;
        if (active) {
            st.activeFrameCount++;
            if (!st.active) {
                st.burstCount++;
                st.currentRunFrames = 1;
            } else {
                st.currentRunFrames++;
            }
            if (st.currentRunFrames > st.maxRunFrames) st.maxRunFrames = st.currentRunFrames;
        } else if (st.active) {
            st.lastRunFrames = st.currentRunFrames;
            if (st.currentRunFrames > st.maxRunFrames) st.maxRunFrames = st.currentRunFrames;
            st.currentRunFrames = 0;
        }
        st.active = active;
        st.lastRaw = raw;
        st.lastSeenMs = nowMs;

        if (!wasSeen) {
            signalObserverEventPush(SO_EVT_FIRST_SEEN, i, def, st, nowMs, prevRaw, raw, active);
        } else if (rawChanged) {
            signalObserverEventPush(SO_EVT_RAW_CHANGE, i, def, st, nowMs, prevRaw, raw, active);
        }
        if (!wasActive && active) {
            signalObserverEventPush(SO_EVT_ACTIVE_START, i, def, st, nowMs, prevRaw, raw, active);
        } else if (wasActive && !active) {
            signalObserverEventPush(SO_EVT_ACTIVE_END, i, def, st, nowMs, prevRaw, raw, active);
        }
    }
}

inline bool signalObserverIdAlreadyPresent(const uint32_t *ids, uint8_t count, uint32_t frameId) {
    for (uint8_t i = 0; i < count; ++i) {
        if (ids[i] == frameId) return true;
    }
    return false;
}

inline uint8_t signalObserverFillAFilterIds(uint32_t *ids, uint8_t maxCount) {
    if (!ids || maxCount < 5) return 0;
    uint8_t count = 0;
    ids[count++] = 280;
    ids[count++] = 390;
    ids[count++] = 921;
    ids[count++] = 1016;
    ids[count++] = 1021;

    uint8_t observerCount = (uint8_t)signalObserverCount;
    if (observerCount > kSignalObserverMaxSignals) observerCount = kSignalObserverMaxSignals;
    for (uint8_t i = 0; i < observerCount && count < maxCount; ++i) {
        const SignalObserverDef &def = signalObserverDefs[i];
        if (!def.enabled || (def.channelMask & kSignalObserverChannelA) == 0) continue;
        if (signalObserverIdAlreadyPresent(ids, count, def.frameId)) continue;
        ids[count++] = def.frameId;
    }
    return count;
}


inline constexpr uint8_t kATxGuardReasonNone = 0;
inline constexpr uint8_t kATxGuardReasonTec = 1;
inline constexpr uint8_t kATxGuardReasonEflg = 2;
inline constexpr uint8_t kATxGuardReasonTxFail = 3;
inline constexpr uint32_t kATxGuardDurationMs = 15000;
inline constexpr uint8_t kATxGuardTecThreshold = 24;
inline constexpr uint32_t kAMcpBusOffRecoverIntervalMs = 1000;
inline constexpr uint32_t kAMcpBusOffRestartFallbackMs = 10000;


// 사용자가 차량 화면의 AP/Nag 경고를 직접 본 순간을 찍는 수동 마커.
// eventLog에는 ms 단위 이벤트로, timeseries에는 5초 구간 델타로 함께 남긴다.
inline constexpr uint32_t kUserMarkerApWarningStart = 1;
inline constexpr uint32_t kUserMarkerApWarningEnd = 2;
inline constexpr uint32_t kUserMarkerApWarning = kUserMarkerApWarningStart;
inline Shared<uint32_t> userMarkerCount{0};
inline Shared<uint32_t> userMarkerLastMs{0};
inline Shared<uint32_t> userMarkerLastDetail{0};
inline Shared<bool> userMarkerActive{false};

inline const char* userMarkerDetailName(uint32_t detail) {
    switch (detail) {
    case kUserMarkerApWarningStart: return "AP_WARNING_START";
    case kUserMarkerApWarningEnd: return "AP_WARNING_END";
    default: return "UNKNOWN";
    }
}

inline constexpr uint16_t kNagFixedTargetId = 880;  // 4/10 정상 기준: B채널 Nag 대상 ID 고정

inline constexpr uint8_t kNagDecisionNone = 0;
inline constexpr uint8_t kNagDecisionEcho = 1;
inline constexpr uint8_t kNagDecisionRuntimeOff = 2;
inline constexpr uint8_t kNagDecisionHandsOn = 3;
inline constexpr uint8_t kNagDecisionDasIdle = 4;
inline constexpr uint8_t kNagDecisionLateDrop = 5;
inline constexpr uint8_t kNagDecisionNo880 = 6;
inline constexpr uint8_t kNagDecisionNo921 = 7;
inline constexpr uint8_t kNagDecisionNoEcho = 8;
inline constexpr uint8_t kNagDecisionApBlocked = 9;

inline const char* nagDecisionName(uint8_t code) {
    switch (code) {
    case kNagDecisionEcho: return "ECHO";
    case kNagDecisionRuntimeOff: return "OFF";
    case kNagDecisionHandsOn: return "HANDS_ON";
    case kNagDecisionDasIdle: return "DAS_IDLE";
    case kNagDecisionLateDrop: return "LATE_DROP";
    case kNagDecisionNo880: return "NO_880";
    case kNagDecisionNo921: return "NO_921";
    case kNagDecisionNoEcho: return "NO_ECHO";
    case kNagDecisionApBlocked: return "AP_BLOCK";
    default: return "NONE";
    }
}

inline bool nagDasStateRequiresEcho(uint8_t state) {
    return state == 2 || (state >= 3 && state <= 7) || state == 9 || state == 10;
}

inline const char* dasHandsOnStateName(uint8_t state) {
    switch (state) {
    case 0: return "NOT_REQD";
    case 1: return "REQD_DETECTED";
    case 2: return "REQD_NOT_DETECTED";
    case 3: return "REQD_VISUAL";
    case 4: return "REQD_CHIME_1";
    case 5: return "REQD_CHIME_2";
    case 6: return "REQD_SLOWING";
    case 7: return "REQD_STRUCK_OUT";
    case 8: return "SUSPENDED";
    case 9: return "REQD_ESCALATED_CHIME_1";
    case 10: return "REQD_ESCALATED_CHIME_2";
    case 15: return "SNA";
    case 0xFF: return "NOT_SEEN";
    default: return "UNKNOWN";
    }
}

inline const char* dasHandsOnStateGroup(uint8_t state) {
    switch (state) {
    case 0: return "none";
    case 1: return "required_detected";
    case 2: return "required_not_detected";
    case 3: return "visual";
    case 4:
    case 5: return "chime";
    case 6:
    case 7: return "penalty";
    case 8: return "suspended";
    case 9:
    case 10: return "escalated_chime";
    case 15: return "sna";
    case 0xFF: return "not_seen";
    default: return "unknown";
    }
}

inline uint8_t dasHandsOnWarningLevel(uint8_t state) {
    switch (state) {
    case 1:
    case 2: return 1;
    case 3: return 2;
    case 4:
    case 5: return 3;
    case 9:
    case 10: return 4;
    case 6:
    case 7: return 5;
    default: return 0;
    }
}

inline bool dasHandsOnStateIsWarning(uint8_t state) {
    return dasHandsOnWarningLevel(state) >= 2;
}

// 5초 시계열/상태 로그용 구간 판정.
// nagLastDecision은 마지막 880 프레임의 실제 분기이고, 이 함수는 "이번 5초"의 요약 verdict다.
inline uint8_t nagIntervalDecision(uint32_t d880, uint32_t dDasStatus, uint32_t dEcho,
                                   uint32_t dDrop, uint32_t dSkipRuntime,
                                   uint32_t dSkipHandsOn, uint32_t dSkipDas,
                                   bool runtimeOn, uint32_t dSkipAp = 0) {
    if (d880 == 0) return kNagDecisionNo880;
    if (!runtimeOn || dSkipRuntime > 0) return kNagDecisionRuntimeOff;
    if (dEcho > 0) return kNagDecisionEcho;
    if (dDrop > 0) return kNagDecisionLateDrop;
    if (dSkipAp > 0) return kNagDecisionApBlocked;
    if (dSkipHandsOn > 0) return kNagDecisionHandsOn;
    if (dSkipDas > 0) return kNagDecisionDasIdle;
    if (dDasStatus == 0) return kNagDecisionNo921;
    return kNagDecisionNoEcho;
}

// ===================================================================
// 👇 [추가] B채널 상태 추적 변수 (DEBUG_NAG_KILLER 호출 시에만 업데이트)
// ===================================================================
struct BChannelDiagnostics {
    Shared<bool> driverBInitialized{false};      // driverB->init() 성공 여부
    Shared<bool> nagTaskCreated{false};          // xTaskCreatePinnedToCore 성공 여부
    Shared<bool> twaiConnected{false};           // TWAI 연결 상태
    Shared<bool> lastTwaiOk{false};              // 마지막 read() 성공 여부
    Shared<uint32_t> frameIdReceived{0};         // 마지막 수신 프레임 ID
    Shared<uint32_t> framesReceivedTotal{0};     // 총 수신 프레임 수
    Shared<float>    frameHz{0.0f};              // B채널 ID 880 수신 속도 (Hz)
    Shared<float>    filteredHz{0.0f};           // B채널 감시 ID 합산 속도 (880/921/923/297 Hz)
    Shared<uint32_t> frames880{0};               // ID 880 수신 프레임 수
    Shared<uint32_t> frames921{0};               // ID 921 수신 프레임 수
    Shared<uint32_t> frames923{0};               // ID 923 수신 프레임 수 (DAS_status 후보)
    Shared<uint32_t> framesFilteredInTotal{0};   // SW 필터 통과 프레임 수 (감시 ID)
    Shared<uint32_t> framesFilteredOutTotal{0};  // SW 필터 제외 프레임 수
    Shared<uint32_t> echoCount{0};               // 발사한 에코 패킷 수
    Shared<uint32_t> skipRuntimeOrInactive{0};   // nag 비활성/런타임 OFF로 스킵된 880 수
    Shared<uint32_t> skipApState{0};             // Mode B AP state gate로 스킵된 880 수
    Shared<uint32_t> skipHandsOn{0};             // handsOn!=0 로 스킵된 880 수
    Shared<uint32_t> skipDasState{0};            // DAS 만족/대기/미지원 상태로 스킵된 880 수
    Shared<uint32_t> last880RxMs{0};             // 마지막 880 수신 시각
    Shared<uint32_t> last921RxMs{0};             // 마지막 921 수신 시각
    Shared<uint32_t> last923RxMs{0};             // 마지막 923 수신 시각
    Shared<uint32_t> lastEchoTxMs{0};            // 마지막 echo 발사 시각
    Shared<uint8_t>  nagLastDecision{kNagDecisionNone}; // 마지막 NagHandler 판정
    Shared<uint8_t> twaiStateCode{0};            // TWAI 상태 코드 (0=초기, 1=정상, 2=Bus Off, 3=복구중)
    Shared<uint32_t> busoffCount{0};             // 누적 BUS-OFF 발생 횟수
    Shared<uint32_t> recoveryAttemptCount{0};    // BUS-OFF 복구 시도 횟수
    Shared<uint32_t> recoverySuccessCount{0};    // 복구 후 RUNNING 복귀 성공 횟수
    Shared<uint32_t> recoveryFailCount{0};       // 복구 시작/재시작 실패 횟수
    Shared<uint32_t> lastBusoffMs{0};            // 최근 BUS-OFF 감지 시각(ms)
    Shared<uint32_t> lastRecoveryStartMs{0};     // 최근 복구 시작 시각(ms)
    Shared<uint32_t> lastRecoveryDoneMs{0};      // 최근 복구 완료 시각(ms)
    Shared<uint32_t> lastRecoveryDurationMs{0};  // 최근 복구 소요 시간(ms)
    Shared<uint32_t> maxRecoveryDurationMs{0};   // 세션 내 최대 복구 소요 시간(ms)
    Shared<uint32_t> twaiRxErrPeak{0};           // RX 에러 카운터 최대값 (피크)
    Shared<uint32_t> twaiTxErrPeak{0};           // TX 에러 카운터 최대값 (피크)
    Shared<uint32_t> twaiRxErrNow{0};            // RX 에러 카운터 현재값
    Shared<uint32_t> twaiTxErrNow{0};            // TX 에러 카운터 현재값
    // ── B채널 TWAI 심층 진단 카운터 (twai_status_info_t 누적값, 5초 폴링) ────
    //  · arbLost     : Arbitration Lost (다른 노드와 같은 ID 송신 충돌 → 가설 H2 동등)
    //  · busError    : Bit/Stuff/CRC/Form/ACK 에러 누적 (가설 H1: ACK 부재 후보)
    //  · txFailed    : TWAI 드라이버 송신 실패 누적
    //  · rxMissed    : RX 큐 가득 차서 놓친 프레임 (B루프 폴링 부족)
    Shared<uint32_t> bArbLost{0};
    Shared<uint32_t> bBusError{0};
    Shared<uint32_t> bTxFailed{0};
    Shared<uint32_t> bRxMissed{0};
    // v2 stats: 실시간 에코 품질 지표
    Shared<uint32_t> txFail{0};                  // driver.send() 실패 누적 수
    Shared<uint32_t> echoLatUs{0};               // 최근 에코 레이턴시 (µs)
    Shared<uint8_t>  realHo{0};                  // 버스에서 읽은 실제 handsOn 값 (0..3)
    Shared<float>    realTorqueNm{0.0f};          // 버스에서 읽은 실제 토크 (Nm)
    // DAS_status 진단: 921/923 후보 모두 지원 (0xFF = 아직 DAS_status 미수신)
    Shared<uint8_t>  dasHandsOnStateRx{0xFF};    // (frame.data[5]>>2)&0x0F, 0xFF=미수신
    Shared<uint32_t> dasStatusSourceId{0};       // 마지막 DAS_status 소스 ID (921 또는 923)
    Shared<uint32_t> lastDasStatusRxMs{0};       // 마지막 DAS_status(921/923) 수신 시각
    Shared<uint32_t> nagFiredNoDas{0};           // DAS_status 미수신 상태에서 에코 발사 누적
    Shared<uint32_t> echoDroppedLate{0};         // 수신→에코 6ms 초과로 드롭된 에코 수 (ECU TX 충돌 방지)
    // ── Mode B (스마트 상태머신) 진단 필드 ──────────────────────────────────
    Shared<uint8_t>  nagMode{1};                     // 호환용 모드 값 (현재는 스마트 토크 고정)
    Shared<uint8_t>  smartProfile{0};                // 스마트 토크 실험 프로파일 (0=기본, 1=A안, 2=B안, 3=C안, 4=D안)
    Shared<uint8_t>  dasAutopilotStateRx{0};         // DAS_status DAS_autopilotState (0|4@1+)
    Shared<float>    steeringAngleDeg{0.0f};         // ID 297 SCCM_steeringAngle (deg)
    Shared<uint32_t> frames297{0};                   // ID 297 수신 프레임 수
    Shared<uint32_t> last297RxMs{0};                 // 마지막 297 수신 시각
    // Mode B 상태머신 페이즈 (0=idle 1=grace 2=state2_delay 3=state2_mild 4=strong_delay 5=strong_ramp 6=strong_hold)
    Shared<uint8_t>  modeBPhase{0};
    Shared<uint32_t> modeBInjectCount{0};            // Mode B 토크 주입 횟수
    Shared<float>    modeBLastTorqueNm{0.0f};        // Mode B 최근 주입 토크 (Nm)
    Shared<uint32_t> modeBStateEnterMs{0};           // 현재 DAS hands-on state 진입 시각
    Shared<uint32_t> modeBPhaseEnterMs{0};           // 현재 Mode B phase 진입 시각
    Shared<uint32_t> modeBFirstEchoDelayMs{0};       // 현재 state 진입 후 첫 Mode B echo까지 걸린 시간(ms), 0=아직 없음
    // BUS-OFF 복구 쿨다운 (ms): 웹 UI /api/busoff-cooldown으로 런타임 조정 가능
    // 300~10000ms 범위, 기본 1000ms. nagKillerTask → driverB->setCooldownMs() 경로로 적용
    Shared<uint32_t> busoffCooldownMs{1000};
    Shared<uint32_t> lastStatusUpdateMs{0};      // 마지막 상태 업데이트 시각
    Shared<uint32_t> lastFrameRxMs{0};           // 마지막 B채널 프레임 수신 시각
    Shared<uint32_t> lastLoopMs{0};              // 마지막 B태스크 루프 시각
    Shared<int32_t> taskCoreId{-1};              // B채널 태스크가 실행 중인 코어 ID
};

inline BChannelDiagnostics bChannelDiag;

// ===================================================================
// 👇 [추가] A채널 상태 추적 변수
// ===================================================================
struct AChannelDiagnostics {
    Shared<uint32_t> framesReceivedTotal{0};     // 총 수신 프레임 수
    Shared<float>    frameHz{0.0f};              // A채널 수신 프레임레이트 (Hz)
    Shared<uint32_t> frames280{0};               // DI_systemStatus 프레임 수
    Shared<uint32_t> frames390{0};               // Drive inverter 상태 프레임 수
    Shared<uint32_t> frames921{0};               // DAS 상태 프레임 수
    Shared<uint32_t> frames1016{0};              // UI_driverAssistControl 프레임 수
    Shared<uint32_t> frames1021{0};              // UI_autopilotControl 프레임 수
    Shared<uint32_t> summonUnlockModifiedCount{0}; // 조건부 Summon Unlock 적용 횟수
    Shared<uint32_t> tsllcModifiedCount{0};       // TSLLC 주입 횟수 (스톱/초록불 비트 세팅)
    Shared<uint32_t> lastFrameIdReceived{0};     // 마지막 수신 프레임 ID
    Shared<uint32_t> lastStatusUpdateMs{0};      // 마지막 상태 업데이트 시각
    Shared<uint32_t> lastLoopMs{0};              // 마지막 A루프 실행 시각
    Shared<int32_t> loopCoreId{-1};              // A채널 루프가 실행 중인 코어 ID
    // MCP2515 EFLG 에러 상태 (5초 주기 폴링, Normal Mode 유지)
    // bit5=TXBO(BUS-OFF), bit4=TXEP(TX에러패시브≥128), bit2=TXWAR(TEC≥96)
    // bit7=RX1OVR, bit6=RX0OVR
    Shared<uint8_t>  mcpEflg{0};                // 현재 EFLG 레지스터 값
    Shared<uint8_t>  mcpEflgPeak{0};            // 세션 내 최악 EFLG (누적 OR)
    Shared<uint32_t> mcpTxBoCount{0};           // BUS-OFF 진입 횟수 (TXBO 비트 감지)
    // ── A채널 송수신 진단 카운터 (5초 폴링 + handler 호출 경로) ──────────────
    // 가설 분리용 핵심 신호:
    //  · aTxOk/aTxFail : sendCheck() 결과. Fail↑ + MERRF↑ → ALLTXBUSY 또는 ACK부재
    //  · aTec/aRec     : 0~255 실시간 카운터. Peak로 BUS-OFF 임박 추적
    //  · aMerrfCount   : 메시지 에러(ACK/Bit/Stuff) 발생 횟수
    //  · aRxOvrCount   : RX 버퍼 오버런 발생 횟수 (clear 후 재발 = 폴링 부족)
    Shared<uint32_t> aTxOk{0};
    Shared<uint32_t> aTxFail{0};
    Shared<uint8_t>  aTec{0};
    Shared<uint8_t>  aRec{0};
    Shared<uint8_t>  aTecPeak{0};
    Shared<uint32_t> aMerrfCount{0};
    Shared<uint32_t> aRxOvrCount{0};
    Shared<uint8_t>  aRecPeak{0};               // REC 피크값 (세션 내 최대값)
    Shared<uint32_t> lastFrameRxMs{0};          // 마지막 A채널 프레임 수신 시각
    Shared<uint32_t> lastTxMs{0};               // 마지막 TX 성공 시각
    Shared<uint32_t> mcpEflgEventCount{0};      // EFLG 0→비제로 전환 횟수 (에러 발생 이벤트)
    Shared<uint32_t> mcpRecoveryAttemptCount{0}; // MCP2515 BUS-OFF 재초기화 시도 횟수
    Shared<uint32_t> mcpRecoverySuccessCount{0}; // MCP2515 BUS-OFF 재초기화 성공 횟수
    Shared<uint32_t> mcpRecoveryFailCount{0};    // MCP2515 BUS-OFF 재초기화 실패 횟수
    Shared<uint32_t> mcpBusOffSinceMs{0};        // 현재 BUS-OFF 지속 시작 시각(ms), 아니면 0
    Shared<uint32_t> mcpLastRecoveryMs{0};       // 최근 MCP2515 재초기화 시각(ms)
    Shared<uint32_t> aTxGuardUntilMs{0};       // A채널 1021 수정 송신 보호모드 종료 시각
    Shared<uint32_t> aTxGuardCount{0};         // 보호모드 진입 횟수
    Shared<uint32_t> aTxGuardSkipCount{0};     // 보호모드로 TSLLC/Summon 송신을 건너뛴 횟수
    Shared<uint8_t>  aTxGuardLastReason{0};    // kATxGuardReason*
};

inline AChannelDiagnostics aChannelDiag;

inline bool aTxGuardActive(uint32_t nowMs)
{
    if (!(bool)aTxGuardRuntime) return false;
    return (uint32_t)aChannelDiag.aTxGuardUntilMs > nowMs;
}

inline const char* aTxGuardReasonName(uint8_t reason)
{
    switch (reason) {
    case kATxGuardReasonTec: return "TEC";
    case kATxGuardReasonEflg: return "EFLG";
    case kATxGuardReasonTxFail: return "TX_FAIL";
    default: return "NONE";
    }
}

// ===================================================================
// BUS-OFF 이벤트 자동 로그 (차량 운행 중 발생 시 자동 기록, 16개 링 버퍼)
// nagKillerTask에서 busoffCount 증가 감지 시 push, 통합 로그에 포함
// ===================================================================
struct BusOffEvent {
    uint32_t timestampMs;   // 발생 시각 (millis)
    uint32_t seqNum;        // 누적 BUS-OFF 번호
    uint32_t tec;           // TX 에러 카운터 (BUS-OFF 진입 직전)
    uint32_t rec;           // RX 에러 카운터 (BUS-OFF 진입 직전)
    uint32_t recoveryDurMs; // 복구 소요 시간 (ms)
    uint32_t sinceLastMs;   // 직전 이벤트로부터 경과 시간 (ms), 0=첫 이벤트
    uint8_t  recovered;     // 1=성공, 0=실패
    uint8_t  pad[3];        // 구조체 정렬
};

struct BusOffEventLog {
    static constexpr uint8_t kCapacity = 16; // 16개 × 28 bytes = 448 bytes RAM
    BusOffEvent entries[kCapacity] = {};
    uint32_t head = 0;  // 다음 쓸 위치 (총 누적 카운트)

    void push(const BusOffEvent& ev) {
        entries[head % kCapacity] = ev;
        head++;
    }
    uint32_t count() const { return head; }
    const BusOffEvent& at(uint32_t idx) const {
        return entries[idx % kCapacity];
    }
    void clear() { head = 0; }
};

inline BusOffEventLog busOffLog;
inline uint8_t readMuxID(const CanFrame &frame)
{
    return frame.data[0] & 0x07;
}

// FSD 선택 여부 읽기 (data[4] bit6)
inline bool isFSDSelectedInUI(const CanFrame &frame)
{
    if (bypassTlsscRequirementRuntime)
        return true;
    return (frame.data[4] >> 6) & 0x01;
}

inline void setTrackModeRequest(CanFrame &frame, uint8_t request)
{
    frame.data[0] &= static_cast<uint8_t>(~0x03);
    frame.data[0] |= static_cast<uint8_t>(request & 0x03);
}

// (미사용) 속도 프로필 V12/V13 설정 (data[6] bit1~2)
inline void setSpeedProfileV12V13(CanFrame &frame, int profile)
{
    frame.data[6] &= ~0x06;
    frame.data[6] |= (profile << 1);
}

// 테슬라 전용 CAN 체크섬 계산
// 계산 방식: (ID 하위 8비트 + ID 상위 8비트 + 체크섬 바이트 제외 전체 data) & 0xFF
// DBC 기준 예시: BO_ 659(UI_chassisControl)의 SG_ UI_chassisControlChecksum : 56|8@1+
// 기본 checksumByteIndex=7(byte7)을 사용하며, 659 프레임 주입 시 이 함수를 호출
inline uint8_t computeTeslaChecksum(const CanFrame &frame, uint8_t checksumByteIndex = 7)
{
    if (checksumByteIndex >= frame.dlc)
        return 0;

    uint16_t sum = static_cast<uint16_t>(frame.id & 0xFF) +
                   static_cast<uint16_t>((frame.id >> 8) & 0xFF);
    for (uint8_t i = 0; i < frame.dlc; ++i)
    {
        if (i == checksumByteIndex)
            continue;
        sum += frame.data[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

// Counter 52|4, checksum 56|8 형식의 8바이트 Tesla 프레임 송신 전 마무리
inline void finalizeTeslaCounter52Checksum56(CanFrame &frame)
{
    if (frame.dlc < 8)
        return;

    const uint8_t counter = static_cast<uint8_t>(((frame.data[6] >> 4) + 1U) & 0x0FU);
    frame.data[6] = static_cast<uint8_t>((frame.data[6] & 0x0FU) | (counter << 4));
    frame.data[7] = computeTeslaChecksum(frame, 7);
}

// CAN 프레임의 특정 비트 설정/해제
// bit: 0~63 (byte0-bit0 ~ byte7-bit7), 리틀엔디안 바이트 순서
// value: true=1로 설정, false=0으로 클리어
inline void setBit(CanFrame &frame, int bit, bool value)
{
    if (bit < 0 || bit >= 64)
        return; // 범위 검사: CanFrame.data는 8바이트
    int byteIndex = bit / 8;
    int bitIndex = bit % 8;
    uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if (value)
    {
        frame.data[byteIndex] |= mask;
    }
    else
    {
        frame.data[byteIndex] &= static_cast<uint8_t>(~mask);
    }
}

// ===================================================================
// [B채널] Nag Killer 런타임 설정 구조체
// 현재 실차 기준은 스마트 토크 / ID 880 고정이다. 일부 필드는 NVS/API 호환용으로 유지한다.
// ===================================================================

// 토크 하드 캡 (펌웨어에서 강제, 대시보드에서 초과 불가)
//   +1.80 Nm = raw 2230 = 0x8B6
//   -1.80 Nm = raw 1870 = 0x74E
inline constexpr uint16_t kNagTorqueRawMax = 0x8B6;
inline constexpr uint16_t kNagTorqueRawMin = 0x74E;
inline constexpr uint8_t  kNagMaxTorqueEntries = 8;

// legacy NVS/API mode 값은 읽지 않고 스마트 상태머신 값만 기록한다.
inline constexpr uint8_t kNagModeB      = 1;

inline constexpr uint8_t kNagSmartProfileDefault = 0;
inline constexpr uint8_t kNagSmartProfileA       = 1;
inline constexpr uint8_t kNagSmartProfileB       = 2;
inline constexpr uint8_t kNagSmartProfileC       = 3;
inline constexpr uint8_t kNagSmartProfileD       = 4;

struct NagSmartProfileSettings {
    uint8_t id;
    const char *label;
    const char *summary;
    uint16_t state1GraceMs;
    uint16_t state2DelayMs;
    uint16_t strongDelayMs;
    uint16_t strongRampMs;
    uint16_t state2MildMinRawDelta;
    uint16_t state2MildMaxRawDelta;
    uint16_t state2BurstMs;
    uint16_t state2PauseMs;
    uint16_t strongBurstMs;
    uint16_t strongPauseMs;
};

// 2026-05-09 실차 로그 canmod_20260509_165201 기준 1차 조정.
// AP=3 허용 구간 첫 echo가 2000/1000ms로 늦어 state2/strong 지연만 줄이고 AP gate는 유지한다.
inline constexpr uint16_t kNagModeBState1GraceMs = 500;
inline constexpr uint16_t kNagModeBState2DelayMs = 700;
inline constexpr uint16_t kNagModeBStrongDelayMs = 400;
inline constexpr uint16_t kNagModeBStrongRampMs = 500;
inline constexpr uint16_t kNagModeBState2MildMinRawDelta = 50;   // 0.50 Nm
inline constexpr uint16_t kNagModeBState2MildMaxRawDelta = 150;  // 1.50 Nm

inline constexpr NagSmartProfileSettings kNagSmartProfileDefaultSettings = {
    kNagSmartProfileDefault,
    "기본",
    "현재 검증 기준. 700/400ms 타이밍을 유지하고 조건이 맞는 동안 연속 관찰 주입.",
    kNagModeBState1GraceMs,
    kNagModeBState2DelayMs,
    kNagModeBStrongDelayMs,
    kNagModeBStrongRampMs,
    kNagModeBState2MildMinRawDelta,
    kNagModeBState2MildMaxRawDelta,
    0,
    0,
    0,
    0,
};

inline constexpr NagSmartProfileSettings kNagSmartProfileASettings = {
    kNagSmartProfileA,
    "A안",
    "초기 grace를 줄이고 짧은 burst 후 쉬는 구간을 둔다.",
    150,
    kNagModeBState2DelayMs,
    kNagModeBStrongDelayMs,
    kNagModeBStrongRampMs,
    kNagModeBState2MildMinRawDelta,
    kNagModeBState2MildMaxRawDelta,
    250,
    750,
    500,
    1000,
};

inline constexpr NagSmartProfileSettings kNagSmartProfileBSettings = {
    kNagSmartProfileB,
    "B안",
    "가장 보수적. state1 주입을 없애고 더 짧게 반응한 뒤 길게 관찰한다.",
    0,
    900,
    600,
    kNagModeBStrongRampMs,
    kNagModeBState2MildMinRawDelta,
    kNagModeBState2MildMaxRawDelta,
    150,
    1350,
    300,
    1700,
};

inline constexpr NagSmartProfileSettings kNagSmartProfileCSettings = {
    kNagSmartProfileC,
    "C안",
    "1차 delay+torque 후보. state2는 600ms로 앞당기고 mild 상한을 1.7Nm까지 올리며 strong은 400ms/2.10Nm 유지.",
    kNagModeBState1GraceMs,
    600,
    kNagModeBStrongDelayMs,
    kNagModeBStrongRampMs,
    kNagModeBState2MildMinRawDelta,
    170,
    0,
    0,
    0,
    0,
};

inline constexpr NagSmartProfileSettings kNagSmartProfileDSettings = {
    kNagSmartProfileD,
    "D안",
    "C안 + 직선 저조향각 sign hold 후보. 토크와 timing은 C안과 같고 방향만 1.5초 유지.",
    kNagModeBState1GraceMs,
    600,
    kNagModeBStrongDelayMs,
    kNagModeBStrongRampMs,
    kNagModeBState2MildMinRawDelta,
    170,
    0,
    0,
    0,
    0,
};

inline uint8_t nagSmartProfileClamp(uint8_t profile) {
    return (profile <= kNagSmartProfileD) ? profile : kNagSmartProfileDefault;
}

inline const NagSmartProfileSettings& nagSmartProfileSettings(uint8_t profile) {
    switch (nagSmartProfileClamp(profile)) {
    case kNagSmartProfileA: return kNagSmartProfileASettings;
    case kNagSmartProfileB: return kNagSmartProfileBSettings;
    case kNagSmartProfileC: return kNagSmartProfileCSettings;
    case kNagSmartProfileD: return kNagSmartProfileDSettings;
    default: return kNagSmartProfileDefaultSettings;
    }
}

struct NagConfig {
    uint8_t  mode;                              // 호환용: 현재 항상 kNagModeB
    uint8_t  smartProfile;                      // 스마트 토크 실험 프로파일
    uint16_t targetId;                          // 현재 항상 kNagFixedTargetId(880)
    uint8_t  torqueCount;                       // 호환용
    uint8_t  torqueB2[kNagMaxTorqueEntries];    // 호환용
    uint8_t  torqueB3[kNagMaxTorqueEntries];    // 호환용
    uint8_t  hoRatePct;                         // 호환용
};

inline void nagCfgDefaultsSmart(NagConfig &c) {
    c.mode         = kNagModeB;
    c.smartProfile = kNagSmartProfileDefault;
    c.targetId     = kNagFixedTargetId;
    c.torqueCount  = 1;
    for (uint8_t i = 0; i < kNagMaxTorqueEntries; ++i) {
        c.torqueB2[i] = 0;
        c.torqueB3[i] = 0;
    }
    c.torqueB2[0]  = 0x08;  // +1.80 Nm
    c.torqueB3[0]  = 0xB6;
    c.hoRatePct    = 100;
}

// 토크 값 하드 캡 적용 (b2 하위 니블 + b3 조합으로 raw 계산 후 클램프)
inline void nagCfgClampTorque(uint8_t &b2, uint8_t &b3) {
    uint16_t raw = static_cast<uint16_t>(((b2 & 0x0F) << 8) | b3);
    if (raw > kNagTorqueRawMax) raw = kNagTorqueRawMax;
    if (raw < kNagTorqueRawMin) raw = kNagTorqueRawMin;
    b2 = (b2 & 0xF0) | static_cast<uint8_t>((raw >> 8) & 0x0F);
    b3 = static_cast<uint8_t>(raw & 0xFF);
}

inline void nagCfgClampAll(NagConfig &c) {
    c.mode = kNagModeB;
    c.smartProfile = nagSmartProfileClamp(c.smartProfile);
    if (c.torqueCount < 1) c.torqueCount = 1;
    if (c.torqueCount > kNagMaxTorqueEntries) c.torqueCount = kNagMaxTorqueEntries;
    if (c.hoRatePct > 100) c.hoRatePct = 100;
    for (uint8_t i = 0; i < c.torqueCount; i++)
        nagCfgClampTorque(c.torqueB2[i], c.torqueB3[i]);
}

inline NagConfig nagConfig;
#ifndef NATIVE_BUILD
#include <freertos/portmacro.h>
inline portMUX_TYPE nagCfgMux = portMUX_INITIALIZER_UNLOCKED;
#endif
