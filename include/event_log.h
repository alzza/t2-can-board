// 밀리초 단위 CAN 이벤트 링버퍼 (A/B채널 진단용)
// 통합 로그 [5] 섹션에 포함되며, /api/events.csv는 디버그용 보조 엔드포인트다.
// 외과적 추가: 기존 구조 변경 없음
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
    EV_USER_MARK      = 10, // 사용자가 경고 발생 시점 표시 버튼을 누름
    EV_NAG_MODE       = 11, // Nag mode 전환 (detail: 1=MODE1, 2=MODE2, 3=MODE3)
    EV_MODEB_STATE    = 12, // Mode B DAS hands-on state 전이 (detail: ap<<16 | old<<8 | new)
    EV_MODEB_PHASE    = 13, // Mode B phase 전이 (detail: phase<<24 | ap<<16 | ho<<8 | decision)
    EV_MODEB_FIRST_ECHO = 14, // 현재 DAS state 진입 후 첫 echo 지연(ms)
    EV_A_EFLG_SET     = 15, // A채널 MCP2515 EFLG 0→비제로
    EV_A_EFLG_CLEAR   = 16, // A채널 MCP2515 EFLG 비제로→0
    EV_A_RX_OVERRUN   = 17, // A채널 MCP2515 RX0OVR/RX1OVR 감지
    EV_A_WAKE_FIRST_TX = 18, // A채널 재수신 시작→첫 Summon TX 성공 지연
};

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
    default: return "UNKNOWN";
    }
}

#ifndef NATIVE_BUILD
inline const char* eventDetailText(uint8_t type, uint32_t detail, char* out, size_t outLen) {
    if (!out || outLen == 0) return "";
    out[0] = '\0';
    switch (type) {
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
    case EV_A_RX_OVERRUN:
        snprintf(out, outLen,
                 "eflg=0x%02X state=%s RX1OVR=%u RX0OVR=%u TXBO=%u TXEP=%u RXEP=%u TXWAR=%u RXWAR=%u EWARN=%u",
                 (unsigned)(detail & 0xFFU), aMcpEflgStateName((uint8_t)detail),
                 (unsigned)((detail >> 7) & 1U), (unsigned)((detail >> 6) & 1U),
                 (unsigned)((detail >> 5) & 1U), (unsigned)((detail >> 4) & 1U),
                 (unsigned)((detail >> 3) & 1U), (unsigned)((detail >> 2) & 1U),
                 (unsigned)((detail >> 1) & 1U), (unsigned)(detail & 1U));
        break;
    case EV_A_WAKE_FIRST_TX:
        snprintf(out, outLen, "wake_to_summon_tx_ms=%u", (unsigned)detail);
        break;
    default:
        snprintf(out, outLen, "raw=%u", (unsigned)detail);
        break;
    }
    return out;
}
#endif

struct CanEvent {
    uint32_t t_ms;
    uint8_t  type;
    uint16_t tec;
    uint16_t rec;
    uint32_t detail;   // alert raw / last id / 추가 정보
};

static constexpr size_t EVT_CAP = 200;
inline CanEvent evtBuf[EVT_CAP];
inline volatile size_t evtHead = 0;
inline volatile size_t evtCount = 0;
#ifndef NATIVE_BUILD
inline portMUX_TYPE evtMux = portMUX_INITIALIZER_UNLOCKED;
#endif

inline void eventLogPush(uint8_t type, uint16_t tec, uint16_t rec, uint32_t detail) {
#ifndef NATIVE_BUILD
    uint32_t now = millis();
    portENTER_CRITICAL(&evtMux);
    CanEvent& e = evtBuf[evtHead];
    e.t_ms = now;
    e.type = type;
    e.tec = tec;
    e.rec = rec;
    e.detail = detail;
    evtHead = (evtHead + 1) % EVT_CAP;
    if (evtCount < EVT_CAP) ++evtCount;
    portEXIT_CRITICAL(&evtMux);
#else
    (void)type; (void)tec; (void)rec; (void)detail;
#endif
}

inline void eventLogReset() {
#ifndef NATIVE_BUILD
    portENTER_CRITICAL(&evtMux);
    evtHead = 0;
    evtCount = 0;
    portEXIT_CRITICAL(&evtMux);
#else
    evtHead = 0;
    evtCount = 0;
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

inline esp_err_t eventLogCsvHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=events.csv");
    size_t n = 0;
    size_t head = 0;
    eventLogSnapshot(n, head);
    char meta[96];
    snprintf(meta, sizeof(meta), "# uptime_ms=%u  count=%u\n",
        (unsigned)millis(), (unsigned)n);
    httpd_resp_sendstr_chunk(req, meta);
    httpd_resp_sendstr_chunk(req,
        "# type: 0=BUSOFF 1=REC_OK 2=REC_FAIL 3=REC_SOFT 4=ERR_PASS 5=ARB_LOST 6=BUS_ERR 7=TX_FAIL 8=RX_FULL 9=TX_BACKOFF 10=USER_MARK 11=NAG_MODE 12=MODEB_STATE 13=MODEB_PHASE 14=MODEB_FIRST_ECHO 15=A_EFLG_SET 16=A_EFLG_CLEAR 17=A_RX_OVERRUN 18=A_WAKE_FIRST_TX\n");
    httpd_resp_sendstr_chunk(req, "# marker detail: 1=AP_WARNING_START 2=AP_WARNING_END | NAG_MODE detail: 1=MODE1 2=MODE2 3=MODE3(default) | MODEB_STATE detail: ap<<16|oldHo<<8|newHo | MODEB_PHASE detail: phase<<24|ap<<16|ho<<8|decision | FIRST_ECHO detail: delay_ms | detailText is decoded for analysis\n");
    httpd_resp_sendstr_chunk(req, "t_ms,type,typeName,tec,rec,detail,detailText\n");
    char line[260];
    char detailText[180];
    size_t start = (n < EVT_CAP) ? 0 : head;
    for (size_t i = 0; i < n; ++i) {
        CanEvent e;
        eventLogCopyAt(start + i, e);
        snprintf(line, sizeof(line), "%u,%u,%s,%u,%u,%u,%s\n",
            (unsigned)e.t_ms, (unsigned)e.type,
            eventTypeName(e.type),
            (unsigned)e.tec, (unsigned)e.rec, (unsigned)e.detail,
            eventDetailText(e.type, e.detail, detailText, sizeof(detailText)));
        httpd_resp_sendstr_chunk(req, line);
    }
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
#endif
