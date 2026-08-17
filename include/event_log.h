// 밀리초 단위 CAN 이벤트 링버퍼 (A/B채널 진단용)
// 통합 로그 [5]에는 보존 통계가, 전체 행은 /api/events.csv에 저장된다.
#pragma once
#include <stdint.h>
#include <string.h>
#include "can_helpers.h"
#include "version.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <esp_http_server.h>
#include <freertos/portmacro.h>
#include <stdio.h>
#endif

// 이벤트 타입 (CSV에 그대로 출력)
enum CanEventType : uint8_t {
    EV_BUSOFF        = 0,  // BUS-OFF 진입
    EV_RECOVERY_OK   = 1,  // 복구 성공
    EV_RECOVERY_FAIL = 2,  // 복구 실패 (하드 폴백 포함)
    EV_RECOVERY_SOFT = 3,  // 소프트 복구 시도
    EV_ALERT_ERR_PASS = 4, // TEC≥128 (Error Passive 진입)
    EV_ALERT_ARB_LOST = 5, // 중재 실패
    EV_ALERT_BUS_ERR  = 6, // 비트/스터프/CRC/폼/ACK 중 하나
    EV_ALERT_TX_FAIL  = 7, // 단일 송신 실패 (ss=true 일 때)
    EV_ALERT_RX_FULL  = 8, // RX 큐 오버플로
    EV_TX_BACKOFF     = 9, // TX 백오프 진입
    EV_USER_MARK      = 10, // 사용자가 분석 구간 시작/종료 지점을 표시
    EV_NAG_MODE       = 11, // Nag mode 전환 (detail: 1=MODE1, 2=MODE2)
    // 12~14는 제거된 실험용 Mode 3 이벤트 번호로 재사용하지 않는다.
    EV_A_EFLG_SET     = 15, // A채널 MCP2515 EFLG 0→비제로
    EV_A_EFLG_CLEAR   = 16, // A채널 MCP2515 EFLG 비제로→0
    EV_A_RX_OVERRUN   = 17, // A채널 MCP2515 RX0OVR/RX1OVR 감지
    EV_A_WAKE_FIRST_TX = 18, // A채널 재수신 시작→첫 Summon TX 성공 지연
    EV_CAPTURE_START  = 19, // 사용자가 수동 기록 시작
    EV_CAPTURE_STOP   = 20, // 사용자가 수동 기록 정지
    EV_CAPTURE_RESET  = 21, // 사용자가 기록 초기화
    EV_FEATURE_STATE  = 22, // 주요 TX/기능 플래그 변경
    EV_A_TX_GUARD_SET = 23, // A채널 TX Guard 진입
    EV_A_TX_GUARD_CLEAR = 24, // A채널 TX Guard 해제
    EV_A_SPI_TARGET = 25, // A채널 SPI 목표 속도 변경(재부팅 적용)
    EV_FEATURE_ACTIVITY = 26, // 5초 구간 기능별 실제 주입 활동 전이
    EV_A_TX_FAILURE = 27, // A채널 MCP2515 TXERR 기능/버퍼/결과 세부 기록
    EV_SUMMONING_STATE = 28, // 검증 INO 게이트의 실제 Summoning 상태 전이
    EV_NAG_INJECTION_SESSION = 29, // 실제 Nag 주입 세션 시작/종료
    EV_SUMMON_UNLOCK_ACTIVITY = 30, // bit19/46 주입 활동 시작/종료(실제 Summoning과 분리)
    EV_NAG_GATE_STATE = 31, // Nag 허용/차단 사유 밀리초 전이
    // 5초 구간에 기능별 A채널 MLOA가 과도할 때만 남기는 품질 경고.
    // CAN 송신 조건을 바꾸지 않는 관측 전용 이벤트다.
    EV_A_TX_QUALITY = 32,
    // B채널 BUS_ERR 경보가 들어온 정확한 순간의 Nag/AP 문맥 스냅샷.
    // 원본 BUS_ERR 행의 TWAI alert flag와 분리해 읽는다.
    EV_B_BUS_ERR_SNAPSHOT = 33,
    // 실제 Summoning 종료 때 해당 세션의 원본+재시도 최종 TX 결과를 기록한다.
    EV_SUMMON_TX_SESSION = 34,
    // 실제 Summoning 구간의 MLOA 단발 재시도 결과를 기록한다.
    EV_SUMMON_RETRY_SESSION = 35,
    // 실제 Summoning 구간의 연속 MLOA, 성공 공백, TSLLC 보류를 기록한다.
    EV_SUMMON_TX_TIMING = 36,
    // 실제 Summon ACA+SPR 정책의 허용/종료 사유 전이.
    EV_SUMMON_POLICY_STATE = 37,
    EV_TYPE_COUNT
};

enum CanEventChannel : uint8_t {
    EV_CH_SYSTEM = 0,
    EV_CH_A = 1,
    EV_CH_B = 2,
    EV_CH_AB = 3,
};

enum CanEventSeverity : uint8_t {
    EV_SEV_INFO = 0,
    EV_SEV_WARN = 1,
    EV_SEV_ERROR = 2,
};

static constexpr uint32_t kEventAggregateWindowMs = 30000;

// CSV/통합 번들에서 숫자 event type만 보고 해석하지 않도록 사람이 읽는 이름도 같이 출력한다.
inline const char* eventTypeName(uint8_t type) {
    switch (type) {
    case EV_BUSOFF: return "BUSOFF";
    case EV_RECOVERY_OK: return "REC_OK";
    case EV_RECOVERY_FAIL: return "REC_FAIL";
    case EV_RECOVERY_SOFT: return "REC_SOFT";
    case EV_ALERT_ERR_PASS: return "ERR_PASS";
    case EV_ALERT_ARB_LOST: return "ARB_LOST";
    case EV_ALERT_BUS_ERR: return "BUS_ERR";
    case EV_ALERT_TX_FAIL: return "TX_FAIL";
    case EV_ALERT_RX_FULL: return "RX_FULL";
    case EV_TX_BACKOFF: return "TX_BACKOFF";
    case EV_USER_MARK: return "USER_MARK";
    case EV_NAG_MODE: return "NAG_MODE";
    case EV_A_EFLG_SET: return "A_EFLG_SET";
    case EV_A_EFLG_CLEAR: return "A_EFLG_CLEAR";
    case EV_A_RX_OVERRUN: return "A_RX_OVERRUN";
    case EV_A_WAKE_FIRST_TX: return "A_WAKE_FIRST_TX";
    case EV_CAPTURE_START: return "CAPTURE_START";
    case EV_CAPTURE_STOP: return "CAPTURE_STOP";
    case EV_CAPTURE_RESET: return "CAPTURE_RESET";
    case EV_FEATURE_STATE: return "FEATURE_STATE";
    case EV_A_TX_GUARD_SET: return "A_TX_GUARD_SET";
    case EV_A_TX_GUARD_CLEAR: return "A_TX_GUARD_CLEAR";
    case EV_A_SPI_TARGET: return "A_SPI_TARGET";
    case EV_FEATURE_ACTIVITY: return "FEATURE_ACTIVITY";
    case EV_A_TX_FAILURE: return "A_TX_FAILURE";
    case EV_SUMMONING_STATE: return "SUMMONING_STATE";
    case EV_NAG_INJECTION_SESSION: return "NAG_INJECTION_SESSION";
    case EV_SUMMON_UNLOCK_ACTIVITY: return "SUMMON_UNLOCK_ACTIVITY";
    case EV_NAG_GATE_STATE: return "NAG_GATE_STATE";
    case EV_A_TX_QUALITY: return "A_TX_QUALITY";
    case EV_B_BUS_ERR_SNAPSHOT: return "B_BUS_ERR_SNAPSHOT";
    case EV_SUMMON_TX_SESSION: return "SUMMON_TX_SESSION";
    case EV_SUMMON_RETRY_SESSION: return "SUMMON_RETRY_SESSION";
    case EV_SUMMON_TX_TIMING: return "SUMMON_TX_TIMING";
    case EV_SUMMON_POLICY_STATE: return "SUMMON_POLICY_STATE";
    default: return "UNKNOWN";
    }
}

inline uint8_t eventChannel(uint8_t type) {
    switch (type) {
    case EV_A_EFLG_SET:
    case EV_A_EFLG_CLEAR:
    case EV_A_RX_OVERRUN:
    case EV_A_WAKE_FIRST_TX:
    case EV_A_TX_GUARD_SET:
    case EV_A_TX_GUARD_CLEAR:
    case EV_A_SPI_TARGET:
    case EV_A_TX_FAILURE:
    case EV_A_TX_QUALITY:
    case EV_SUMMONING_STATE:
    case EV_SUMMON_UNLOCK_ACTIVITY:
    case EV_SUMMON_TX_SESSION:
    case EV_SUMMON_RETRY_SESSION:
    case EV_SUMMON_TX_TIMING:
    case EV_SUMMON_POLICY_STATE:
        return EV_CH_A;
    case EV_BUSOFF:
    case EV_RECOVERY_OK:
    case EV_RECOVERY_FAIL:
    case EV_RECOVERY_SOFT:
    case EV_ALERT_ERR_PASS:
    case EV_ALERT_ARB_LOST:
    case EV_ALERT_BUS_ERR:
    case EV_ALERT_TX_FAIL:
    case EV_ALERT_RX_FULL:
    case EV_TX_BACKOFF:
    case EV_NAG_MODE:
    case EV_NAG_INJECTION_SESSION:
    case EV_NAG_GATE_STATE:
    case EV_B_BUS_ERR_SNAPSHOT:
        return EV_CH_B;
    case EV_USER_MARK:
    case EV_FEATURE_ACTIVITY:
        return EV_CH_AB;
    default:
        return EV_CH_SYSTEM;
    }
}

inline const char* eventChannelName(uint8_t channel) {
    switch (channel) {
    case EV_CH_A: return "A";
    case EV_CH_B: return "B";
    case EV_CH_AB: return "A/B";
    default: return "SYSTEM";
    }
}

inline uint8_t eventSeverity(uint8_t type, uint32_t detail) {
    switch (type) {
    case EV_BUSOFF:
    case EV_RECOVERY_FAIL:
    case EV_ALERT_ERR_PASS:
    case EV_ALERT_BUS_ERR:
    case EV_ALERT_TX_FAIL:
    case EV_ALERT_RX_FULL:
        return EV_SEV_ERROR;
    case EV_A_EFLG_SET:
        return (detail & 0x20U) ? EV_SEV_ERROR : EV_SEV_WARN;
    case EV_A_RX_OVERRUN:
    case EV_A_TX_GUARD_SET:
    case EV_A_TX_FAILURE:
        return EV_SEV_WARN;
    case EV_SUMMON_POLICY_STATE:
        return ((detail & 0x0FU) == SUMMON_SESSION_IDLE ||
                (detail & 0x0FU) == SUMMON_SESSION_ALLOWED)
                   ? EV_SEV_INFO : EV_SEV_WARN;
    case EV_A_TX_QUALITY: {
        // MLOA만 높고 완료 프레임이 존재하면 같은 ID의 정상 중재 경쟁이다.
        // 완료가 전혀 없거나 ABTF가 함께 있을 때만 운용 경고로 올린다.
        const uint32_t completed = (detail >> 2) & 0xFFU;
        const uint32_t aborted = (detail >> 18) & 0xFFU;
        return (completed == 0U || aborted > 0U) ? EV_SEV_WARN : EV_SEV_INFO;
    }
    default:
        return EV_SEV_INFO;
    }
}

inline const char* eventSeverityName(uint8_t severity) {
    switch (severity) {
    case EV_SEV_WARN: return "WARN";
    case EV_SEV_ERROR: return "ERROR";
    default: return "INFO";
    }
}

inline bool eventTypeIsAggregated(uint8_t type) {
    switch (type) {
    case EV_ALERT_ARB_LOST:
    case EV_ALERT_BUS_ERR:
    case EV_ALERT_TX_FAIL:
    case EV_ALERT_RX_FULL:
    case EV_A_EFLG_SET:
    case EV_A_EFLG_CLEAR:
    case EV_A_RX_OVERRUN:
    case EV_A_TX_FAILURE:
    case EV_A_TX_QUALITY:
    case EV_B_BUS_ERR_SNAPSHOT:
    // Hands-on/AP 차단은 수백 ms 단위로 왕복할 수 있다. 개별 전이를 모두
    // 보관하면 256행 이벤트 링이 먼저 소진되므로 30초 구간으로 묶는다.
    // 실제 Nag 주입 세션의 START/END는 별도 이벤트라 묶지 않는다.
    case EV_NAG_GATE_STATE:
        return true;
    default:
        return false;
    }
}

inline uint32_t eventAggregateKey(uint8_t type, uint32_t detail) {
    // A RX 오버런 detail 상위 24비트에는 관측 당시 A 폴링 공백(us)이 들어간다.
    // EFLG 비트가 같으면 같은 30초 구간으로 묶고 최대 공백은 별도로 보존한다.
    if (type == EV_A_RX_OVERRUN) return detail & 0xFFU;
    // 게이트 상태가 HANDS_ON <-> AP_BLOCK처럼 반복 전이해도 같은 구간으로
    // 합산한다. 최신 detail은 eventLogPushAt에서 보존한다.
    if (type == EV_NAG_GATE_STATE || type == EV_B_BUS_ERR_SNAPSHOT) return type;
    // 기능별 품질 관측은 출처와 심각도별로 30초간 합산한다. INFO 중재 경쟁이
    // 완료 0건/ABTF 동반 WARN을 덮지 않도록 심각도도 집계 키에 포함한다.
    if (type == EV_A_TX_QUALITY)
        return (detail & 0x03U) | ((uint32_t)eventSeverity(type, detail) << 2);
    return detail;
}

inline uint32_t eventFeatureStateDetail() {
    uint32_t detail = 0;
    if ((bool)aChannelTxRuntime) detail |= 1U << 0;
    if ((bool)summonUnlockRuntime) detail |= 1U << 1;
    if ((bool)tsllcRuntime) detail |= 1U << 2;
    if ((bool)nagKillerRuntime) detail |= 1U << 3;
    if ((bool)aMcpOneShotRuntime) detail |= 1U << 4;
    if ((bool)aTxGuardRuntime) detail |= 1U << 5;
    if ((bool)nagApOnlyRuntime) detail |= 1U << 6;
    if ((bool)summonConditionLimitRuntime) detail |= 1U << 7;
    return detail;
}

inline uint32_t eventFeatureActivityDetail(bool summonTx, bool tsllcTx, bool nagTx,
                                           bool summonGate, bool aGuard,
                                           bool apActive)
{
    uint32_t detail = 0;
    if (summonTx) detail |= 1U << 0;
    if (tsllcTx) detail |= 1U << 1;
    if (nagTx) detail |= 1U << 2;
    if (summonGate) detail |= 1U << 3;
    if (aGuard) detail |= 1U << 4;
    if (apActive) detail |= 1U << 5;
    if ((bool)nagApOnlyRuntime) detail |= 1U << 6;
    return detail;
}

// source: 0=OTHER, 1=SUMMON, 2=TSLLC
// polledResult: 0=sendMessage 직후 결과, 1=TX 버퍼 완료 폴링 결과
// buffer: 실제 TXB0~2. (3은 버퍼를 특정하지 못한 레거시 기록용)
// ctrl: 실패 순간 TXBnCTRL 스냅샷, driverCode: 즉시 결과의 MCP 오류 코드
inline uint32_t eventATxFailureDetail(uint8_t source, bool polledResult, uint8_t buffer,
                                      uint8_t ctrl, uint8_t driverCode)
{
    return (uint32_t)(source & 0x03U) |
           (polledResult ? (1U << 2) : 0U) |
           ((uint32_t)(buffer & 0x03U) << 3) |
           ((uint32_t)ctrl << 8) |
           ((uint32_t)driverCode << 16);
}

// 5초 시계열 구간의 기능별 A채널 송신 결과. 모든 값은 255에서 포화한다.
// source: 1=SUMMON, 2=TSLLC. 중재 손실 비율은 completed+MLOA+ABTF 대비 MLOA다.
inline uint32_t eventATxQualityDetail(uint8_t source, uint32_t completed,
                                      uint32_t arbitrationLost, uint32_t aborted)
{
    const uint32_t completed8 = completed > 255U ? 255U : completed;
    const uint32_t lost8 = arbitrationLost > 255U ? 255U : arbitrationLost;
    const uint32_t aborted8 = aborted > 255U ? 255U : aborted;
    return (uint32_t)(source & 0x03U) |
           (completed8 << 2) |
           (lost8 << 10) |
           (aborted8 << 18);
}

inline uint8_t eventATxQualityMloaPercent(uint32_t completed, uint32_t arbitrationLost,
                                          uint32_t aborted)
{
    const uint32_t attempts = completed + arbitrationLost + aborted;
    if (attempts == 0U) return 0;
    return (uint8_t)((arbitrationLost * 100U) / attempts);
}

inline bool eventATxQualityIsWarning(uint32_t completed, uint32_t arbitrationLost,
                                     uint32_t aborted)
{
    const uint32_t attempts = completed + arbitrationLost + aborted;
    return attempts >= 10U && eventATxQualityMloaPercent(completed, arbitrationLost, aborted) >= 50U;
}

// BUS_ERR 직전/직후에 추정하지 않고 alert 폴링 순간의 상태를 압축해 보관한다.
inline uint32_t eventBBusErrSnapshotDetail(uint8_t twaiState, uint16_t tec, uint16_t rec,
                                           uint8_t nagMode, bool nagEnabled, bool apActive,
                                           uint8_t handsOn, uint8_t decision, uint8_t dasState)
{
    return (uint32_t)(twaiState & 0x03U) |
           ((uint32_t)(tec & 0xFFU) << 2) |
           ((uint32_t)(rec & 0xFFU) << 10) |
           ((uint32_t)(nagMode & 0x03U) << 18) |
           (nagEnabled ? (1U << 20) : 0U) |
           (apActive ? (1U << 21) : 0U) |
           ((uint32_t)(handsOn & 0x03U) << 22) |
           ((uint32_t)(decision & 0x0FU) << 24) |
           ((uint32_t)(dasState & 0x0FU) << 28);
}

// Summoning 상태와 해당 세션의 A채널 결과를 작은 이벤트 detail에 함께 보관한다.
// count는 세션 범위에서 255로 포화한다. active=1은 START, 0은 END다.
inline uint32_t eventSummoningStateDetail(bool active, bool parked, bool aca, bool spr,
                                          bool gateOpen, uint8_t gateReason,
                                          uint32_t txOk, uint32_t txFail,
                                          uint32_t blocked)
{
    const uint32_t txOk8 = txOk > 255U ? 255U : txOk;
    const uint32_t txFail8 = txFail > 255U ? 255U : txFail;
    const uint32_t blocked8 = blocked > 255U ? 255U : blocked;
    return (active ? 1U : 0U) |
           (parked ? (1U << 1) : 0U) |
           (aca ? (1U << 2) : 0U) |
           (spr ? (1U << 3) : 0U) |
           (gateOpen ? (1U << 4) : 0U) |
           ((uint32_t)(gateReason & 0x07U) << 5) |
           (txOk8 << 8) |
           (txFail8 << 16) |
           (blocked8 << 24);
}

// 실제 Summoning 세션의 최종 MCP2515 결과. 각 값은 255에서 포화한다.
inline uint32_t eventSummonTxSessionDetail(uint32_t completed,
                                           uint32_t arbitrationLost,
                                           uint32_t aborted,
                                           uint32_t error)
{
    const uint32_t completed8 = completed > 255U ? 255U : completed;
    const uint32_t lost8 = arbitrationLost > 255U ? 255U : arbitrationLost;
    const uint32_t aborted8 = aborted > 255U ? 255U : aborted;
    const uint32_t error8 = error > 255U ? 255U : error;
    return completed8 | (lost8 << 8) | (aborted8 << 16) | (error8 << 24);
}

// actual Summoning에서만 허용한 One-shot MLOA 단발 재시도 세션 결과.
// discarded는 만료·새 프레임·게이트·Guard·OTA 취소와 재시도 하드 실패 합계다.
inline uint32_t eventSummonRetrySessionDetail(uint32_t scheduled,
                                              uint32_t completed,
                                              uint32_t arbitrationLost,
                                              uint32_t discarded)
{
    const uint32_t scheduled8 = scheduled > 255U ? 255U : scheduled;
    const uint32_t completed8 = completed > 255U ? 255U : completed;
    const uint32_t lost8 = arbitrationLost > 255U ? 255U : arbitrationLost;
    const uint32_t discarded8 = discarded > 255U ? 255U : discarded;
    return scheduled8 | (completed8 << 8) | (lost8 << 16) | (discarded8 << 24);
}

// maxGapMs는 16비트(65.535초)에서 포화하고 나머지는 8비트에서 포화한다.
inline uint32_t eventSummonTxTimingDetail(uint32_t maxMloaStreak,
                                         uint32_t maxGapMs,
                                         uint32_t tsllcSuppressed)
{
    const uint32_t streak8 = maxMloaStreak > 255U ? 255U : maxMloaStreak;
    const uint32_t gap16 = maxGapMs > 65535U ? 65535U : maxGapMs;
    const uint32_t held8 = tsllcSuppressed > 255U ? 255U : tsllcSuppressed;
    return streak8 | (gap16 << 8) | (held8 << 24);
}

inline uint32_t eventSummonPolicyStateDetail(uint8_t reason, bool allowed,
                                             bool aca, bool spr, uint8_t diGear,
                                             uint8_t secondaryGear,
                                             uint16_t vehicleSpeedRaw)
{
    return (uint32_t)(reason & 0x0FU) |
           (allowed ? (1U << 4) : 0U) |
           (aca ? (1U << 5) : 0U) |
           (spr ? (1U << 6) : 0U) |
           ((uint32_t)(diGear & 0x07U) << 7) |
           ((uint32_t)(secondaryGear & 0x07U) << 10) |
           ((uint32_t)(vehicleSpeedRaw & 0x0FFFU) << 13);
}

// Nag 실제 주입 세션. active=1은 START, 0은 END, injections는 세션 누적값이다.
inline uint32_t eventNagInjectionSessionDetail(bool active, uint8_t mode, bool apActive,
                                               uint8_t phase, uint8_t decision,
                                               uint32_t injections)
{
    const uint32_t injections16 = injections > 65535U ? 65535U : injections;
    return (active ? 1U : 0U) |
           ((uint32_t)(mode & 0x03U) << 1) |
           (apActive ? (1U << 3) : 0U) |
           ((uint32_t)(phase & 0x0FU) << 4) |
           ((uint32_t)decision << 8) |
           (injections16 << 16);
}

// Summon Unlock 주입 활동 구간. queued/completed/arbitrationLost는 세션 범위에서
// 255로 포화한다. 실제 차량 Summoning은 EV_SUMMONING_STATE로 별도 기록한다.
inline uint32_t eventSummonUnlockActivityDetail(bool active, bool parked, bool apActive,
                                                bool summoning, uint8_t gateReason,
                                                uint32_t queued, uint32_t completed,
                                                uint32_t arbitrationLost)
{
    const uint32_t queued8 = queued > 255U ? 255U : queued;
    const uint32_t completed8 = completed > 255U ? 255U : completed;
    const uint32_t lost8 = arbitrationLost > 255U ? 255U : arbitrationLost;
    return (active ? 1U : 0U) |
           (parked ? (1U << 1) : 0U) |
           (apActive ? (1U << 2) : 0U) |
           (summoning ? (1U << 3) : 0U) |
           ((uint32_t)(gateReason & 0x07U) << 4) |
           (queued8 << 8) |
           (completed8 << 16) |
           (lost8 << 24);
}

inline uint32_t eventNagGateStateDetail(uint8_t decision, uint8_t mode, bool apActive,
                                        uint8_t handsOn, uint8_t dasState,
                                        uint16_t dasSource, uint8_t phase)
{
    return (uint32_t)(decision & 0x0FU) |
           ((uint32_t)(mode & 0x03U) << 4) |
           (apActive ? (1U << 6) : 0U) |
           ((uint32_t)(handsOn & 0x03U) << 7) |
           ((uint32_t)(dasState & 0x0FU) << 9) |
           ((uint32_t)(dasSource & 0x07FFU) << 13) |
           ((uint32_t)phase << 24);
}

#ifndef NATIVE_BUILD
inline const char* eventDetailText(uint8_t type, uint32_t detail, char* out, size_t outLen) {
    if (!out || outLen == 0) return "";
    out[0] = '\0';
    switch (type) {
    case EV_BUSOFF:
        snprintf(out, outLen, "busoff_sequence=%u", (unsigned)detail);
        break;
    case EV_RECOVERY_SOFT:
        snprintf(out, outLen, "recovery_attempt=%u", (unsigned)detail);
        break;
    case EV_RECOVERY_OK:
        snprintf(out, outLen, "recovery_duration_ms=%u", (unsigned)detail);
        break;
    case EV_RECOVERY_FAIL:
        snprintf(out, outLen, "recovery_fail_count=%u", (unsigned)detail);
        break;
    case EV_ALERT_ERR_PASS:
    case EV_ALERT_ARB_LOST:
    case EV_ALERT_BUS_ERR:
    case EV_ALERT_TX_FAIL:
    case EV_ALERT_RX_FULL:
        snprintf(out, outLen, "twai_alert_flags=0x%08X", (unsigned)detail);
        break;
    case EV_USER_MARK:
        snprintf(out, outLen, "marker=%s", userMarkerDetailName(detail));
        break;
    case EV_NAG_MODE: {
        uint8_t mode = nagModeClamp(static_cast<uint8_t>(detail));
        snprintf(out, outLen, "mode=%u label=%s", (unsigned)mode, nagModeName(mode));
        break;
    }
    case EV_A_EFLG_SET:
    case EV_A_EFLG_CLEAR:
        snprintf(out, outLen,
                 "eflg=0x%02X state=%s RX1OVR=%u RX0OVR=%u TXBO=%u TXEP=%u RXEP=%u TXWAR=%u RXWAR=%u EWARN=%u",
                 (unsigned)(detail & 0xFFU), aMcpEflgStateName((uint8_t)detail),
                 (unsigned)((detail >> 7) & 1U), (unsigned)((detail >> 6) & 1U),
                 (unsigned)((detail >> 5) & 1U), (unsigned)((detail >> 4) & 1U),
                 (unsigned)((detail >> 3) & 1U), (unsigned)((detail >> 2) & 1U),
                 (unsigned)((detail >> 1) & 1U), (unsigned)(detail & 1U));
        break;
    case EV_A_RX_OVERRUN: {
        const uint8_t eflg = (uint8_t)(detail & 0xFFU);
        const uint32_t loopGapUs = detail >> 8;
        snprintf(out, outLen,
                 "eflg=0x%02X state=%s RX1OVR=%u RX0OVR=%u loop_gap_us=%u",
                 (unsigned)eflg, aMcpEflgStateName(eflg),
                 (unsigned)((eflg >> 7) & 1U), (unsigned)((eflg >> 6) & 1U),
                 (unsigned)loopGapUs);
        break;
    }
    case EV_A_WAKE_FIRST_TX:
        snprintf(out, outLen, "wake_to_summon_tx_ms=%u", (unsigned)detail);
        break;
    case EV_CAPTURE_START:
        snprintf(out, outLen, "manual_capture=START");
        break;
    case EV_CAPTURE_STOP:
        snprintf(out, outLen, "manual_capture=STOP");
        break;
    case EV_CAPTURE_RESET:
        snprintf(out, outLen, "capture=RESET");
        break;
    case EV_FEATURE_STATE:
        snprintf(out, outLen,
                 "A_TX=%u EAP_SUMMON=%u TSLLC=%u NAG=%u ONE_SHOT=%u TX_GUARD=%u NAG_AP_ONLY=%u SUMMON_CONDITION_LIMIT=%u SUMMON_POLICY=%s",
                 (unsigned)((detail >> 0) & 1U), (unsigned)((detail >> 1) & 1U),
                 (unsigned)((detail >> 2) & 1U), (unsigned)((detail >> 3) & 1U),
                 (unsigned)((detail >> 4) & 1U), (unsigned)((detail >> 5) & 1U),
                 (unsigned)((detail >> 6) & 1U), (unsigned)((detail >> 7) & 1U),
                 kSummonPolicyName);
        break;
    case EV_FEATURE_ACTIVITY:
        snprintf(out, outLen,
                 "SUMMON_TX=%u TSLLC_TX=%u NAG_TX=%u SUMMON_GATE=%u A_GUARD=%u AP_ACTIVE=%u NAG_AP_ONLY=%u",
                 (unsigned)((detail >> 0) & 1U), (unsigned)((detail >> 1) & 1U),
                 (unsigned)((detail >> 2) & 1U), (unsigned)((detail >> 3) & 1U),
                 (unsigned)((detail >> 4) & 1U), (unsigned)((detail >> 5) & 1U),
                 (unsigned)((detail >> 6) & 1U));
        break;
    case EV_A_TX_GUARD_SET:
    case EV_A_TX_GUARD_CLEAR:
        snprintf(out, outLen, "reason=%s trigger=%s",
                 aTxGuardReasonName((uint8_t)detail),
                 aTxSourceMaskName((uint8_t)(detail >> 8)));
        break;
    case EV_A_TX_FAILURE: {
        const uint8_t source = (uint8_t)(detail & 0x03U);
        const bool polledResult = (detail & (1U << 2)) != 0;
        const uint8_t buffer = (uint8_t)((detail >> 3) & 0x03U);
        const uint8_t ctrl = (uint8_t)((detail >> 8) & 0xFFU);
        const uint8_t driverCode = (uint8_t)((detail >> 16) & 0xFFU);
        snprintf(out, outLen,
                 "source=%s phase=%s tx_buffer=TXB%u ctrl=0x%02X TXERR=%u MLOA=%u ABTF=%u driver_error=%u",
                 aTxSourceName(source), polledResult ? "POLL_RESULT" : "IMMEDIATE_RESULT",
                 (unsigned)buffer, (unsigned)ctrl,
                 (unsigned)((ctrl >> 4) & 1U), (unsigned)((ctrl >> 5) & 1U),
                 (unsigned)((ctrl >> 6) & 1U), (unsigned)driverCode);
        break;
    }
    case EV_A_TX_QUALITY: {
        const uint8_t source = (uint8_t)(detail & 0x03U);
        const uint8_t completed = (uint8_t)((detail >> 2) & 0xFFU);
        const uint8_t lost = (uint8_t)((detail >> 10) & 0xFFU);
        const uint8_t aborted = (uint8_t)((detail >> 18) & 0xFFU);
        snprintf(out, outLen,
                 "source=%s window=5s completed=%u arbitration_lost=%u aborted=%u mloa_pct=%u threshold=attempts>=10,mloa>=50%%",
                 aTxSourceName(source), (unsigned)completed, (unsigned)lost,
                 (unsigned)aborted,
                 (unsigned)eventATxQualityMloaPercent(completed, lost, aborted));
        break;
    }
    case EV_B_BUS_ERR_SNAPSHOT: {
        const uint8_t twaiState = (uint8_t)(detail & 0x03U);
        const uint8_t tec = (uint8_t)((detail >> 2) & 0xFFU);
        const uint8_t rec = (uint8_t)((detail >> 10) & 0xFFU);
        const uint8_t mode = (uint8_t)((detail >> 18) & 0x03U);
        const uint8_t handsOn = (uint8_t)((detail >> 22) & 0x03U);
        const uint8_t decision = (uint8_t)((detail >> 24) & 0x0FU);
        const uint8_t dasState = (uint8_t)((detail >> 28) & 0x0FU);
        snprintf(out, outLen,
                 "twai_state=%u tec=%u rec=%u nag_mode=%u nag_enabled=%u ap_active=%u hands_on=%u decision=%s das=%u",
                 (unsigned)twaiState, (unsigned)tec, (unsigned)rec, (unsigned)mode,
                 (unsigned)((detail >> 20) & 1U), (unsigned)((detail >> 21) & 1U),
                 (unsigned)handsOn, nagDecisionName(decision), (unsigned)dasState);
        break;
    }
    case EV_SUMMONING_STATE: {
        const bool active = (detail & 1U) != 0;
        const bool parked = (detail & (1U << 1)) != 0;
        const bool aca = (detail & (1U << 2)) != 0;
        const bool spr = (detail & (1U << 3)) != 0;
        const bool gateOpen = (detail & (1U << 4)) != 0;
        const uint8_t reason = (uint8_t)((detail >> 5) & 0x07U);
        snprintf(out, outLen,
                 "state=%s gate=%s open=%u parked=%u aca=%u spr=%u tx_ok=%u tx_fail=%u blocked=%u",
                 active ? "START" : "END", summonGateReasonNameFromCode(reason),
                 (unsigned)gateOpen, (unsigned)parked, (unsigned)aca, (unsigned)spr,
                 (unsigned)((detail >> 8) & 0xFFU), (unsigned)((detail >> 16) & 0xFFU),
                 (unsigned)((detail >> 24) & 0xFFU));
        break;
    }
    case EV_SUMMON_TX_SESSION:
        snprintf(out, outLen,
                 "completed=%u arbitration_lost=%u aborted=%u error=%u",
                 (unsigned)(detail & 0xFFU), (unsigned)((detail >> 8) & 0xFFU),
                 (unsigned)((detail >> 16) & 0xFFU),
                 (unsigned)((detail >> 24) & 0xFFU));
        break;
    case EV_SUMMON_RETRY_SESSION:
        snprintf(out, outLen,
                 "scheduled=%u completed=%u arbitration_lost=%u discarded=%u delay_ms=%u expiry_ms=%u",
                 (unsigned)(detail & 0xFFU), (unsigned)((detail >> 8) & 0xFFU),
                 (unsigned)((detail >> 16) & 0xFFU),
                 (unsigned)((detail >> 24) & 0xFFU),
                 (unsigned)kSummonRetryDelayMs, (unsigned)kSummonRetryExpiryMs);
        break;
    case EV_SUMMON_TX_TIMING:
        snprintf(out, outLen,
                 "max_consecutive_mloa=%u max_success_gap_ms=%u tsllc_held=%u",
                 (unsigned)(detail & 0xFFU),
                 (unsigned)((detail >> 8) & 0xFFFFU),
                 (unsigned)((detail >> 24) & 0xFFU));
        break;
    case EV_SUMMON_POLICY_STATE:
        snprintf(out, outLen,
                 "policy=%s reason=%s allowed=%u aca=%u spr=%u di_gear=%u secondary_gear=%u speed_validation=0 speed_raw_sna=%u",
                 kSummonPolicyName,
                 summonSessionReasonName((uint8_t)(detail & 0x0FU)),
                 (unsigned)((detail >> 4) & 1U),
                 (unsigned)((detail >> 5) & 1U),
                 (unsigned)((detail >> 6) & 1U),
                 (unsigned)((detail >> 7) & 0x07U),
                 (unsigned)((detail >> 10) & 0x07U),
                 (unsigned)((detail >> 13) & 0x0FFFU));
        break;
    case EV_NAG_INJECTION_SESSION:
        snprintf(out, outLen,
                 "state=%s mode=%u ap_active=%u phase=%u decision=%s injections=%u",
                 (detail & 1U) ? "START" : "END", (unsigned)((detail >> 1) & 0x03U),
                 (unsigned)((detail >> 3) & 1U), (unsigned)((detail >> 4) & 0x0FU),
                 nagDecisionName((uint8_t)((detail >> 8) & 0xFFU)),
                 (unsigned)((detail >> 16) & 0xFFFFU));
        break;
    case EV_SUMMON_UNLOCK_ACTIVITY: {
        const uint8_t reason = (uint8_t)((detail >> 4) & 0x07U);
        snprintf(out, outLen,
                 "state=%s gate=%s parked=%u ap_active=%u summoning=%u queued=%u completed=%u arbitration_lost=%u",
                 (detail & 1U) ? "START" : "END", summonGateReasonNameFromCode(reason),
                 (unsigned)((detail >> 1) & 1U), (unsigned)((detail >> 2) & 1U),
                 (unsigned)((detail >> 3) & 1U), (unsigned)((detail >> 8) & 0xFFU),
                 (unsigned)((detail >> 16) & 0xFFU),
                 (unsigned)((detail >> 24) & 0xFFU));
        break;
    }
    case EV_NAG_GATE_STATE: {
        const uint8_t decision = (uint8_t)(detail & 0x0FU);
        snprintf(out, outLen,
                 "gate=%s decision=%s mode=%u ap_active=%u hands_on=%u das=%u source=%u phase=%u",
                 nagGateClassName(nagDecisionGateClass(decision)), nagDecisionName(decision),
                 (unsigned)((detail >> 4) & 0x03U), (unsigned)((detail >> 6) & 1U),
                 (unsigned)((detail >> 7) & 0x03U), (unsigned)((detail >> 9) & 0x0FU),
                 (unsigned)((detail >> 13) & 0x07FFU),
                 (unsigned)((detail >> 24) & 0xFFU));
        break;
    }
    case EV_A_SPI_TARGET:
        snprintf(out, outLen, "spi_target_mhz=%u reboot_required=1", (unsigned)detail);
        break;
    default:
        snprintf(out, outLen, "raw=%u", (unsigned)detail);
        break;
    }
    return out;
}
#endif

struct CanEvent {
    uint32_t t_ms;       // 최초 발생 시각
    uint32_t last_ms;    // 묶음 내 최종 발생 시각
    uint32_t sequence;   // 고유 이벤트 레코드 순번
    uint32_t occurrences;// 동일 이벤트가 묶인 횟수
    uint8_t  type;
    uint16_t tec;
    uint16_t rec;
    uint32_t detail;   // alert raw / last id / 추가 정보
};

static constexpr size_t EVT_CAP = 256;
inline CanEvent evtBuf[EVT_CAP];
inline volatile size_t evtHead = 0;
inline volatile size_t evtCount = 0;
inline volatile uint32_t evtSequence = 0;
inline volatile uint32_t evtOccurrenceTotal = 0;
inline volatile uint32_t evtCoalescedTotal = 0;
inline volatile uint32_t evtOverwrittenTotal = 0;
inline uint32_t evtLastAggregateSequence[EV_TYPE_COUNT] = {};
#ifndef NATIVE_BUILD
inline portMUX_TYPE evtMux = portMUX_INITIALIZER_UNLOCKED;
#endif

inline void eventLogPushAt(uint32_t now, uint8_t type, uint16_t tec, uint16_t rec, uint32_t detail) {
    if (type >= EV_TYPE_COUNT) return;
#ifndef NATIVE_BUILD
    portENTER_CRITICAL(&evtMux);
#endif
    evtOccurrenceTotal++;
    if (eventTypeIsAggregated(type)) {
        const uint32_t lastSequence = evtLastAggregateSequence[type];
        if (lastSequence > 0 && evtSequence >= lastSequence &&
            evtSequence - lastSequence < EVT_CAP) {
            CanEvent& previous = evtBuf[(lastSequence - 1U) % EVT_CAP];
            if (previous.sequence == lastSequence && previous.type == type &&
                eventAggregateKey(type, previous.detail) == eventAggregateKey(type, detail) &&
                (uint32_t)(now - previous.t_ms) <= kEventAggregateWindowMs) {
                previous.last_ms = now;
                previous.tec = tec;
                previous.rec = rec;
                if (type == EV_A_RX_OVERRUN &&
                    (detail >> 8) > (previous.detail >> 8)) {
                    previous.detail = detail;
                }
                // 묶음의 마지막 게이트 상태가 CSV의 마지막 시각과 일치하도록
                // 최신 detail을 남긴다. occurrences가 구간 내 전이 횟수다.
                if (type == EV_NAG_GATE_STATE || type == EV_A_TX_QUALITY ||
                    type == EV_B_BUS_ERR_SNAPSHOT) previous.detail = detail;
                previous.occurrences++;
                evtCoalescedTotal++;
#ifndef NATIVE_BUILD
                portEXIT_CRITICAL(&evtMux);
#endif
                return;
            }
        }
    }

    CanEvent& e = evtBuf[evtHead];
    evtSequence++;
    e.t_ms = now;
    e.last_ms = now;
    e.sequence = evtSequence;
    e.occurrences = 1;
    e.type = type;
    e.tec = tec;
    e.rec = rec;
    e.detail = detail;
    if (eventTypeIsAggregated(type)) evtLastAggregateSequence[type] = e.sequence;
    evtHead = (evtHead + 1) % EVT_CAP;
    if (evtCount < EVT_CAP) {
        ++evtCount;
    } else {
        ++evtOverwrittenTotal;
    }
#ifndef NATIVE_BUILD
    portEXIT_CRITICAL(&evtMux);
#endif
}

inline void eventLogPush(uint8_t type, uint16_t tec, uint16_t rec, uint32_t detail) {
#ifndef NATIVE_BUILD
    eventLogPushAt(millis(), type, tec, rec, detail);
#else
    eventLogPushAt(evtOccurrenceTotal + 1U, type, tec, rec, detail);
#endif
}

inline void eventLogReset() {
#ifndef NATIVE_BUILD
    portENTER_CRITICAL(&evtMux);
#endif
    evtHead = 0;
    evtCount = 0;
    evtSequence = 0;
    evtOccurrenceTotal = 0;
    evtCoalescedTotal = 0;
    evtOverwrittenTotal = 0;
    memset(evtLastAggregateSequence, 0, sizeof(evtLastAggregateSequence));
#ifndef NATIVE_BUILD
    portEXIT_CRITICAL(&evtMux);
#endif
}

#ifndef NATIVE_BUILD
inline void eventLogSnapshot(size_t& count, size_t& head) {
    portENTER_CRITICAL(&evtMux);
    count = evtCount;
    head = evtHead;
    portEXIT_CRITICAL(&evtMux);
}

inline void eventLogCopyAt(size_t idx, CanEvent& out) {
    portENTER_CRITICAL(&evtMux);
    out = evtBuf[idx % EVT_CAP];
    portEXIT_CRITICAL(&evtMux);
}

using EventTimeFormatter = void(*)(uint32_t, char*, size_t);
inline EventTimeFormatter eventTimeFormatter = nullptr;

inline void eventLogFormatTime(uint32_t timestampMs, char *out, size_t outLen) {
    if (eventTimeFormatter) {
        eventTimeFormatter(timestampMs, out, outLen);
    } else {
        snprintf(out, outLen, "uptime:%u", (unsigned)timestampMs);
    }
}

inline void eventLogCsvEscape(const char *in, char *out, size_t outLen) {
    if (!out || outLen < 3) return;
    size_t j = 0;
    out[j++] = '"';
    if (in) {
        for (size_t i = 0; in[i] && j + 2 < outLen; ++i) {
            if (in[i] == '"') out[j++] = '"';
            out[j++] = (in[i] == '\r' || in[i] == '\n') ? ' ' : in[i];
        }
    }
    out[j++] = '"';
    out[j] = '\0';
}

inline void eventLogCsvHeader(httpd_req_t* req) {
    httpd_resp_sendstr_chunk(req,
        "schema_version,firmware_version,firmware_build_id,wall_time_first,wall_time_last,uptime_first_ms,uptime_last_ms,"
        "sequence,channel,severity,event,type,occurrences,tec,rec,detail,detail_text\r\n");
}

inline esp_err_t eventLogCsvRow(httpd_req_t* req, const CanEvent& e) {
    char firstTime[40];
    char lastTime[40];
    char detailText[192];
    char detailCsv[392];
    char line[768];
    eventLogFormatTime(e.t_ms, firstTime, sizeof(firstTime));
    eventLogFormatTime(e.last_ms, lastTime, sizeof(lastTime));
    eventLogCsvEscape(
        eventDetailText(e.type, e.detail, detailText, sizeof(detailText)),
        detailCsv, sizeof(detailCsv));
    snprintf(line, sizeof(line),
        "2,%s,%s,%s,%s,%u,%u,%u,%s,%s,%s,%u,%u,%u,%u,%u,%s\r\n",
        FIRMWARE_VERSION, FIRMWARE_BUILD_ID, firstTime, lastTime,
        (unsigned)e.t_ms, (unsigned)e.last_ms, (unsigned)e.sequence,
        eventChannelName(eventChannel(e.type)),
        eventSeverityName(eventSeverity(e.type, e.detail)),
        eventTypeName(e.type), (unsigned)e.type, (unsigned)e.occurrences,
        (unsigned)e.tec, (unsigned)e.rec, (unsigned)e.detail, detailCsv);
    return httpd_resp_sendstr_chunk(req, line);
}

inline esp_err_t eventLogCsvHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/csv; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"can_events.csv\"");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    size_t n = 0;
    size_t head = 0;
    eventLogSnapshot(n, head);
    httpd_resp_sendstr_chunk(req, "\xEF\xBB\xBF");
    eventLogCsvHeader(req);
    size_t start = (n < EVT_CAP) ? 0 : head;
    for (size_t i = 0; i < n; ++i) {
        CanEvent e;
        eventLogCopyAt(start + i, e);
        if (eventLogCsvRow(req, e) != ESP_OK) return ESP_FAIL;
    }
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
#endif
