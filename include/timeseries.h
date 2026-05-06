// 5초 간격 시계열 통계 수집 (최근 30분, RAM wrap-around)
// 부팅 시 자동 시작 → GET /api/timeseries.csv 로 회수
// 외과적 추가: 기존 구조 변경 없음
#pragma once
#include "can_helpers.h"
#include "event_log.h"

#ifndef NATIVE_BUILD
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>
#include <esp_http_server.h>
#include <Arduino.h>
#include <stdio.h>

struct TsSample {
    uint32_t t_ms;
    uint32_t busoff;
    uint32_t tec;
    uint32_t rec;
    uint32_t arbLost;
    uint32_t busErr;
    uint32_t txFail;
    uint32_t echoCnt;
    uint32_t f880;
    uint32_t f921;
    uint32_t echoDrop;
    uint32_t skipRuntime;
    uint32_t skipHandsOn;
    uint32_t skipDas;
    uint32_t noDasEcho;
    // 사용자가 "지금 경고가 떴다" 버튼을 누른 횟수. dUserMark가 해당 5초 구간 기준점이다.
    uint32_t userMark;
    uint16_t d880;
    uint16_t d921;
    uint16_t dEcho;
    uint16_t dDrop;
    uint16_t dSkipRuntime;
    uint16_t dSkipHandsOn;
    uint16_t dSkipDas;
    uint16_t dNoDasEcho;
    uint16_t dUserMark;
    uint8_t  handsOn;
    uint8_t  dasState;
    uint8_t  lastDecision;
    // intervalDecision은 5초 구간 요약, lastDecision은 마지막 880 처리 분기다.
    uint8_t  intervalDecision;
};

static constexpr size_t TS_CAP = 120;  // 120 × 5s = 10분 (메모리 절약: 31KB → 10.5KB)
inline TsSample tsBuf[TS_CAP];
inline volatile size_t tsHead = 0;
inline volatile size_t tsCount = 0;
inline volatile uint32_t tsResetMs = 0;  // 마지막 리셋 시각 (CSV 메타용, 0=부팅 이후 리셋 없음)
inline volatile uint32_t tsRecStartMs = 0;  // 사용자 '기록시작' 시각 (0=정지/미시작)
inline volatile bool tsRecording = false;   // REC 표시 ON/OFF (수집 자체는 항상 동작)
inline portMUX_TYPE tsMux = portMUX_INITIALIZER_UNLOCKED;

inline volatile uint32_t tsBaseBusoff = 0;
inline volatile uint32_t tsBaseArbLost = 0;
inline volatile uint32_t tsBaseBusErr = 0;
inline volatile uint32_t tsBaseTxFail = 0;
inline volatile uint32_t tsBaseEcho = 0;
inline volatile uint32_t tsBaseF880 = 0;
inline volatile uint32_t tsBaseF921 = 0;
inline volatile uint32_t tsBaseEchoDrop = 0;
inline volatile uint32_t tsBaseSkipRuntime = 0;
inline volatile uint32_t tsBaseSkipHandsOn = 0;
inline volatile uint32_t tsBaseSkipDas = 0;
inline volatile uint32_t tsBaseNoDasEcho = 0;
inline volatile uint32_t tsBaseUserMark = 0;

inline volatile uint32_t tsPrevEcho = 0;
inline volatile uint32_t tsPrevF880 = 0;
inline volatile uint32_t tsPrevF921 = 0;
inline volatile uint32_t tsPrevEchoDrop = 0;
inline volatile uint32_t tsPrevSkipRuntime = 0;
inline volatile uint32_t tsPrevSkipHandsOn = 0;
inline volatile uint32_t tsPrevSkipDas = 0;
inline volatile uint32_t tsPrevNoDasEcho = 0;
inline volatile uint32_t tsPrevUserMark = 0;

inline uint32_t tsDelta(uint32_t current, uint32_t base) {
    return current - base;
}

inline uint16_t tsDelta16(uint32_t current, uint32_t base) {
    uint32_t value = current - base;
    return value > 65535U ? 65535U : (uint16_t)value;
}

// CSV 메타 라인 콜백 — web_server.h에서 드라이버 토글 상태 주입·
using TsMetaWriter = void(*)(httpd_req_t*);
inline TsMetaWriter tsMetaWriter = nullptr;

static void timeseriesTaskFn(void*) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        TsSample s;
        s.t_ms     = millis();
        s.busoff   = tsDelta((uint32_t)bChannelDiag.busoffCount, (uint32_t)tsBaseBusoff);
        s.tec      = (uint32_t)bChannelDiag.twaiTxErrNow;
        s.rec      = (uint32_t)bChannelDiag.twaiRxErrNow;
        s.arbLost  = tsDelta((uint32_t)bChannelDiag.bArbLost, (uint32_t)tsBaseArbLost);
        s.busErr   = tsDelta((uint32_t)bChannelDiag.bBusError, (uint32_t)tsBaseBusErr);
        s.txFail   = tsDelta((uint32_t)bChannelDiag.txFail, (uint32_t)tsBaseTxFail);
        uint32_t curEcho = (uint32_t)bChannelDiag.echoCount;
        uint32_t cur880 = (uint32_t)bChannelDiag.frames880;
        uint32_t cur921 = (uint32_t)bChannelDiag.frames921;
        uint32_t curDrop = (uint32_t)bChannelDiag.echoDroppedLate;
        uint32_t curSkipRuntime = (uint32_t)bChannelDiag.skipRuntimeOrInactive;
        uint32_t curSkipHandsOn = (uint32_t)bChannelDiag.skipHandsOn;
        uint32_t curSkipDas = (uint32_t)bChannelDiag.skipDasState;
        uint32_t curNoDas = (uint32_t)bChannelDiag.nagFiredNoDas;
        uint32_t curUserMark = (uint32_t)userMarkerCount;
        s.echoCnt  = tsDelta(curEcho, (uint32_t)tsBaseEcho);
        s.f880     = tsDelta(cur880, (uint32_t)tsBaseF880);
        s.f921     = tsDelta(cur921, (uint32_t)tsBaseF921);
        s.echoDrop = tsDelta(curDrop, (uint32_t)tsBaseEchoDrop);
        s.skipRuntime = tsDelta(curSkipRuntime, (uint32_t)tsBaseSkipRuntime);
        s.skipHandsOn = tsDelta(curSkipHandsOn, (uint32_t)tsBaseSkipHandsOn);
        s.skipDas = tsDelta(curSkipDas, (uint32_t)tsBaseSkipDas);
        s.noDasEcho = tsDelta(curNoDas, (uint32_t)tsBaseNoDasEcho);
        s.userMark = tsDelta(curUserMark, (uint32_t)tsBaseUserMark);
        s.d880 = tsDelta16(cur880, (uint32_t)tsPrevF880);
        s.d921 = tsDelta16(cur921, (uint32_t)tsPrevF921);
        s.dEcho = tsDelta16(curEcho, (uint32_t)tsPrevEcho);
        s.dDrop = tsDelta16(curDrop, (uint32_t)tsPrevEchoDrop);
        s.dSkipRuntime = tsDelta16(curSkipRuntime, (uint32_t)tsPrevSkipRuntime);
        s.dSkipHandsOn = tsDelta16(curSkipHandsOn, (uint32_t)tsPrevSkipHandsOn);
        s.dSkipDas = tsDelta16(curSkipDas, (uint32_t)tsPrevSkipDas);
        s.dNoDasEcho = tsDelta16(curNoDas, (uint32_t)tsPrevNoDasEcho);
        s.dUserMark = tsDelta16(curUserMark, (uint32_t)tsPrevUserMark);
        s.handsOn  = (uint8_t)bChannelDiag.realHo;
        s.dasState = (uint8_t)bChannelDiag.dasHandsOnStateRx;
        s.lastDecision = (uint8_t)bChannelDiag.nagLastDecision;
        s.intervalDecision = nagIntervalDecision(s.d880, s.d921, s.dEcho, s.dDrop,
            s.dSkipRuntime, s.dSkipHandsOn, s.dSkipDas,
            (bool)nagKillerRuntime);
        portENTER_CRITICAL(&tsMux);
        tsBuf[tsHead] = s;
        tsPrevEcho = curEcho;
        tsPrevF880 = cur880;
        tsPrevF921 = cur921;
        tsPrevEchoDrop = curDrop;
        tsPrevSkipRuntime = curSkipRuntime;
        tsPrevSkipHandsOn = curSkipHandsOn;
        tsPrevSkipDas = curSkipDas;
        tsPrevNoDasEcho = curNoDas;
        tsPrevUserMark = curUserMark;
        tsHead = (tsHead + 1) % TS_CAP;
        if (tsCount < TS_CAP) ++tsCount;
        portEXIT_CRITICAL(&tsMux);
    }
}

inline void timeseriesStart() {
    xTaskCreatePinnedToCore(timeseriesTaskFn, "ts", 2048, nullptr, 1, nullptr, 0);
}

// 시계열 버퍼 + 기준점을 리셋. 드라이버 누적 카운터는 건드리지 않는다.
inline void timeseriesReset() {
    uint32_t now = millis();
    portENTER_CRITICAL(&tsMux);
    tsHead = 0;
    tsCount = 0;
    tsResetMs = now;
    tsBaseBusoff = (uint32_t)bChannelDiag.busoffCount;
    tsBaseArbLost = (uint32_t)bChannelDiag.bArbLost;
    tsBaseBusErr = (uint32_t)bChannelDiag.bBusError;
    tsBaseTxFail = (uint32_t)bChannelDiag.txFail;
    tsBaseEcho = (uint32_t)bChannelDiag.echoCount;
    tsBaseF880 = (uint32_t)bChannelDiag.frames880;
    tsBaseF921 = (uint32_t)bChannelDiag.frames921;
    tsBaseEchoDrop = (uint32_t)bChannelDiag.echoDroppedLate;
    tsBaseSkipRuntime = (uint32_t)bChannelDiag.skipRuntimeOrInactive;
    tsBaseSkipHandsOn = (uint32_t)bChannelDiag.skipHandsOn;
    tsBaseSkipDas = (uint32_t)bChannelDiag.skipDasState;
    tsBaseNoDasEcho = (uint32_t)bChannelDiag.nagFiredNoDas;
    tsBaseUserMark = (uint32_t)userMarkerCount;
    tsPrevEcho = (uint32_t)bChannelDiag.echoCount;
    tsPrevF880 = (uint32_t)bChannelDiag.frames880;
    tsPrevF921 = (uint32_t)bChannelDiag.frames921;
    tsPrevEchoDrop = (uint32_t)bChannelDiag.echoDroppedLate;
    tsPrevSkipRuntime = (uint32_t)bChannelDiag.skipRuntimeOrInactive;
    tsPrevSkipHandsOn = (uint32_t)bChannelDiag.skipHandsOn;
    tsPrevSkipDas = (uint32_t)bChannelDiag.skipDasState;
    tsPrevNoDasEcho = (uint32_t)bChannelDiag.nagFiredNoDas;
    tsPrevUserMark = (uint32_t)userMarkerCount;
    portEXIT_CRITICAL(&tsMux);
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
    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=timeseries.csv");
    size_t n = 0;
    size_t head = 0;
    uint32_t resetMs = 0;
    uint32_t recStartMs = 0;
    bool recording = false;
    timeseriesSnapshot(n, head, resetMs, recStartMs, recording);
    char meta[160];
    snprintf(meta, sizeof(meta), "# uptime_ms=%u  reset_at_ms=%u  rec_start_ms=%u  rec=%s  samples=%u  interval_s=5\n",
        (unsigned)millis(), (unsigned)resetMs, (unsigned)recStartMs,
        recording?"ON":"OFF", (unsigned)n);
    httpd_resp_sendstr_chunk(req, meta);
    if (tsMetaWriter) tsMetaWriter(req);  // web_server에서 드라이버 토글 등 주입
    const char* hdr = "t_s,busoff,tec,rec,arbLost,busErr,txFail,echo,f880,f921,ho,dasState,echoDrop,skipOff,skipHO,skipDAS,noDAS,userMark,d880,d921,dEcho,dDrop,dSkipOff,dSkipHO,dSkipDAS,dNoDAS,dUserMark,lastDecision,intervalDecision\n";
    httpd_resp_sendstr_chunk(req, hdr);
    char line[384];
    size_t start = (n < TS_CAP) ? 0 : head;  // oldest first
    for (size_t i = 0; i < n; ++i) {
        TsSample s;
        timeseriesCopyAt(start + i, s);
        snprintf(line, sizeof(line), "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
            (unsigned)(s.t_ms / 1000),
            (unsigned)s.busoff, (unsigned)s.tec, (unsigned)s.rec,
            (unsigned)s.arbLost, (unsigned)s.busErr, (unsigned)s.txFail,
            (unsigned)s.echoCnt, (unsigned)s.f880, (unsigned)s.f921,
            (unsigned)s.handsOn, (unsigned)s.dasState,
            (unsigned)s.echoDrop, (unsigned)s.skipRuntime,
            (unsigned)s.skipHandsOn, (unsigned)s.skipDas,
            (unsigned)s.noDasEcho, (unsigned)s.userMark,
            (unsigned)s.d880, (unsigned)s.d921,
            (unsigned)s.dEcho, (unsigned)s.dDrop, (unsigned)s.dSkipRuntime,
            (unsigned)s.dSkipHandsOn, (unsigned)s.dSkipDas,
            (unsigned)s.dNoDasEcho, (unsigned)s.dUserMark, (unsigned)s.lastDecision,
            (unsigned)s.intervalDecision);
        httpd_resp_sendstr_chunk(req, line);
    }
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

// POST /api/timeseries/reset
inline esp_err_t timeseriesResetHandler(httpd_req_t* req) {
    timeseriesReset();
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
        timeseriesReset();          // 깨끗한 구간으로 시작
        portENTER_CRITICAL(&tsMux);
        tsRecStartMs = millis();
        tsRecording = true;
        portEXIT_CRITICAL(&tsMux);
    } else {
        portENTER_CRITICAL(&tsMux);
        tsRecording = false;        // 시작 시각은 그대로 두어 경과 표시 유지 가능
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
