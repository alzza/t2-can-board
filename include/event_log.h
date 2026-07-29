// 밀리초 단위 CAN 이벤트 링버퍼 (A/B채널 진단용)
// 통합 로그 [5]에는 보존 통계가, 전체 행은 /api/events.csv에 저장된다.
#pragma once
#include <stdint.h>
#include <string.h>
#include "can_helpers.h"

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
    EV_NAG_MODE       = 11, // Nag mode 전환 (detail: 1=MODE1, 2=MODE2, 3=MODE3)
    EV_MODEB_STATE    = 12, // Mode B DAS hands-on state 전이 (detail: ap<<16 | old<<8 | new)
    EV_MODEB_PHASE    = 13, // Mode B phase 전이 (detail: phase<<24 | ap<<16 | ho<<8 | decision)
    EV_MODEB_FIRST_ECHO = 14, // 현재 DAS state 진입 후 첫 echo 지연(ms)
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
    case EV_MODEB_STATE: return "MODEB_STATE";
    case EV_MODEB_PHASE: return "MODEB_PHASE";
    case EV_MODEB_FIRST_ECHO: return "MODEB_FIRST_ECHO";
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
    case EV_MODEB_STATE:
    case EV_MODEB_PHASE:
    case EV_MODEB_FIRST_ECHO:
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
        return EV_SEV_WARN;
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
        return true;
    default:
        return false;
    }
}

inline uint32_t eventAggregateKey(uint8_t type, uint32_t detail) {
    // A RX 오버런 detail 상위 24비트에는 관측 당시 A 폴링 공백(us)이 들어간다.
    // EFLG 비트가 같으면 같은 30초 구간으로 묶고 최대 공백은 별도로 보존한다.
    return type == EV_A_RX_OVERRUN ? (detail & 0xFFU) : detail;
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
    case EV_MODEB_STATE: {
        uint8_t ap = static_cast<uint8_t>((detail >> 16) & 0xFF);
        uint8_t oldHo = static_cast<uint8_t>((detail >> 8) & 0xFF);
        uint8_t newHo = static_cast<uint8_t>(detail & 0xFF);
        snprintf(out, outLen, "ap=%u oldHo=0x%02X oldName=%s newHo=0x%02X newName=%s group=%s warnLevel=%u warning=%u",
                 (unsigned)ap,
                 (unsigned)oldHo, dasHandsOnStateName(oldHo),
                 (unsigned)newHo, dasHandsOnStateName(newHo), dasHandsOnStateGroup(newHo),
                 (unsigned)dasHandsOnWarningLevel(newHo), dasHandsOnStateIsWarning(newHo) ? 1U : 0U);
        break;
    }
    case EV_MODEB_PHASE: {
        uint8_t phase = static_cast<uint8_t>((detail >> 24) & 0xFF);
        uint8_t ap = static_cast<uint8_t>((detail >> 16) & 0xFF);
        uint8_t ho = static_cast<uint8_t>((detail >> 8) & 0xFF);
        uint8_t decision = static_cast<uint8_t>(detail & 0xFF);
        snprintf(out, outLen, "phase=%u ap=%u ho=0x%02X hoName=%s warnLevel=%u decision=%s",
                 (unsigned)phase, (unsigned)ap,
                 (unsigned)ho, dasHandsOnStateName(ho),
                 (unsigned)dasHandsOnWarningLevel(ho), nagDecisionName(decision));
        break;
    }
    case EV_MODEB_FIRST_ECHO:
        snprintf(out, outLen, "delay_ms=%u", (unsigned)detail);
        break;
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
                 "A_TX=%u SUMMON=%u TSLLC=%u NAG=%u ONE_SHOT=%u TX_GUARD=%u NAG_AP_ONLY=%u",
                 (unsigned)((detail >> 0) & 1U), (unsigned)((detail >> 1) & 1U),
                 (unsigned)((detail >> 2) & 1U), (unsigned)((detail >> 3) & 1U),
                 (unsigned)((detail >> 4) & 1U), (unsigned)((detail >> 5) & 1U),
                 (unsigned)((detail >> 6) & 1U));
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
        snprintf(out, outLen, "reason=%s", aTxGuardReasonName((uint8_t)detail));
        break;
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
        "schema_version,wall_time_first,wall_time_last,uptime_first_ms,uptime_last_ms,"
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
        "2,%s,%s,%u,%u,%u,%s,%s,%s,%u,%u,%u,%u,%u,%s\r\n",
        firstTime, lastTime,
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
