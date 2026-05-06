// CAN 프레임 헬퍼 함수 및 빌드 플래그별 기본값/런타임 변수 정의
#pragma once

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

#if defined(ENHANCED_AUTOPILOT)
inline constexpr bool kEnhancedAutopilotDefaultEnabled = false;  // 기본값 OFF (웹 UI에서 활성화)
inline constexpr bool kEnhancedAutopilotBuildEnabled = true;
#else
inline constexpr bool kEnhancedAutopilotDefaultEnabled = false;
inline constexpr bool kEnhancedAutopilotBuildEnabled = false;
#endif

#if defined(ENHANCED_AUTOPILOT)
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
#define T2CAN_SPI_FREQ_HZ 8000000
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
inline Shared<bool> enhancedAutopilotRuntime{kEnhancedAutopilotDefaultEnabled};
inline Shared<bool> nagKillerRuntime{kNagKillerDefaultEnabled};
inline Shared<bool> tsllcRuntime{kTsllcDefaultEnabled};         // TSLLC 런타임 토글 (스톱사인/초록불 제어)
inline Shared<bool> aChannelTxRuntime{true};     // A채널 1021 수정 송신 마스터 토글
inline Shared<uint32_t> aMcpSpiFreqHz{kAMcpDefaultSpiFreqHz};
inline Shared<uint32_t> aMcpRequestedSpiFreqHz{kAMcpDefaultSpiFreqHz};
inline Shared<bool> aMcpOneShotRuntime{kAMcpOneShotDefaultEnabled};
inline Shared<bool> aTxGuardRuntime{kATxGuardDefaultEnabled};


inline constexpr uint8_t kATxGuardReasonNone = 0;
inline constexpr uint8_t kATxGuardReasonTec = 1;
inline constexpr uint8_t kATxGuardReasonEflg = 2;
inline constexpr uint8_t kATxGuardReasonTxFail = 3;
inline constexpr uint32_t kATxGuardDurationMs = 15000;
inline constexpr uint8_t kATxGuardTecThreshold = 24;


// 사용자가 차량 화면의 AP/Nag 경고를 직접 본 순간을 찍는 수동 마커.
// eventLog에는 ms 단위 이벤트로, timeseries에는 5초 구간 델타로 함께 남긴다.
inline constexpr uint32_t kUserMarkerApWarning = 1;
inline Shared<uint32_t> userMarkerCount{0};
inline Shared<uint32_t> userMarkerLastMs{0};
inline Shared<uint32_t> userMarkerLastDetail{0};

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
    default: return "NONE";
    }
}

// 5초 시계열/상태 로그용 구간 판정.
// nagLastDecision은 마지막 880 프레임의 실제 분기이고, 이 함수는 "이번 5초"의 요약 verdict다.
inline uint8_t nagIntervalDecision(uint32_t d880, uint32_t d921, uint32_t dEcho,
                                   uint32_t dDrop, uint32_t dSkipRuntime,
                                   uint32_t dSkipHandsOn, uint32_t dSkipDas,
                                   bool runtimeOn) {
    if (d880 == 0) return kNagDecisionNo880;
    if (!runtimeOn || dSkipRuntime > 0) return kNagDecisionRuntimeOff;
    if (dEcho > 0) return kNagDecisionEcho;
    if (dDrop > 0) return kNagDecisionLateDrop;
    if (dSkipHandsOn > 0) return kNagDecisionHandsOn;
    if (dSkipDas > 0) return kNagDecisionDasIdle;
    if (d921 == 0) return kNagDecisionNo921;
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
    Shared<float>    frameHz{0.0f};              // B채널 수신 프레임레이트 (Hz)
    Shared<uint32_t> frames880{0};               // ID 880 수신 프레임 수
    Shared<uint32_t> frames921{0};               // ID 921 수신 프레임 수
    Shared<uint32_t> framesFilteredInTotal{0};   // SW 필터 통과 프레임 수 (880/921)
    Shared<uint32_t> framesFilteredOutTotal{0};  // SW 필터 제외 프레임 수
    Shared<uint32_t> echoCount{0};               // 발사한 에코 패킷 수
    Shared<uint32_t> skipRuntimeOrInactive{0};   // nag 비활성/런타임 OFF로 스킵된 880 수
    Shared<uint32_t> skipHandsOn{0};             // handsOn!=0 로 스킵된 880 수
    Shared<uint32_t> skipDasState{0};            // DAS 상태(0/8)로 스킵된 880 수
    Shared<uint32_t> last880RxMs{0};             // 마지막 880 수신 시각
    Shared<uint32_t> last921RxMs{0};             // 마지막 921 수신 시각
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
    // ID 921 진단: DAS 핸즈온 상태 (0xFF = 아직 921 미수신)
    Shared<uint8_t>  dasHandsOnStateRx{0xFF};    // (frame.data[5]>>2)&0x0F, 0xFF=미수신
    Shared<uint32_t> nagFiredNoDas{0};           // DAS 921 미수신 상태에서 에코 발사 누적 (bus B에 921 없을 때 증가)
    Shared<uint32_t> echoDroppedLate{0};         // 수신→에코 6ms 초과로 드롭된 에코 수 (ECU TX 충돌 방지)
    // ── Mode B (스마트 상태머신) 진단 필드 ──────────────────────────────────
    Shared<uint8_t>  nagMode{0};                     // 현재 활성 모드 (0=A 스텔스 / 1=B 스마트)
    Shared<uint8_t>  dasAutopilotStateRx{0};         // ID 921 DAS_autopilotState (0|4@1+)
    Shared<float>    steeringAngleDeg{0.0f};         // ID 297 SCCM_steeringAngle (deg)
    Shared<uint32_t> frames297{0};                   // ID 297 수신 프레임 수
    // Mode B 상태머신 페이즈 (0=idle 1=grace 2=state2_delay 3=state2_mild 4=strong_delay 5=strong_ramp 6=strong_hold)
    Shared<uint8_t>  modeBPhase{0};
    Shared<uint32_t> modeBInjectCount{0};            // Mode B 토크 주입 횟수
    Shared<float>    modeBLastTorqueNm{0.0f};        // Mode B 최근 주입 토크 (Nm)
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
    Shared<uint32_t> frames1021{0};              // EAP 프레임 수
    Shared<uint32_t> eapModifiedCount{0};        // 규제 완화 적용 횟수
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
    Shared<uint32_t> aTxGuardUntilMs{0};       // A채널 1021 수정 송신 보호모드 종료 시각
    Shared<uint32_t> aTxGuardCount{0};         // 보호모드 진입 횟수
    Shared<uint32_t> aTxGuardSkipCount{0};     // 보호모드로 TSLLC/EAP 송신을 건너뛴 횟수
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
// nagKillerTask에서 busoffCount 증가 감지 시 push, 웹에서 CSV 다운로드
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
// 현재 실차 기준은 Mode A / ID 880 고정이다. 일부 필드는 NVS/API 호환용으로 유지한다.
// ===================================================================

// 토크 하드 캡 (펌웨어에서 강제, 대시보드에서 초과 불가)
//   +1.80 Nm = raw 2230 = 0x8B6
//   -1.80 Nm = raw 1870 = 0x74E
inline constexpr uint16_t kNagTorqueRawMax = 0x8B6;
inline constexpr uint16_t kNagTorqueRawMin = 0x74E;
inline constexpr uint8_t  kNagMaxTorqueEntries = 8;

// 0=A(스텔스 PRNG, 기본) / 1=B(스마트 상태머신: DAS AP state 게이팅 + 조향각 방향 토크)
inline constexpr uint8_t kNagModeA      = 0;
inline constexpr uint8_t kNagModeB      = 1;

struct NagConfig {
    uint8_t  mode;                              // 현재 항상 kNagModeA
    uint16_t targetId;                          // 현재 항상 kNagFixedTargetId(880)
    uint8_t  torqueCount;                       // 호환용: Mode A에서는 PRNG가 토크를 관리
    uint8_t  torqueB2[kNagMaxTorqueEntries];    // 호환용
    uint8_t  torqueB3[kNagMaxTorqueEntries];    // 호환용
    uint8_t  hoRatePct;                         // 호환용
};

// Mode A 기본값: ID 880, 토크 테이블은 스텔스 PRNG가 관리하므로 1개만 정의
inline void nagCfgDefaultsA(NagConfig &c) {
    c.mode         = kNagModeA;
    c.targetId     = kNagFixedTargetId;
    c.torqueCount  = 1;
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

