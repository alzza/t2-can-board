// 밀리초 단위 CAN 이벤트 링버퍼 (B채널 진단용)
// 통합 로그 [5] 섹션에 포함되며, /api/events.csv는 디버그용 보조 엔드포인트다.
// 외과적 추가: 기존 구조 변경 없음
#pragma once
#include <stdint.h>
#include <string.h>

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
    EV_NAG_MODE       = 11, // Nag 모드 전환 (detail: 0=A, 1=B)
    EV_MODEB_STATE    = 12, // Mode B DAS hands-on state 전이 (detail: ap<<16 | old<<8 | new)
    EV_MODEB_PHASE    = 13, // Mode B phase 전이 (detail: phase<<24 | ap<<16 | ho<<8 | decision)
    EV_MODEB_FIRST_ECHO = 14, // 현재 DAS state 진입 후 첫 echo 지연(ms)
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
    default: return "UNKNOWN";
    }
}

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
        "# type: 0=BUSOFF 1=REC_OK 2=REC_FAIL 3=REC_SOFT 4=ERR_PASS 5=ARB_LOST 6=BUS_ERR 7=TX_FAIL 8=RX_FULL 9=TX_BACKOFF 10=USER_MARK 11=NAG_MODE 12=MODEB_STATE 13=MODEB_PHASE 14=MODEB_FIRST_ECHO\n");
    httpd_resp_sendstr_chunk(req, "# marker detail: 1=AP_WARNING | NAG_MODE detail: 0=A 1=B | MODEB_STATE detail: ap<<16|oldHo<<8|newHo | MODEB_PHASE detail: phase<<24|ap<<16|ho<<8|decision | FIRST_ECHO detail: delay_ms\n");
    httpd_resp_sendstr_chunk(req, "t_ms,type,typeName,tec,rec,detail\n");
    char line[120];
    size_t start = (n < EVT_CAP) ? 0 : head;
    for (size_t i = 0; i < n; ++i) {
        CanEvent e;
        eventLogCopyAt(start + i, e);
        snprintf(line, sizeof(line), "%u,%u,%s,%u,%u,%u\n",
            (unsigned)e.t_ms, (unsigned)e.type,
            eventTypeName(e.type),
            (unsigned)e.tec, (unsigned)e.rec, (unsigned)e.detail);
        httpd_resp_sendstr_chunk(req, line);
    }
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
#endif
