// CAN 프레임 헬퍼 함수 및 빌드 플래그별 기본값/런타임 변수 정의
#pragma once

#include <cstddef>
#include <cstring>
#include "can_frame_types.h"
#include "shared_types.h"
#ifndef NATIVE_BUILD
#include <esp_attr.h>
#endif

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
inline constexpr bool kNagKillerDefaultEnabled = false;
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
// Mode 1/2의 선제 주입 범위. true면 AP 상태 3~6에서만 주입한다.
inline constexpr bool kNagApOnlyDefaultEnabled = true;
// HW3 bit19/bit46 송신 허용 조건. ON이면 주차·Summoning·AP 안정 상태에서만
// 두 비트를 함께 적용하며, OFF는 실험을 위해 ID 1021 mux 1을 조건 없이 수정한다.
inline constexpr bool kSummonConditionLimitDefaultEnabled = true;

// TSLLC는 ev-open 플러그인의 ID/mux/bit 규칙을 사용하되, 현재 프로젝트의
// EAP/Summon과 분리된 주행 전용 안전 게이트를 적용한다.
inline constexpr uint32_t kTsllcStartupDelayMs = 15000;
inline constexpr uint32_t kTsllcMinValidAFrames = 1000;
inline constexpr uint32_t kTsllcApFreshnessMs = 500;
inline constexpr uint32_t kTsllcApStableRequiredMs = 1000;

// MCP2515 RX overrun 단계 보호 정책. 첫 발생은 일시 보류 후 상태를
// 재검증하고, 짧은 구간 내 재발하면 해당 부팅 동안 A TX를 잠근다.
inline constexpr uint32_t kASafetyHoldMinMs = 2000;
inline constexpr uint32_t kASafetyOverrunWindowMs = 60000;
inline constexpr uint32_t kASafetyRecoveryFrames = 100;
inline constexpr uint32_t kASafetyFrameFreshnessMs = 250;

inline Shared<bool> bypassTlsscRequirementRuntime{kBypassTlsscRequirementDefaultEnabled};
inline Shared<bool> isaSpeedChimeSuppressRuntime{kIsaSpeedChimeSuppressDefaultEnabled};
inline Shared<bool> emergencyVehicleDetectionRuntime{kEmergencyVehicleDetectionDefaultEnabled};
inline Shared<bool> summonUnlockRuntime{kSummonUnlockDefaultEnabled};
inline Shared<bool> summonConditionLimitRuntime{kSummonConditionLimitDefaultEnabled};
inline Shared<bool> nagKillerRuntime{kNagKillerDefaultEnabled};
inline Shared<bool> nagApOnlyRuntime{kNagApOnlyDefaultEnabled};
inline Shared<bool> tsllcRuntime{kTsllcDefaultEnabled};         // TSLLC 런타임 토글 (스톱사인/초록불 제어)
inline Shared<bool> tsllcRearmRequired{false};  // 비정상 재부팅 뒤 사용자 재승인 필요
inline Shared<bool> aChannelTxRuntime{true};     // A채널 1021 수정 송신 마스터 토글
inline Shared<uint32_t> aCanStartedMs{0};        // A MCP2515 초기화 완료 시각
inline Shared<uint32_t> aMcpSpiFreqHz{kAMcpDefaultSpiFreqHz};
inline Shared<uint32_t> aMcpRequestedSpiFreqHz{kAMcpDefaultSpiFreqHz};
inline Shared<bool> aMcpOneShotRuntime{kAMcpOneShotDefaultEnabled};
inline Shared<bool> aTxGuardRuntime{kATxGuardDefaultEnabled};

// OTA 시작 경계에서만 쓰는 송신 차단 장치.
// 기능 토글 OFF만으로는 다른 코어의 CAN 핸들러가 이미 sendCheck() 직전까지
// 진행한 경우를 막지 못한다. OTA 핸들러는 먼저 quiescing을 설정하고, 이미
// 허가된 수정 송신이 끝난 뒤에만 플래시 쓰기를 시작한다.
inline Shared<bool> canTxQuiescing{false};
inline Shared<uint32_t> canTxInFlight{0};

#ifdef NATIVE_BUILD
inline bool canTxPermitBegin()
{
    if (canTxQuiescing) return false;
    ++canTxInFlight;
    return true;
}
inline void canTxPermitEnd()
{
    if (canTxInFlight > 0) --canTxInFlight;
}
inline void canTxQuiesceBegin() { canTxQuiescing = true; }
inline void canTxQuiesceCancel() { canTxQuiescing = false; }
#else
inline portMUX_TYPE canTxQuiesceMux = portMUX_INITIALIZER_UNLOCKED;
inline bool canTxPermitBegin()
{
    portENTER_CRITICAL(&canTxQuiesceMux);
    const bool allowed = !(bool)canTxQuiescing;
    if (allowed) canTxInFlight = (uint32_t)canTxInFlight + 1;
    portEXIT_CRITICAL(&canTxQuiesceMux);
    return allowed;
}
inline void canTxPermitEnd()
{
    portENTER_CRITICAL(&canTxQuiesceMux);
    if ((uint32_t)canTxInFlight > 0) canTxInFlight = (uint32_t)canTxInFlight - 1;
    portEXIT_CRITICAL(&canTxQuiesceMux);
}
inline void canTxQuiesceBegin()
{
    portENTER_CRITICAL(&canTxQuiesceMux);
    canTxQuiescing = true;
    portEXIT_CRITICAL(&canTxQuiesceMux);
}
inline void canTxQuiesceCancel()
{
    portENTER_CRITICAL(&canTxQuiesceMux);
    canTxQuiescing = false;
    portEXIT_CRITICAL(&canTxQuiesceMux);
}
#endif

inline bool canTxQuiesceIdle() { return (uint32_t)canTxInFlight == 0; }

// Validated SummonUnlock gate state (HW3).
inline constexpr uint32_t kSummonParkedTimeoutMs = 5000;
inline constexpr uint32_t kSummonApStableRequiredMs = 1000;
inline constexpr uint16_t kSummonSpeedSnaRaw = 4095;
inline constexpr uint16_t kSummonSignalAgeSnaMs = 65535;
inline constexpr bool kSummonSpeedValidationEnabled = false;
inline constexpr char kSummonPolicyName[] = "ACA_SPR_1315";

enum SummonSessionReason : uint8_t {
    SUMMON_SESSION_IDLE = 0,
    SUMMON_SESSION_ALLOWED = 1,
    SUMMON_SESSION_DI_MISSING = 2,
    SUMMON_SESSION_DI_STALE = 3,
    SUMMON_SESSION_SPEED_MISSING = 4,
    SUMMON_SESSION_SPEED_STALE = 5,
    SUMMON_SESSION_AP_MISSING = 6,
    SUMMON_SESSION_AP_STALE = 7,
    SUMMON_SESSION_UI_MISSING = 8,
    SUMMON_SESSION_UI_STALE = 9,
    SUMMON_SESSION_GEAR_INVALID = 10,
    SUMMON_SESSION_GEAR_CONFLICT = 11,
    SUMMON_SESSION_SPEED_INVALID = 12,
    SUMMON_SESSION_PARK_MOVING = 13,
    SUMMON_SESSION_AP_ACTIVE = 14,
    SUMMON_SESSION_SPR_UNCONFIRMED = 15,
};

struct SummonGateDiagnostics {
    Shared<bool> apActive{false};
    Shared<uint8_t> apState{0};
    Shared<uint32_t> last921Ms{0};
    Shared<uint32_t> apActiveSinceMs{0};
    Shared<bool> parked{true};
    Shared<bool> summoning{false};
    Shared<bool> acaActive{false};
    Shared<bool> sprSeen{false};
    Shared<bool> sessionAllowed{false};
    Shared<uint8_t> sessionReason{SUMMON_SESSION_IDLE};
    Shared<uint8_t> diGear{0};
    Shared<uint8_t> secondaryGear{0};
    Shared<uint8_t> selfParkRequest{0};
    Shared<uint16_t> vehicleSpeedRaw{kSummonSpeedSnaRaw};
    Shared<uint32_t> last280Ms{0};
    Shared<uint32_t> last390Ms{0};
    Shared<uint32_t> last599Ms{0};
    Shared<uint32_t> last1016Ms{0};
    Shared<uint32_t> sprConfirmedMs{0};
    Shared<uint32_t> frames280{0};
    Shared<uint32_t> frames390{0};
    Shared<uint32_t> frames599{0};
    Shared<uint32_t> frames921{0};
    Shared<uint32_t> frames1016{0};
    Shared<uint32_t> mux1Received{0};
    Shared<uint32_t> txOk{0};
    Shared<uint32_t> txBusy{0};
    Shared<uint32_t> txFail{0};
    Shared<uint32_t> blocked{0};
    Shared<uint32_t> lastTxMs{0};
    Shared<uint32_t> lastBlockedMs{0};
};

inline SummonGateDiagnostics summonGateDiag;

inline const char *summonSessionReasonName(uint8_t reason) {
    switch (reason) {
    case SUMMON_SESSION_IDLE: return "IDLE";
    case SUMMON_SESSION_ALLOWED: return "ALLOWED";
    case SUMMON_SESSION_DI_MISSING: return "DI_MISSING";
    case SUMMON_SESSION_DI_STALE: return "DI_STALE";
    case SUMMON_SESSION_SPEED_MISSING: return "SPEED_MISSING";
    case SUMMON_SESSION_SPEED_STALE: return "SPEED_STALE";
    case SUMMON_SESSION_AP_MISSING: return "AP_MISSING";
    case SUMMON_SESSION_AP_STALE: return "AP_STALE";
    case SUMMON_SESSION_UI_MISSING: return "UI_MISSING";
    case SUMMON_SESSION_UI_STALE: return "UI_STALE";
    case SUMMON_SESSION_GEAR_INVALID: return "GEAR_INVALID";
    case SUMMON_SESSION_GEAR_CONFLICT: return "GEAR_CONFLICT";
    case SUMMON_SESSION_SPEED_INVALID: return "SPEED_INVALID";
    case SUMMON_SESSION_PARK_MOVING: return "PARK_MOVING";
    case SUMMON_SESSION_AP_ACTIVE: return "AP_ACTIVE";
    case SUMMON_SESSION_SPR_UNCONFIRMED: return "SPR_UNCONFIRMED";
    default: return "UNKNOWN";
    }
}

inline bool summonRequestConfirmsSession(uint8_t request) {
    switch (request) {
    case 1: case 2: case 4: case 5: case 6:
    case 7: case 8: case 10: case 11: case 12:
        return true;
    default:
        return false;
    }
}

inline bool summonRequestCancelsSession(uint8_t request) {
    return request == 3 || request == 9;
}

inline uint8_t summonEvaluateSession(uint32_t nowMs) {
    (void)nowMs;
    // 1.3.15 실차 기준: 실제 Summoning은 검증 INO와 동일하게 ACA와
    // 확인된 SelfParkRequest가 함께 유지될 때만 열린다. 1.3.16에서 추가한
    // DI_speed(0x257)·500ms 다중 검증은 MCP2515 RX 부하 때문에 사용하지 않는다.
    return (bool)summonGateDiag.acaActive && (bool)summonGateDiag.sprSeen
        ? SUMMON_SESSION_ALLOWED
        : SUMMON_SESSION_IDLE;
}

inline int8_t summonGearState(uint8_t gear) {
    if (gear == 1) return 1;
    if (gear == 2 || gear == 3 || gear == 4) return 0;
    return -1;
}

inline uint32_t summonApStableMs(uint32_t nowMs) {
    if (!(bool)summonGateDiag.apActive) return 0;
    const uint32_t since = (uint32_t)summonGateDiag.apActiveSinceMs;
    return since == 0 ? 0 : nowMs - since;
}

inline bool summonApStable(uint32_t nowMs) {
    return (bool)summonGateDiag.apActive &&
           summonApStableMs(nowMs) >= kSummonApStableRequiredMs;
}

inline bool summonGateOpen(uint32_t nowMs = 0) {
    if (!(bool)summonConditionLimitRuntime) return true;
    if (nowMs == 0) {
#ifndef NATIVE_BUILD
        nowMs = millis();
#endif
    }
    if ((bool)summonGateDiag.parked) return true;
    if ((bool)summonGateDiag.summoning) return true;
    return summonApStable(nowMs);
}

inline const char *summonGateReasonName(uint32_t nowMs) {
    if (!(bool)summonConditionLimitRuntime) return "UNRESTRICTED";
    if ((bool)summonGateDiag.summoning) return "SUMMONING";
    if ((bool)summonGateDiag.parked) return "PARKED";
    if (!(bool)summonGateDiag.apActive) return "AP_INACTIVE";
    return summonApStable(nowMs) ? "AP_STABLE" : "AP_STABILIZING";
}

inline uint8_t summonGateReasonCode(uint32_t nowMs) {
    if (!(bool)summonConditionLimitRuntime) return 0; // UNRESTRICTED
    if ((bool)summonGateDiag.summoning) return 1;
    if ((bool)summonGateDiag.parked) return 2;
    if (!(bool)summonGateDiag.apActive) return 3;
    return summonApStable(nowMs) ? 4 : 5;
}

inline const char *summonGateReasonNameFromCode(uint8_t code) {
    switch (code) {
    case 0: return "UNRESTRICTED";
    case 1: return "SUMMONING";
    case 2: return "PARKED";
    case 4: return "AP_STABLE";
    case 5: return "AP_STABILIZING";
    case 6: return "SUMMON_BLOCKED";
    default: return "AP_INACTIVE";
    }
}

inline void summonRecompute(uint32_t nowMs) {
    const uint8_t reason = summonEvaluateSession(nowMs);
    summonGateDiag.sessionReason = reason;
    summonGateDiag.sessionAllowed = reason == SUMMON_SESSION_ALLOWED;
    summonGateDiag.summoning = reason == SUMMON_SESSION_ALLOWED;
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
    summonGateDiag.diGear = gear;
    const int8_t gearState = summonGearState(gear);
    if (gearState == 1) summonGateDiag.parked = true;
    if (gearState == 0) summonGateDiag.parked = false;

    const bool aca = (frame.data[6] & 0x04U) != 0;
    if ((bool)summonGateDiag.acaActive && !aca) summonGateDiag.sprSeen = false;
    summonGateDiag.acaActive = aca;
    summonRecompute(nowMs);
    summonClearOnParkIfAcaInactive(gear);
}

inline void summonHandle390(const CanFrame &frame, uint32_t nowMs) {
    if (frame.dlc < 8) return;
    summonGateDiag.frames390 = (uint32_t)summonGateDiag.frames390 + 1;
    summonGateDiag.last390Ms = nowMs;
    const uint8_t gear = (frame.data[7] >> 3) & 0x07;
    summonGateDiag.secondaryGear = gear;
    const int8_t gearState = summonGearState(gear);
    if (gearState < 0) return;
    const uint32_t last280Ms = (uint32_t)summonGateDiag.last280Ms;
    if (last280Ms == 0 || nowMs - last280Ms > kSummonParkedTimeoutMs) {
        summonGateDiag.parked = (gearState == 1);
        summonClearOnParkIfAcaInactive(gear);
    }
    summonRecompute(nowMs);
}

inline void summonHandle921(const CanFrame &frame, uint32_t nowMs) {
    if (frame.dlc < 1) return;
    summonGateDiag.frames921 = (uint32_t)summonGateDiag.frames921 + 1;
    summonGateDiag.last921Ms = nowMs;
    const uint8_t status = frame.data[0] & 0x0F;
    const bool active = status >= 3 && status <= 6;
    if (active && !(bool)summonGateDiag.apActive)
        summonGateDiag.apActiveSinceMs = nowMs;
    if (!active)
        summonGateDiag.apActiveSinceMs = 0;
    summonGateDiag.apState = status;
    summonGateDiag.apActive = active;
}

inline void summonHandle1016(const CanFrame &frame, uint32_t nowMs) {
    if (frame.dlc < 4) return;
    summonGateDiag.frames1016 = (uint32_t)summonGateDiag.frames1016 + 1;
    summonGateDiag.last1016Ms = nowMs;
    const uint8_t spr = (frame.data[3] >> 4) & 0x0F;
    summonGateDiag.selfParkRequest = spr;
    if (summonRequestConfirmsSession(spr)) {
        summonGateDiag.sprSeen = true;
        summonGateDiag.sprConfirmedMs = nowMs;
    } else if (summonRequestCancelsSession(spr)) {
        summonGateDiag.sprSeen = false;
    }
    summonRecompute(nowMs);
}

inline void summonGateMaintain(uint32_t nowMs) {
    const uint32_t last280Ms = (uint32_t)summonGateDiag.last280Ms;
    if (last280Ms > 0 && nowMs - last280Ms > kSummonParkedTimeoutMs) {
        summonGateDiag.parked = true;
    }
    summonRecompute(nowMs);
}

inline constexpr uint8_t kATxGuardReasonNone = 0;
inline constexpr uint8_t kATxGuardReasonTec = 1;
inline constexpr uint8_t kATxGuardReasonEflg = 2;
inline constexpr uint8_t kATxGuardReasonTxFail = 3;
inline constexpr uint32_t kATxGuardDurationMs = 15000;
inline constexpr uint8_t kATxGuardTecThreshold = 24;
// aTxFail은 최종 TXERR 또는 컨트롤러 오류만 포함한다. MLOA/ABTF는 각각
// 중재 손실/중단 카운터로 분리하며 Guard 입력으로 사용하지 않는다.
// 실제 TX 오류가 1초 창에서 2건 이상이면 보호하고 EFLG·TEC 보호는 즉시 유지한다.
inline constexpr uint8_t kATxGuardTxFailBurstThreshold = 2;
inline constexpr uint32_t kAChannelWakeGapMs = 2000;
inline constexpr uint32_t kAMcpBusOffRecoverIntervalMs = 1000;
inline constexpr uint32_t kAMcpBusOffRestartFallbackMs = 10000;

// MCP2515 One-shot은 중재 손실(MLOA) 뒤 하드웨어 자동 재전송을 하지 않는다.
// 실제 Summoning(ACA+SPR) 구간에 한해서만 최신 mux 1 프레임을 짧게 한 번
// 재시도한다. 20ms를 넘긴 프레임은 차량 상태가 바뀌었을 수 있어 폐기한다.
inline constexpr uint32_t kSummonRetryDelayMs = 3;
inline constexpr uint32_t kSummonRetryExpiryMs = 20;

inline constexpr uint8_t kSummonRetryCancelNone = 0;
inline constexpr uint8_t kSummonRetryCancelExpired = 1;
inline constexpr uint8_t kSummonRetryCancelSuperseded = 2;
inline constexpr uint8_t kSummonRetryCancelGate = 3;
inline constexpr uint8_t kSummonRetryCancelGuard = 4;
inline constexpr uint8_t kSummonRetryCancelOta = 5;
inline constexpr uint8_t kSummonRetryCancelDisabled = 6;
inline constexpr uint8_t kSummonRetryCancelControllerReset = 7;
inline constexpr uint8_t kSummonRetryCancelSafety = 8;

inline constexpr uint8_t kATxSourceMaskNone   = 0;
inline constexpr uint8_t kATxSourceMaskSummon = 1U << 0;
inline constexpr uint8_t kATxSourceMaskTsllc  = 1U << 1;
inline constexpr uint8_t kATxSourceMaskOther  = 1U << 2;

inline constexpr uint8_t kACanPhaseIdle = 0;
inline constexpr uint8_t kACanPhaseRxDrain = 1;
inline constexpr uint8_t kACanPhaseFrameHandle = 2;
inline constexpr uint8_t kACanPhaseTxResult = 3;
inline constexpr uint8_t kACanPhaseBReceive = 4;
inline constexpr uint8_t kACanPhaseErrorPoll = 5;
inline constexpr uint8_t kACanPhaseIdleWait = 6;

inline const char *aCanPhaseName(uint8_t phase)
{
    switch (phase) {
    case kACanPhaseRxDrain: return "A_RX_DRAIN";
    case kACanPhaseFrameHandle: return "A_HANDLE";
    case kACanPhaseTxResult: return "A_TX_RESULT";
    case kACanPhaseBReceive: return "B_RX";
    case kACanPhaseErrorPoll: return "A_ERROR_POLL";
    case kACanPhaseIdleWait: return "IDLE_WAIT";
    default: return "IDLE";
    }
}


// 로그 분석에서 임의 이벤트 구간을 표시하는 일반 수동 마커.
// START와 END 한 쌍이 끝날 때 완료 카운트가 1 증가한다.
inline constexpr uint32_t kUserMarkerStart = 1;
inline constexpr uint32_t kUserMarkerEnd = 2;
inline Shared<uint32_t> userMarkerCount{0};
inline Shared<uint32_t> userMarkerLastMs{0};
inline Shared<uint32_t> userMarkerLastDetail{0};
inline Shared<bool> userMarkerActive{false};

inline const char* userMarkerDetailName(uint32_t detail) {
    switch (detail) {
    case kUserMarkerStart: return "USER_MARK_START";
    case kUserMarkerEnd: return "USER_MARK_END";
    default: return "UNKNOWN";
    }
}

inline constexpr uint16_t kNagFixedTargetId = 880;  // 4/10 정상 기준: B채널 Nag 대상 ID 고정
inline constexpr uint32_t kNagWarmupMs = 15000;
inline constexpr uint32_t kNagWarmupTargetFrames = 1000;
inline constexpr uint32_t kNagEchoDeadlineUs = 6000;
inline constexpr uint32_t kNagEchoMatchMaxAgeUs = 100000;
inline constexpr uint8_t kNagRecentEchoSlots = 8;

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
inline constexpr uint8_t kNagDecisionWarmup = 10;
inline constexpr uint8_t kNagDecisionModePause = 13;

inline constexpr uint8_t kNagReadinessCanWait = 0;
inline constexpr uint8_t kNagReadinessWarmupTime = 1;
inline constexpr uint8_t kNagReadinessWarmupFrames = 2;
inline constexpr uint8_t kNagReadinessReady = 3;

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
    case kNagDecisionWarmup: return "WARMUP";
    case kNagDecisionModePause: return "MODE_PAUSE";
    default: return "NONE";
    }
}

// 고빈도 ECHO/MODE_PAUSE 전환은 같은 "주입 허용" 상태로 묶고,
// 실제 차단 사유가 바뀔 때만 밀리초 이벤트를 남긴다.
inline constexpr uint8_t kNagGateUnknown = 0;
inline constexpr uint8_t kNagGateReady = 1;
inline constexpr uint8_t kNagGateOff = 2;
inline constexpr uint8_t kNagGateWarmup = 3;
inline constexpr uint8_t kNagGateHandsOn = 4;
inline constexpr uint8_t kNagGateApBlocked = 5;
inline constexpr uint8_t kNagGateDasBlocked = 6;

inline uint8_t nagDecisionGateClass(uint8_t decision) {
    switch (decision) {
    case kNagDecisionEcho:
    case kNagDecisionModePause:
    case kNagDecisionLateDrop:
    case kNagDecisionNoEcho:
        return kNagGateReady;
    case kNagDecisionRuntimeOff: return kNagGateOff;
    case kNagDecisionWarmup: return kNagGateWarmup;
    case kNagDecisionHandsOn: return kNagGateHandsOn;
    case kNagDecisionNo921:
    case kNagDecisionApBlocked:
        return kNagGateApBlocked;
    case kNagDecisionDasIdle: return kNagGateDasBlocked;
    default: return kNagGateUnknown;
    }
}

inline const char* nagGateClassName(uint8_t gateClass) {
    switch (gateClass) {
    case kNagGateReady: return "READY";
    case kNagGateOff: return "OFF";
    case kNagGateWarmup: return "WARMUP";
    case kNagGateHandsOn: return "HANDS_ON";
    case kNagGateApBlocked: return "AP_BLOCK";
    case kNagGateDasBlocked: return "DAS_BLOCK";
    default: return "UNKNOWN";
    }
}

inline const char* nagReadinessName(uint8_t code) {
    switch (code) {
    case kNagReadinessWarmupTime: return "WARMUP_TIME";
    case kNagReadinessWarmupFrames: return "WARMUP_880";
    case kNagReadinessReady: return "READY";
    default: return "CAN_WAIT";
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

inline bool nagApStateAllowsInjection(uint8_t apState) {
    return apState >= 3 && apState <= 6;
}

// 5초 시계열/상태 로그용 구간 판정.
// nagLastDecision은 마지막 880 프레임의 실제 분기이고, 이 함수는 "이번 5초"의 요약 verdict다.
inline uint8_t nagIntervalDecision(uint32_t d880, uint32_t dDasStatus, uint32_t dEcho,
                                   uint32_t dDrop, uint32_t dSkipRuntime,
                                   uint32_t dSkipHandsOn, uint32_t dSkipDas,
                                   bool runtimeOn, uint32_t dSkipAp = 0,
                                   uint32_t dSkipWarmup = 0) {
    if (d880 == 0) return kNagDecisionNo880;
    if (!runtimeOn || dSkipRuntime > 0) return kNagDecisionRuntimeOff;
    if (dEcho > 0) return kNagDecisionEcho;
    if (dDrop > 0) return kNagDecisionLateDrop;
    if (dSkipWarmup > 0) return kNagDecisionWarmup;
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
    Shared<uint32_t> frameIdReceived{0};         // 마지막 수신 프레임 ID
    Shared<uint32_t> framesReceivedTotal{0};     // 총 수신 프레임 수
    Shared<float>    frameHz{0.0f};              // B채널 ID 880 수신 속도 (Hz)
    Shared<float>    filteredHz{0.0f};           // B채널 감시 ID 합산 속도 (880/921/923 Hz)
    Shared<uint32_t> frames880{0};               // ID 880 수신 프레임 수
    Shared<uint32_t> frames921{0};               // ID 921 수신 프레임 수
    Shared<uint32_t> frames923{0};               // ID 923 수신 프레임 수 (DAS_status 후보)
    Shared<uint32_t> framesFilteredInTotal{0};   // SW 필터 통과 프레임 수 (감시 ID)
    Shared<uint32_t> framesFilteredOutTotal{0};  // SW 필터 제외 프레임 수
    Shared<uint32_t> echoCount{0};               // 발사한 에코 패킷 수
    Shared<uint32_t> txAttemptCount{0};           // 조건 통과 후 TWAI 송신을 시도한 횟수
    Shared<uint32_t> txSuccessCount{0};           // TWAI 송신 큐 등록 성공 횟수
    Shared<uint32_t> echoConfirmCount{0};         // 최근 TX 이력과 일치한 버스 수신 에코 수
    Shared<uint32_t> skipRuntimeOrInactive{0};   // nag 비활성/런타임 OFF로 스킵된 880 수
    Shared<uint32_t> skipWarmup{0};               // 부팅 준비 시간/880 누적 조건 미충족 스킵
    Shared<uint32_t> skipApState{0};             // Mode B AP state gate로 스킵된 880 수
    Shared<uint32_t> skipHandsOn{0};             // handsOn!=0 로 스킵된 880 수
    Shared<uint32_t> skipDasState{0};            // DAS 만족/대기/미지원 상태로 스킵된 880 수
    Shared<uint32_t> last880RxMs{0};             // 마지막 880 수신 시각
    Shared<uint32_t> last921RxMs{0};             // 마지막 921 수신 시각
    Shared<uint32_t> last923RxMs{0};             // 마지막 923 수신 시각
    Shared<uint32_t> lastEchoTxMs{0};            // 마지막 echo 발사 시각
    Shared<uint32_t> lastEchoRxMs{0};            // 최근 TX 프레임이 버스에서 재수신된 시각
    Shared<uint8_t>  nagLastDecision{kNagDecisionNone}; // 마지막 NagHandler 판정
    Shared<bool>     nagReady{false};             // 15초 + ID 880 1000프레임 준비 게이트 완료
    Shared<uint8_t>  nagReadiness{kNagReadinessCanWait}; // CAN_WAIT/WARMUP_TIME/WARMUP_880/READY
    Shared<uint32_t> nagWarmupStartMs{0};         // TWAI 시작 시각
    Shared<uint32_t> nagWarmupFramesSeen{0};      // TWAI 시작 뒤 실제 차량 ID 880 수신 수
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
    Shared<uint32_t> recoveryQuietRemainingMs{0}; // BUS-OFF 복구 후 B채널 TX 정지 잔여 시간
    Shared<uint32_t> recoveryQuietSkipCount{0};   // 복구 안정화 대기 중 억제한 Nag TX 누적
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
    Shared<uint32_t> txLatencyUs{0};             // ID 880 수신→TWAI 송신 등록 지연 (µs)
    Shared<uint32_t> echoLatUs{0};               // TWAI 송신→동일 프레임 버스 재수신 지연 (µs)
    Shared<uint8_t>  realHo{0};                  // 버스에서 읽은 실제 handsOn 값 (0..3)
    Shared<float>    realTorqueNm{0.0f};          // 버스에서 읽은 실제 토크 (Nm)
    Shared<uint8_t>  lastTxHandsOn{0};            // 최근 주사 프레임의 handsOn 값
    Shared<float>    lastTxTorqueNm{0.0f};         // 최근 주사 프레임의 토크 (Nm)
    // 최근 실제 차량 프레임 원문. low는 data[0..3], high는 data[4..7] little-endian pack.
    Shared<uint32_t> raw880Seq{0};
    Shared<uint32_t> raw880Low{0};
    Shared<uint32_t> raw880High{0};
    Shared<uint32_t> rawDasSeq{0};
    Shared<uint32_t> rawDasLow{0};
    Shared<uint32_t> rawDasHigh{0};
    // DAS_status 진단: 921/923 후보 모두 지원 (0xFF = 아직 DAS_status 미수신)
    Shared<uint8_t>  dasHandsOnStateRx{0xFF};    // (frame.data[5]>>2)&0x0F, 0xFF=미수신
    Shared<uint32_t> dasStatusSourceId{0};       // 마지막 DAS_status 소스 ID (921 또는 923)
    Shared<uint32_t> lastDasStatusRxMs{0};       // 마지막 DAS_status(921/923) 수신 시각
    Shared<uint32_t> nagFiredNoDas{0};           // DAS_status 미수신 상태에서 에코 발사 누적
    Shared<uint32_t> echoDroppedLate{0};         // 수신→에코 6ms 초과로 드롭된 에코 수 (ECU TX 충돌 방지)
    // ── Nag Mode 1/2 진단 필드 ──────────────────────────────────────────────
    Shared<uint8_t>  nagMode{2};                     // 1=고정, 2=순환(기본)
    Shared<uint8_t>  dasAutopilotStateRx{0};         // DAS_status DAS_autopilotState (0|4@1+)
    // 주입 페이즈 (0=휴지/차단, 1=Mode 1 고정, 2=Mode 2 순환)
    Shared<uint8_t>  modeBPhase{0};
    Shared<uint32_t> modeBInjectCount{0};            // Nag 토크 주입 횟수
    Shared<float>    modeBLastTorqueNm{0.0f};        // 최근 주입 토크 (Nm)
    // BUS-OFF 복구 쿨다운 (ms): 웹 UI /api/busoff-cooldown으로 런타임 조정 가능
    // 300~10000ms 범위, 기본 1000ms. nagKillerTask → driverB->setCooldownMs() 경로로 적용
    Shared<uint32_t> busoffCooldownMs{1000};
    Shared<uint32_t> lastFrameRxMs{0};           // 마지막 B채널 프레임 수신 시각
    Shared<uint32_t> lastLoopMs{0};              // 마지막 B태스크 루프 시각
    Shared<int32_t> taskCoreId{-1};              // B채널 태스크가 실행 중인 코어 ID
};

inline BChannelDiagnostics bChannelDiag;

// ===================================================================
// 👇 [추가] A채널 상태 추적 변수
// ===================================================================
struct AChannelDiagnostics {
    Shared<bool>     driverInitialized{false};  // MCP2515 init() 실제 성공 여부
    Shared<uint32_t> framesReceivedTotal{0};     // 총 수신 프레임 수
    Shared<float>    frameHz{0.0f};              // A채널 수신 프레임레이트 (Hz)
    Shared<uint32_t> frames280{0};               // DI_systemStatus 프레임 수
    Shared<uint32_t> frames390{0};               // Drive inverter 상태 프레임 수
    Shared<uint32_t> frames921{0};               // DAS 상태 프레임 수
    Shared<uint32_t> frames1016{0};              // UI_driverAssistControl 프레임 수
    Shared<uint32_t> frames1021{0};              // UI_autopilotControl 프레임 수
    Shared<uint32_t> summonUnlockModifiedCount{0}; // 조건부 Summon Unlock 적용 횟수
    Shared<uint32_t> tsllcModifiedCount{0};       // TSLLC 주입 횟수 (스톱/초록불 비트 세팅)
    Shared<uint32_t> tsllcTxOk{0};                // TSLLC 전용 송신 성공
    Shared<uint32_t> tsllcTxBusy{0};              // TSLLC 송신 시 MCP TX 버퍼 포화
    Shared<uint32_t> tsllcTxFail{0};              // TSLLC 전용 송신 실패
    Shared<uint32_t> lastTsllcTxMs{0};            // 최근 TSLLC 송신 성공 시각
    Shared<uint32_t> lastFrameIdReceived{0};     // 마지막 수신 프레임 ID
    Shared<uint32_t> lastLoopMs{0};              // 마지막 A루프 실행 시각
    Shared<int32_t> loopCoreId{-1};              // A채널 루프가 실행 중인 코어 ID
    // MCP2515 EFLG 에러 상태 (5초 주기 폴링, Normal Mode 유지)
    // bit5=TXBO(BUS-OFF), bit4=TXEP(TX에러패시브≥128), bit2=TXWAR(TEC≥96)
    // bit7=RX1OVR, bit6=RX0OVR
    Shared<uint8_t>  mcpEflg{0};                // 현재 EFLG 레지스터 값
    Shared<uint8_t>  mcpEflgPeak{0};            // 세션 내 최악 EFLG (누적 OR)
    Shared<uint32_t> mcpTxBoCount{0};           // BUS-OFF 진입 횟수 (TXBO 비트 감지)
    // ── A채널 송수신 진단 카운터 (1초 EFLG 폴링 + handler 호출 경로) ─────────
    // 가설 분리용 핵심 신호:
    //  · aTxOk/aTxBusy/aTxFail : 큐 등록/버퍼 포화/실제 컨트롤러 오류
    //  · aTec/aRec     : 0~255 실시간 카운터. Peak로 BUS-OFF 임박 추적
    //  · aMerrfCount   : 메시지 에러(ACK/Bit/Stuff) 발생 횟수
    //  · aRxOvrCount   : RX 버퍼 오버런 발생 횟수 (clear 후 재발 = 폴링 부족)
    Shared<uint32_t> aTxOk{0};
    Shared<uint32_t> aTxBusy{0};
    Shared<uint32_t> aTxFail{0};
    // MCP2515 최종 TX 결과를 기능별로 분리한다. aTxOk는 큐 등록 수이며,
    // 실제 버스 완료 여부는 Completed/ArbitrationLost/Aborted/Fail로 판단한다.
    Shared<uint32_t> aTxFailSummon{0};
    Shared<uint32_t> aTxFailTsllc{0};
    Shared<uint32_t> aTxFailOther{0};
    Shared<uint32_t> aTxCompleted{0};
    Shared<uint32_t> aTxArbitrationLost{0};
    Shared<uint32_t> aTxAborted{0};
    Shared<uint32_t> aTxCompletedSummon{0};
    Shared<uint32_t> aTxCompletedTsllc{0};
    Shared<uint32_t> aTxCompletedOther{0};
    Shared<uint32_t> aTxArbitrationLostSummon{0};
    Shared<uint32_t> aTxArbitrationLostTsllc{0};
    Shared<uint32_t> aTxArbitrationLostOther{0};
    Shared<uint32_t> aTxAbortedSummon{0};
    Shared<uint32_t> aTxAbortedTsllc{0};
    Shared<uint32_t> aTxAbortedOther{0};
    // 실제 Summoning 중 One-shot MLOA를 보완하는 단발 재시도 진단.
    // 누적값과 현재 Summoning 세션값을 모두 유지해 시계열/이벤트에서 비교한다.
    Shared<uint32_t> aSummonRetryScheduled{0};
    Shared<uint32_t> aSummonRetryQueued{0};
    Shared<uint32_t> aSummonRetryCompleted{0};
    Shared<uint32_t> aSummonRetryArbitrationLost{0};
    Shared<uint32_t> aSummonRetryFailed{0};
    Shared<uint32_t> aSummonRetryExpired{0};
    Shared<uint32_t> aSummonRetryCanceled{0};
    Shared<uint8_t>  aSummonRetryLastCancelReason{kSummonRetryCancelNone};
    Shared<bool>     aSummonRetryPending{false};
    Shared<uint32_t> aSummonSessionStartMs{0};
    Shared<uint32_t> aSummonSessionTxCompleted{0};
    Shared<uint32_t> aSummonSessionTxArbitrationLost{0};
    Shared<uint32_t> aSummonSessionTxAborted{0};
    Shared<uint32_t> aSummonSessionTxError{0};
    Shared<uint32_t> aSummonSessionRetryScheduled{0};
    Shared<uint32_t> aSummonSessionRetryQueued{0};
    Shared<uint32_t> aSummonSessionRetryCompleted{0};
    Shared<uint32_t> aSummonSessionRetryArbitrationLost{0};
    Shared<uint32_t> aSummonSessionRetryFailed{0};
    Shared<uint32_t> aSummonSessionRetryDiscarded{0};
    Shared<uint32_t> aSummonSessionMloaStreak{0};
    Shared<uint32_t> aSummonSessionMloaStreakMax{0};
    Shared<uint32_t> aSummonSessionLastSuccessMs{0};
    Shared<uint32_t> aSummonSessionSuccessGapLastMs{0};
    Shared<uint32_t> aSummonSessionSuccessGapMaxMs{0};
    Shared<uint32_t> tsllcSuppressedSummoningCount{0};
    Shared<uint32_t> aSummonSessionTsllcSuppressed{0};
    Shared<uint8_t>  aTxFailWindowDelta{0};     // 최근 1초 TX Fail 증가량
    Shared<uint8_t>  aTxFailWindowPeak{0};      // 세션 내 1초 TX Fail 증가량 최대값
    Shared<uint8_t>  aTec{0};
    Shared<uint8_t>  aRec{0};
    Shared<uint8_t>  aTecPeak{0};
    Shared<uint32_t> aMerrfCount{0};
    Shared<uint32_t> aRxOvrCount{0};
    Shared<uint32_t> aRx0OvrCount{0};
    Shared<uint32_t> aRx1OvrCount{0};
    Shared<uint32_t> aRxBuffer0Frames{0};
    Shared<uint32_t> aRxBuffer1Frames{0};
    Shared<uint32_t> aRxDrainCalls{0};
    Shared<uint32_t> aRxDrainFrames{0};
    Shared<uint32_t> aRxQueueHighWater{0};
    Shared<uint32_t> aRxQueueDropCount{0};
    Shared<uint8_t>  aRecPeak{0};               // REC 피크값 (세션 내 최대값)
    Shared<uint32_t> lastFrameRxMs{0};          // 마지막 A채널 프레임 수신 시각
    Shared<uint32_t> lastTxMs{0};               // 마지막 TX 성공 시각
    Shared<uint32_t> loopGapLastUs{0};           // A 폴링 호출 사이 최근 간격
    Shared<uint32_t> loopGapPeakUs{0};           // 부팅 후 A 폴링 최대 공백
    Shared<uint32_t> loopGapWindowPeakUs{0};     // 최근 EFLG 폴링 구간의 최대 공백
    Shared<uint32_t> loopGapOver250usCount{0};   // 250us 초과 A 폴링 공백 횟수
    Shared<uint32_t> loopGapOver500usCount{0};   // 500us 초과 A 폴링 공백 횟수
    Shared<uint32_t> loopGapOver1msCount{0};     // 1ms 초과 A 폴링 공백 횟수
    Shared<uint32_t> loopGapOver2msCount{0};     // 2ms를 넘긴 A 폴링 공백 횟수
    Shared<uint8_t>  canTaskPhase{kACanPhaseIdle};
    Shared<uint8_t>  loopGapWindowPeakPhase{kACanPhaseIdle};
    Shared<uint8_t>  lastOverrunPhase{kACanPhaseIdle};
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
    Shared<uint8_t>  aTxGuardTriggerSourceMask{0}; // 이번 Guard를 만든 기능 조합
    Shared<uint32_t> wakeCount{0};             // 2초 이상 수신 공백 뒤 A채널 재수신 횟수
    Shared<uint32_t> lastWakeRxMs{0};          // 최근 A채널 재수신 시작 시각
    Shared<bool>     wakeAwaitingSummonTx{false}; // 재수신 뒤 첫 Summon TX 대기
    Shared<uint32_t> wakeToSummonTxMs{0};      // 최근 재수신 시작→첫 Summon TX 성공 지연
    // TSLLC 안전 게이트 진단
    Shared<uint32_t> tsllcSkippedUnchanged{0}; // bit38/39가 이미 1이라 무송신
    Shared<uint32_t> tsllcBlockedCount{0};      // 안전 조건 미충족으로 차단
    Shared<uint8_t>  tsllcLastBlockReason{0};   // TsllcBlockReason
    // A RX overrun 단계 보호 진단
    Shared<bool>     aSafetyHold{false};
    Shared<bool>     aSafetyLatched{false};
    Shared<uint32_t> aSafetyHoldStartedMs{0};
    Shared<uint32_t> aSafetyHoldUntilMs{0};
    Shared<uint32_t> aSafetyFramesAtHold{0};
    Shared<uint32_t> aSafetyFirstOverrunMs{0};
    Shared<uint32_t> aSafetyHoldCount{0};
    Shared<uint32_t> aSafetyLatchCount{0};
    Shared<uint32_t> aSafetyRecoveryCount{0};
    Shared<uint32_t> aSafetySkipCount{0};
    // RTC 이전 부팅 스냅샷에 사용할 마지막 A TX 결과
    Shared<uint8_t>  aLastTxSource{0};
    Shared<uint8_t>  aLastTxResult{0};
};

inline AChannelDiagnostics aChannelDiag;

// 아래 TSLLC 게이트가 사용하는 헬퍼의 정의는 기존 파일 하단에 있다.
inline bool aTxGuardActive(uint32_t nowMs);
inline uint8_t readMuxID(const CanFrame &frame);

enum TsllcBlockReason : uint8_t {
    TSLLC_BLOCK_READY = 0,
    TSLLC_BLOCK_DISABLED = 1,
    TSLLC_BLOCK_REARM_REQUIRED = 2,
    TSLLC_BLOCK_A_TX_OFF = 3,
    TSLLC_BLOCK_OTA = 4,
    TSLLC_BLOCK_A_SAFETY_HOLD = 5,
    TSLLC_BLOCK_A_SAFETY_LATCH = 6,
    TSLLC_BLOCK_TX_GUARD = 7,
    TSLLC_BLOCK_STARTUP_TIME = 8,
    TSLLC_BLOCK_STARTUP_FRAMES = 9,
    TSLLC_BLOCK_SUMMONING = 10,
    TSLLC_BLOCK_AP_INACTIVE = 11,
    TSLLC_BLOCK_AP_STALE = 12,
    TSLLC_BLOCK_AP_STABILIZING = 13,
};

inline const char *tsllcBlockReasonName(uint8_t reason)
{
    switch (reason) {
    case TSLLC_BLOCK_READY: return "ACTIVE";
    case TSLLC_BLOCK_DISABLED: return "DISABLED";
    case TSLLC_BLOCK_REARM_REQUIRED: return "REARM_REQUIRED";
    case TSLLC_BLOCK_A_TX_OFF: return "A_TX_OFF";
    case TSLLC_BLOCK_OTA: return "OTA_QUIESCE";
    case TSLLC_BLOCK_A_SAFETY_HOLD: return "A_SAFETY_HOLD";
    case TSLLC_BLOCK_A_SAFETY_LATCH: return "A_SAFETY_LATCH";
    case TSLLC_BLOCK_TX_GUARD: return "TX_GUARD";
    case TSLLC_BLOCK_STARTUP_TIME: return "STARTUP_TIME";
    case TSLLC_BLOCK_STARTUP_FRAMES: return "STARTUP_FRAMES";
    case TSLLC_BLOCK_SUMMONING: return "SUMMONING";
    case TSLLC_BLOCK_AP_INACTIVE: return "AP_INACTIVE";
    case TSLLC_BLOCK_AP_STALE: return "AP_STALE";
    case TSLLC_BLOCK_AP_STABILIZING: return "AP_STABILIZING";
    default: return "UNKNOWN";
    }
}

inline bool aSafetyTxBlocked()
{
    return (bool)aChannelDiag.aSafetyHold || (bool)aChannelDiag.aSafetyLatched;
}

inline bool aSafetyRecoveryReady(uint32_t nowMs)
{
    if (!(bool)aChannelDiag.aSafetyHold || (bool)aChannelDiag.aSafetyLatched)
        return false;
    if (nowMs < (uint32_t)aChannelDiag.aSafetyHoldUntilMs)
        return false;
    if (((uint8_t)aChannelDiag.mcpEflg & 0xC0U) != 0U)
        return false;
    const uint32_t lastFrameMs = (uint32_t)aChannelDiag.lastFrameRxMs;
    if (lastFrameMs == 0U || nowMs - lastFrameMs > kASafetyFrameFreshnessMs)
        return false;
    return (uint32_t)aChannelDiag.framesReceivedTotal -
               (uint32_t)aChannelDiag.aSafetyFramesAtHold >=
           kASafetyRecoveryFrames;
}

// true면 60초 내 두 번째 overrun으로 이번 부팅 A TX를 잠근 상태다.
inline bool aSafetyRecordOverrun(uint32_t nowMs)
{
    const uint32_t firstOverrunMs = (uint32_t)aChannelDiag.aSafetyFirstOverrunMs;
    const bool repeated = firstOverrunMs != 0U &&
                          nowMs - firstOverrunMs <= kASafetyOverrunWindowMs;
    aChannelDiag.aSafetyFramesAtHold = (uint32_t)aChannelDiag.framesReceivedTotal;
    if (repeated) {
        aChannelDiag.aSafetyHold = false;
        aChannelDiag.aSafetyLatched = true;
        aChannelDiag.aSafetyHoldUntilMs = 0;
        aChannelDiag.aSafetyLatchCount =
            (uint32_t)aChannelDiag.aSafetyLatchCount + 1U;
        return true;
    }
    aChannelDiag.aSafetyFirstOverrunMs = nowMs;
    aChannelDiag.aSafetyHold = true;
    aChannelDiag.aSafetyLatched = false;
    aChannelDiag.aSafetyHoldStartedMs = nowMs;
    aChannelDiag.aSafetyHoldUntilMs = nowMs + kASafetyHoldMinMs;
    aChannelDiag.aSafetyHoldCount =
        (uint32_t)aChannelDiag.aSafetyHoldCount + 1U;
    return false;
}

inline bool aSafetyTryRecover(uint32_t nowMs)
{
    if (!aSafetyRecoveryReady(nowMs)) return false;
    aChannelDiag.aSafetyHold = false;
    aChannelDiag.aSafetyHoldUntilMs = 0;
    aChannelDiag.aSafetyRecoveryCount =
        (uint32_t)aChannelDiag.aSafetyRecoveryCount + 1U;
    return true;
}

inline uint32_t tsllcStartupElapsedMs(uint32_t nowMs)
{
    const uint32_t startedMs = (uint32_t)aCanStartedMs;
    return startedMs == 0U ? 0U : nowMs - startedMs;
}

inline uint8_t tsllcInjectionBlockReason(uint32_t nowMs)
{
    if (!(bool)tsllcRuntime) return TSLLC_BLOCK_DISABLED;
    if ((bool)tsllcRearmRequired) return TSLLC_BLOCK_REARM_REQUIRED;
    if (!(bool)aChannelTxRuntime) return TSLLC_BLOCK_A_TX_OFF;
    if ((bool)canTxQuiescing) return TSLLC_BLOCK_OTA;
    if ((bool)aChannelDiag.aSafetyLatched) return TSLLC_BLOCK_A_SAFETY_LATCH;
    if ((bool)aChannelDiag.aSafetyHold) return TSLLC_BLOCK_A_SAFETY_HOLD;
    // 실제 차량 호출 중에는 시작 대기나 AP 상태보다 Summon 우선 차단을
    // 표시해 동일 ID mux 경쟁이 없었음을 로그에서 바로 확인한다.
    if ((bool)summonGateDiag.summoning) return TSLLC_BLOCK_SUMMONING;
    if (aTxGuardActive(nowMs)) return TSLLC_BLOCK_TX_GUARD;
    const uint32_t startedMs = (uint32_t)aCanStartedMs;
    if (startedMs == 0U || nowMs - startedMs < kTsllcStartupDelayMs)
        return TSLLC_BLOCK_STARTUP_TIME;
    if ((uint32_t)aChannelDiag.framesReceivedTotal <= kTsllcMinValidAFrames)
        return TSLLC_BLOCK_STARTUP_FRAMES;
    if (!(bool)summonGateDiag.apActive) return TSLLC_BLOCK_AP_INACTIVE;
    const uint32_t last921Ms = (uint32_t)summonGateDiag.last921Ms;
    if (last921Ms == 0U || nowMs - last921Ms > kTsllcApFreshnessMs)
        return TSLLC_BLOCK_AP_STALE;
    if (summonApStableMs(nowMs) < kTsllcApStableRequiredMs)
        return TSLLC_BLOCK_AP_STABILIZING;
    return TSLLC_BLOCK_READY;
}

inline bool tsllcInjectionAllowed(uint32_t nowMs)
{
    return tsllcInjectionBlockReason(nowMs) == TSLLC_BLOCK_READY;
}

inline bool tsllcFrameNeedsModification(const CanFrame &frame)
{
    if (frame.dlc < 8 || readMuxID(frame) != 0) return false;
    return (frame.data[4] & 0xC0U) != 0xC0U;
}

// RTC slow memory에는 flash 쓰기 없이 마지막 CAN 문맥만 보존한다.
// 전원 완전 차단에서는 보존을 보장하지 않으며, magic/schema가 맞을 때만 사용한다.
inline constexpr uint32_t kRtcCanSnapshotMagic = 0x54324331UL; // "T2C1"
inline constexpr uint16_t kRtcCanSnapshotSchema = 1;

struct RtcCanSnapshot {
    uint32_t magic{0};
    uint16_t schema{0};
    uint16_t reserved{0};
    uint32_t bootCount{0};
    uint32_t uptimeMs{0};
    uint32_t resetReason{0};
    uint32_t aRxOverrunCount{0};
    uint32_t aTxQueued{0};
    uint32_t aTxFail{0};
    uint8_t aEflg{0};
    uint8_t lastTxSource{0};
    uint8_t lastTxResult{0};
    uint8_t featureFlags{0};
};

struct BootDiagnostics {
    Shared<uint32_t> resetReason{0};
    Shared<uint32_t> rtcBootCount{0};
    Shared<bool> unexpectedReset{false};
    Shared<bool> previousSnapshotValid{false};
    RtcCanSnapshot previous{};
};

inline BootDiagnostics bootDiagnostics;
#ifndef NATIVE_BUILD
RTC_DATA_ATTR inline RtcCanSnapshot rtcCanSnapshot;
#else
inline RtcCanSnapshot rtcCanSnapshot;
#endif

inline const char *bootResetReasonName(uint32_t reason)
{
    switch (reason) {
    case 1: return "POWERON";
    case 2: return "EXTERNAL";
    case 3: return "SOFTWARE";
    case 4: return "PANIC";
    case 5: return "INT_WDT";
    case 6: return "TASK_WDT";
    case 7: return "WDT";
    case 8: return "DEEPSLEEP";
    case 9: return "BROWNOUT";
    case 10: return "SDIO";
    default: return "UNKNOWN";
    }
}

inline bool bootResetReasonUnexpected(uint32_t reason)
{
    // 명시적으로 정상인 전원 인가·외부 리셋·SW 재시작·딥슬립 외에는
    // 새 ID가 추가되어도 UNKNOWN으로 안전하게 분류한다.
    return reason != 1U && reason != 2U && reason != 3U && reason != 8U;
}

inline void bootDiagnosticsBegin(uint32_t resetReason)
{
    const bool retained = resetReason != 1U &&
                          rtcCanSnapshot.magic == kRtcCanSnapshotMagic &&
                          rtcCanSnapshot.schema == kRtcCanSnapshotSchema;
    bootDiagnostics.resetReason = resetReason;
    bootDiagnostics.unexpectedReset = bootResetReasonUnexpected(resetReason);
    bootDiagnostics.previousSnapshotValid = retained;
    bootDiagnostics.previous = retained ? rtcCanSnapshot : RtcCanSnapshot{};
    const uint32_t bootCount = retained ? rtcCanSnapshot.bootCount + 1U : 1U;
    bootDiagnostics.rtcBootCount = bootCount;
    rtcCanSnapshot = {};
    rtcCanSnapshot.magic = kRtcCanSnapshotMagic;
    rtcCanSnapshot.schema = kRtcCanSnapshotSchema;
    rtcCanSnapshot.bootCount = bootCount;
    rtcCanSnapshot.resetReason = resetReason;
}

inline void updateRtcCanSnapshot(uint32_t nowMs)
{
    rtcCanSnapshot.magic = kRtcCanSnapshotMagic;
    rtcCanSnapshot.schema = kRtcCanSnapshotSchema;
    rtcCanSnapshot.bootCount = (uint32_t)bootDiagnostics.rtcBootCount;
    rtcCanSnapshot.uptimeMs = nowMs;
    rtcCanSnapshot.resetReason = (uint32_t)bootDiagnostics.resetReason;
    rtcCanSnapshot.aRxOverrunCount = (uint32_t)aChannelDiag.aRxOvrCount;
    rtcCanSnapshot.aTxQueued = (uint32_t)aChannelDiag.aTxOk;
    rtcCanSnapshot.aTxFail = (uint32_t)aChannelDiag.aTxFail;
    rtcCanSnapshot.aEflg = (uint8_t)aChannelDiag.mcpEflg;
    rtcCanSnapshot.lastTxSource = (uint8_t)aChannelDiag.aLastTxSource;
    rtcCanSnapshot.lastTxResult = (uint8_t)aChannelDiag.aLastTxResult;
    rtcCanSnapshot.featureFlags =
        ((bool)summonUnlockRuntime ? 0x01U : 0U) |
        ((bool)tsllcRuntime ? 0x02U : 0U) |
        ((bool)nagKillerRuntime ? 0x04U : 0U) |
        ((bool)aChannelTxRuntime ? 0x08U : 0U) |
        ((bool)aChannelDiag.aSafetyHold ? 0x10U : 0U) |
        ((bool)aChannelDiag.aSafetyLatched ? 0x20U : 0U);
}

inline const char* aMcpEflgStateName(uint8_t eflg)
{
    if (eflg & 0x20U) return "BUS_OFF";
    if (eflg & 0x18U) return "ERROR_PASSIVE";
    if (eflg & 0xC0U) return "RX_OVERRUN";
    if (eflg & 0x07U) return "ERROR_WARNING";
    return "OK";
}

inline uint8_t aMcpEflgSeverity(uint8_t eflg)
{
    if (eflg & 0x20U) return 2;  // error
    if (eflg != 0) return 1;     // warning
    return 0;
}

inline bool aTxGuardActive(uint32_t nowMs)
{
    if (!(bool)aTxGuardRuntime) return false;
    return (uint32_t)aChannelDiag.aTxGuardUntilMs > nowMs;
}

inline const char* summonRetryCancelReasonName(uint8_t reason)
{
    switch (reason) {
    case kSummonRetryCancelExpired: return "EXPIRED";
    case kSummonRetryCancelSuperseded: return "NEW_FRAME";
    case kSummonRetryCancelGate: return "GATE_CLOSED";
    case kSummonRetryCancelGuard: return "TX_GUARD";
    case kSummonRetryCancelOta: return "OTA_QUIESCE";
    case kSummonRetryCancelDisabled: return "DISABLED";
    case kSummonRetryCancelControllerReset: return "CONTROLLER_RESET";
    case kSummonRetryCancelSafety: return "A_SAFETY";
    default: return "NONE";
    }
}

inline bool summonRetryPolicyAllowed(uint32_t nowMs)
{
    return (bool)aMcpOneShotRuntime &&
           (bool)aChannelTxRuntime &&
           (bool)summonUnlockRuntime &&
           (bool)summonGateDiag.summoning &&
           summonGateOpen(nowMs) &&
           !aTxGuardActive(nowMs) &&
           !aSafetyTxBlocked() &&
           !(bool)canTxQuiescing;
}

inline void resetSummonTxSessionDiagnostics(uint32_t nowMs)
{
    aChannelDiag.aSummonSessionStartMs = nowMs;
    aChannelDiag.aSummonSessionTxCompleted = 0;
    aChannelDiag.aSummonSessionTxArbitrationLost = 0;
    aChannelDiag.aSummonSessionTxAborted = 0;
    aChannelDiag.aSummonSessionTxError = 0;
    aChannelDiag.aSummonSessionRetryScheduled = 0;
    aChannelDiag.aSummonSessionRetryQueued = 0;
    aChannelDiag.aSummonSessionRetryCompleted = 0;
    aChannelDiag.aSummonSessionRetryArbitrationLost = 0;
    aChannelDiag.aSummonSessionRetryFailed = 0;
    aChannelDiag.aSummonSessionRetryDiscarded = 0;
    aChannelDiag.aSummonSessionMloaStreak = 0;
    aChannelDiag.aSummonSessionMloaStreakMax = 0;
    aChannelDiag.aSummonSessionLastSuccessMs = 0;
    aChannelDiag.aSummonSessionSuccessGapLastMs = 0;
    aChannelDiag.aSummonSessionSuccessGapMaxMs = 0;
    aChannelDiag.aSummonSessionTsllcSuppressed = 0;
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

inline const char* aTxSourceName(uint8_t source)
{
    switch (source) {
    case 1: return "SUMMON";
    case 2: return "TSLLC";
    default: return "OTHER";
    }
}

inline uint8_t aTxSourceMaskFor(uint8_t source)
{
    switch (source) {
    case 1: return kATxSourceMaskSummon;
    case 2: return kATxSourceMaskTsllc;
    default: return kATxSourceMaskOther;
    }
}

inline const char* aTxSourceMaskName(uint8_t mask)
{
    switch (mask) {
    case kATxSourceMaskSummon: return "SUMMON";
    case kATxSourceMaskTsllc: return "TSLLC";
    case kATxSourceMaskOther: return "OTHER";
    case kATxSourceMaskSummon | kATxSourceMaskTsllc: return "SUMMON+TSLLC";
    case kATxSourceMaskSummon | kATxSourceMaskOther: return "SUMMON+OTHER";
    case kATxSourceMaskTsllc | kATxSourceMaskOther: return "TSLLC+OTHER";
    case kATxSourceMaskSummon | kATxSourceMaskTsllc | kATxSourceMaskOther:
        return "SUMMON+TSLLC+OTHER";
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

// BUS-OFF 진입과 복구 결과를 정확히 한 행으로 묶는 순수 기록기.
// CAN 드라이버 제어와 무관하며 native 회귀 테스트에서 동일한 기록 경로를 검증한다.
struct BusOffEventRecorder {
    uint32_t lastTimestampMs = 0;
    BusOffEvent pending = {};
    bool pendingValid = false;

    BusOffEvent begin(uint32_t timestampMs, uint32_t seqNum,
                      uint32_t tec, uint32_t rec) {
        BusOffEvent ev = {};
        ev.timestampMs = timestampMs;
        ev.seqNum = seqNum;
        ev.tec = tec;
        ev.rec = rec;
        ev.sinceLastMs = (lastTimestampMs > 0)
                         ? (uint32_t)(timestampMs - lastTimestampMs) : 0;
        pending = ev;
        pendingValid = true;
        lastTimestampMs = timestampMs;
        return ev;
    }

    bool complete(BusOffEventLog& log, bool recovered,
                  uint32_t recoveryDurationMs) {
        if (!pendingValid) return false;
        pending.recoveryDurMs = recoveryDurationMs;
        pending.recovered = recovered ? 1U : 0U;
        log.push(pending);
        pendingValid = false;
        return true;
    }
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
// [B채널] HW3 Nag Killer MODE 1/2 런타임 설정
// ===================================================================

// 토크 하드 캡 (펌웨어에서 강제, 대시보드에서 초과 불가)
//   +1.80 Nm = raw 2230 = 0x8B6
//   -1.80 Nm = raw 1870 = 0x74E
inline constexpr uint16_t kNagTorqueRawMax = 0x8B6;
inline constexpr uint16_t kNagTorqueRawMin = 0x74E;
inline constexpr uint8_t kNagMode1 = 1;
inline constexpr uint8_t kNagMode2 = 2;
// 최신 검증 원본과 동일하게 이전 Mode C/Mode 3은 제거했다.
inline constexpr uint8_t kNagModeDefault = kNagMode2;

inline uint8_t nagModeClamp(uint8_t mode) {
    return (mode == kNagMode1 || mode == kNagMode2) ? mode : kNagModeDefault;
}

inline const char *nagModeName(uint8_t mode) {
    switch (nagModeClamp(mode)) {
    case kNagMode1: return "MODE 1";
    case kNagMode2: return "MODE 2";
    default: return "MODE 2";
    }
}

inline const char *nagModeSummary(uint8_t mode) {
    switch (nagModeClamp(mode)) {
    case kNagMode1: return "선제 고정 +1.80Nm 에코. 운전자 손 감지 시 중단.";
    case kNagMode2: return "선제 1초 순환·1.5초 휴지. 운전자 손 감지 시 중단.";
    default: return "선제 1초 순환·1.5초 휴지. 운전자 손 감지 시 중단.";
    }
}

struct NagConfig {
    uint8_t mode;
};

inline void nagCfgDefaults(NagConfig &c) {
    c.mode = kNagModeDefault;
}

inline NagConfig nagConfig;
#ifndef NATIVE_BUILD
#include <freertos/portmacro.h>
inline portMUX_TYPE nagCfgMux = portMUX_INITIALIZER_UNLOCKED;
#endif
