// CAN 통신 자가 진단 엔진
// 웹 UI 버튼 → POST /api/can-diag/start → FreeRTOS 태스크 동적 생성
// 진행: ~19초 (5+5+3초 관찰 + 처리)
// 결과: GET /api/can-diag/log?since=N 폴링 (800ms 간격 권장)
#pragma once

#include "can_helpers.h"
#include "log_buffer.h"

enum class DiagState : uint8_t { IDLE = 0, RUNNING = 1, DONE = 2 };

// 전용 진단 로그 버퍼 (기존 logRing과 분리, 32항목 — 체크리스트 전용)
inline LogRingBuffer diagLog;
inline volatile DiagState diagState = DiagState::IDLE;

#ifndef NATIVE_BUILD
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>

// canDiagStart 동시 호출 경쟁 조건 방지용 spinlock
static portMUX_TYPE diagMux = portMUX_INITIALIZER_UNLOCKED;

static void canDiagTaskFn(void* /*pv*/) {
    char buf[128];
    bool ok = true;

    // 4/10 정상 기준: Mode A / ID 880 고정
    uint16_t targetId = kNagFixedTargetId;
    uint8_t  nagMode  = kNagModeA;

    auto L = [](const char* m) { diagLog.push(m, millis()); };

    L("══ CAN 자가 진단 시작 ══");

    // ──────────────────────────────────────────────────────────
    // [1/6] TWAI 드라이버 & 태스크 상태
    // ──────────────────────────────────────────────────────────
    L("[1/6] TWAI 드라이버 & NagTask 상태");
    uint8_t sc = (uint8_t)bChannelDiag.twaiStateCode;
    bool nagOk = (bool)bChannelDiag.nagTaskCreated;
    snprintf(buf, sizeof(buf), "  TWAI: %s | NagTask: %s",
        sc==1?"running":sc==2?"BUS_OFF":"init/error",
        nagOk ? "OK" : "ERROR");
    L(buf);
    if (sc == 0 || !nagOk) {
        L("  \u274c \ub4dc\ub77c\uc774\ubc84/\ud0dc\uc2a4\ud06c \ubbf8\ucd08\uae30\ud654 \u2192 \uc804\uc6d0\xb7\ubc30\uc120 \ud655\uc778 \ud6c4 \uc7ac\ubd80\ud305");
        ok = false;
    } else if (sc == 2) {
        L("  \u26a0\ufe0f  BUS-OFF \uc0c1\ud0dc (\ubcf5\uad6c \ub300\uae30 \uc911)");
    } else {
        L("  \u2705 PASS");
    }
    // nagKillerRuntime OFF 여부 — 이후 [4/6] 에코 단계에서 오진 방지
    if (!(bool)nagKillerRuntime) {
        L("  \u26a0\ufe0f \uc8fc\uc758: Nag Killer \uae30\ub2a5\uc774 \ud604\uc7ac OFF \uc0c1\ud0dc\uc785\ub2c8\ub2e4. "
          "(\uc774\ud6c4 [4/6] \ub2e8\uacc4\uc5d0\uc11c \uc5d0\ucf54\uac00 \ubc1c\uc0dd\ud558\uc9c0 \uc54a\uc2b5\ub2c8\ub2e4)");
    }
    snprintf(buf, sizeof(buf), "  Mode:%u | \ub300\uc0c1ID: 0x%03X", nagMode, targetId);
    L(buf);
    vTaskDelay(pdMS_TO_TICKS(200));

    // ──────────────────────────────────────────────────────────
    // [2/6] 버스 트래픽 관측 (5초)
    // ──────────────────────────────────────────────────────────
    L("[2/6] \ubc84\uc2a4 \ud2b8\ub798\ud53d \uad00\uce21 (5\ucd08)...");
    uint32_t s_tot = (uint32_t)bChannelDiag.framesReceivedTotal;
    uint32_t s_tgt = (uint32_t)bChannelDiag.frames880;  // nagTargetId 기준 카운터
    uint32_t s_921 = (uint32_t)bChannelDiag.frames921;
    // (a) ArbLost/BusErr/TxFail 스냅샷
    uint32_t s_arb  = (uint32_t)bChannelDiag.bArbLost;
    uint32_t s_berr = (uint32_t)bChannelDiag.bBusError;
    uint32_t s_tfail = (uint32_t)bChannelDiag.txFail;
    // (c) dasState 변화 횟수: 5초 동안 100ms 간격 샘플링 (50회)
    uint8_t  prevDas = (uint8_t)bChannelDiag.dasHandsOnStateRx;
    uint32_t dasChanges = 0;
    for (int i = 0; i < 50; ++i) {
        vTaskDelay(pdMS_TO_TICKS(100));
        uint8_t curDas = (uint8_t)bChannelDiag.dasHandsOnStateRx;
        if (curDas != prevDas && curDas != 0xFF && prevDas != 0xFF) ++dasChanges;
        prevDas = curDas;
    }
    uint32_t d_tot = (uint32_t)bChannelDiag.framesReceivedTotal - s_tot;
    uint32_t d_tgt = (uint32_t)bChannelDiag.frames880 - s_tgt;
    uint32_t d_921 = (uint32_t)bChannelDiag.frames921 - s_921;
    uint32_t d_arb  = (uint32_t)bChannelDiag.bArbLost  - s_arb;
    uint32_t d_berr = (uint32_t)bChannelDiag.bBusError - s_berr;
    uint32_t d_tfail = (uint32_t)bChannelDiag.txFail   - s_tfail;
    snprintf(buf, sizeof(buf), "  \uc804\uccb4+%u | ID0x%03X+%u | ID921+%u | Hz=%.1f",
        d_tot, targetId, d_tgt, d_921, (float)bChannelDiag.frameHz);
    L(buf);
    snprintf(buf, sizeof(buf), "  ArbLost+%u | BusErr+%u | TxFail+%u", d_arb, d_berr, d_tfail);
    L(buf);
    if (d_arb > 5 || d_berr > 5) {
        L("  \u26a0\ufe0f  \ucda9\ub3cc/\ube44\ud2b8 \uc5d0\ub7ec \uc99d\uac00 \u2192 \ub3d9\uc77c ID \uacbd\uc7c1 \ub610\ub294 \ubb3c\ub9ac \uacc4\uce35 \ub178\uc774\uc988 \uc758\uc2ec");
    }
    if (d_tot == 0) {
        L("  \u274c \uc218\uc2e0 \uc5c6\uc74c \u2192 \uba3c\uc800 \ucc28\ub7c9 \uc804\uc6d0(\uc2dc\ub3d9)\uc774 \ucf1c\uc838 \uc788\ub294\uc9c0 \ud655\uc778 "
           "(Sleep \uc0c1\ud0dc\uc5d0\uc11c\ub294 CAN \ud2b8\ub798\ud53d\uc774 \uc5c6\uc2b5\ub2c8\ub2e4)");
        L("  \ud655\uc778 \ud6c4\uc5d0\ub3c4 \uc5c6\uc73c\uba74: \ubc30\uc120 \ub2e8\uc120 \ub610\ub294 \uc885\ub2e8\uc800\ud56d(120\u03a9) \ubd88\ub7c9 \uc758\uc2ec");
        ok = false;
    } else if (d_tgt == 0) {
        if (!(bool)nagKillerRuntime) {
            L("  \u2139\ufe0f  Nag Killer OFF \u2192 \ub300\uc0c1ID \uce74\uc6b4\ud130 \ubcc0\ud654 \uc5c6\uc74c (\uc815\uc0c1)");
        } else {
            snprintf(buf, sizeof(buf), "  \u274c ID 0x%03X \uc5c6\uc74c \u2192 Nag \ub300\uc0c1 \uc544\ub2c8\uac70\ub098 \ub2e4\ub978 \ubc84\uc2a4 \ud63c\ub3d9", targetId);
            L(buf);
            ok = false;
        }
    } else {
        L("  \u2705 PASS");
    }

    // ──────────────────────────────────────────────────────────
    // [3/6] ID 921 / DAS 핸즈온 상태
    // ──────────────────────────────────────────────────────────
    L("[3/6] ID 921 / DAS \ud578\uc988\uc628 \uc0c1\ud0dc");
    uint8_t das = (uint8_t)bChannelDiag.dasHandsOnStateRx;
    snprintf(buf, sizeof(buf), "  921+%u | dasState=0x%02X (%s) | \ubcc0\ud654:%u\ud68c",
        d_921, das,
        das==0xFF ? "\ubbf8\uc218\uc2e0" :
        (das==0||das==8) ? "\ud578\uc988\uc624\ud504" : "\uacbd\uace0\uc911",
        dasChanges);
    L(buf);
    if (d_921 == 0 || das == 0xFF) {
        L("  \u26a0\ufe0f  921 \ubbf8\uc218\uc2e0 \u2192 dasState=0xFF \uace0\ucc29 \u2192 \ud0ac\ub7ec skipDasState \uc601\uad6c \ubc1c\ub3d9");
        L("  \uc5f0\uad6c: 921 \uc5c6\uc73c\uba74 \ud578\ub4e4\ub7ec.h DAS \uccb4\ud06c \ub85c\uc9c1 \ube44\ud65c\uc131 \uac80\ud1a0 \ud544\uc694");
    } else if (das == 0 || das == 8) {
        L("  \u2139\ufe0f  \ud578\uc988\uc624\ud504 (\uacbd\uace0 \uc5c6\uc74c, \ud0ac\ub7ec \ub300\uae30 \uc911 - \uc815\uc0c1)");
    } else {
        snprintf(buf, sizeof(buf), "  \u2705 \ub098\uadf8 \uacbd\uace0 \ud65c\uc131(0x%02X) \u2192 \ud0ac\ub7ec \ubc1c\ub3d9 \uc870\uac74 \ucda9\uc871", das);
        L(buf);
    }
    vTaskDelay(pdMS_TO_TICKS(200));

    // ──────────────────────────────────────────────────────────
    // [4/6] 에코 주입 동작 확인 (5초)
    // ──────────────────────────────────────────────────────────
    L("[4/6] \uc5d0\ucf54 \uc8fc\uc785 \ub3d9\uc791 \ud655\uc778 (5\ucd08)...");
    L("  \U0001f4a1 \uc815\ud655\ud55c \uc5d0\ucf54 \ud14c\uc2a4\ud2b8\ub97c \uc704\ud574\uc11c\ub294 \uc8fc\ud589 \uc911 \uc624\ud1a0\ud30c\uc77c\ub7f3\uc744 \ucf1c"
       "\uace0 Nag(\uacbd\uace0) \uc0c1\ud669\uc744 \uc720\ub3c4\ud574\uc57c \ud569\ub2c8\ub2e4. "
       "\uc8fc\ucc28 \uc911 \ub610\ub294 Nag \uc0c1\ud669\uc774 \uc544\ub2c8\uba74 \uc5d0\ucf54 0 = \uc815\uc0c1\uc785\ub2c8\ub2e4.");
    uint32_t s_echo = (uint32_t)bChannelDiag.echoCount;
    uint32_t s_fail = (uint32_t)bChannelDiag.txFail;
    uint32_t s_tgt4 = (uint32_t)bChannelDiag.frames880;  // nagTargetId 기준
    // (b) 에코 latency 샘플링: 5초간 100ms 간격, 0이 아닌 값만 수집
    uint32_t latMin = UINT32_MAX, latMax = 0, latSum = 0, latN = 0;
    for (int i = 0; i < 50; ++i) {
        vTaskDelay(pdMS_TO_TICKS(100));
        uint32_t lat = (uint32_t)bChannelDiag.echoLatUs;
        if (lat > 0) {
            if (lat < latMin) latMin = lat;
            if (lat > latMax) latMax = lat;
            latSum += lat;
            ++latN;
        }
    }
    uint32_t d_echo = (uint32_t)bChannelDiag.echoCount - s_echo;
    uint32_t d_fail = (uint32_t)bChannelDiag.txFail - s_fail;
    uint32_t d_tgt4 = (uint32_t)bChannelDiag.frames880 - s_tgt4;
    snprintf(buf, sizeof(buf), "  ID0x%03X+%u | \uc5d0\ucf54+%u | TX\ub4dc\ub86d+%u",
        targetId, d_tgt4, d_echo, d_fail);
    L(buf);
    if (latN > 0) {
        snprintf(buf, sizeof(buf), "  Lat min=%uus avg=%uus max=%uus (n=%u)",
            latMin, latSum / latN, latMax, latN);
        L(buf);
        if (latMax > 500) {
            L("  \u26a0\ufe0f  Lat max>500us \u2192 hot-path \uc9c0\uc5f0 \uc758\uc2ec");
        }
    }
    if (d_tgt4 > 0 && d_echo == 0) {
        L("  \u26a0\ufe0f  \ub300\uc0c1ID \uc218\uc2e0 \uc788\uc73c\ub098 \uc5d0\ucf54 0 \u2192 nagKiller OFF \ub610\ub294 DAS \uc870\uac74 \ubd88\ub9cc\uc871");
    } else if (d_fail > 0 && d_echo > 0 && d_fail * 2 > d_echo) {
        snprintf(buf, sizeof(buf), "  \u26a0\ufe0f  TX\ub4dc\ub86d(suppressed \ud3ec\ud568) \ube44\uc728 \ub192\uc74c(%u/%u) \u2192 TX \ud050 \ud3ec\ud654 \ub610\ub294 BUS-OFF \ubcf5\uad6c \uc911", d_fail, d_echo);
        L(buf);
    } else if (d_echo > 0) {
        L("  \u2705 PASS: \uc5d0\ucf54 \uc815\uc0c1 \ubc1c\uc0ac");
    } else {
        L("  \u2139\ufe0f  \ub098\uadf8 \uacbd\uace0 \uc5c6\ub294 \uc0c1\ud669\uc774\uba74 \uc5d0\ucf54 0 = \uc815\uc0c1");
    }

    // ──────────────────────────────────────────────────────────
    // [5/6] TEC/REC 에러 카운터 분석 (3초)
    // ──────────────────────────────────────────────────────────
    L("[5/6] TEC/REC \uc5d0\ub7ec \uce74\uc6b4\ud130 3\ucd08 \uad00\ucc30...");
    uint32_t tec0 = (uint32_t)bChannelDiag.twaiTxErrNow;
    uint32_t rec0 = (uint32_t)bChannelDiag.twaiRxErrNow;
    vTaskDelay(pdMS_TO_TICKS(3000));
    uint32_t tec1 = (uint32_t)bChannelDiag.twaiTxErrNow;
    int32_t dtec = (int32_t)tec1 - (int32_t)tec0;
    snprintf(buf, sizeof(buf), "  TEC %u\u2192%u(\u0394%+d) | REC %u\u2192%u | Peak TX%u/RX%u",
        tec0, tec1, dtec, rec0, (uint32_t)bChannelDiag.twaiRxErrNow,
        (uint32_t)bChannelDiag.twaiTxErrPeak, (uint32_t)bChannelDiag.twaiRxErrPeak);
    L(buf);
    if (dtec > 20) {
        L("  \u274c TEC \uae09\uc0c1\uc2b9 \u2192 GND \uc804\uc704\ucc28 \ub610\ub294 \uc885\ub2e8\uc800\ud56d \ubb38\uc81c");
        L("  \ucc98\ubc29: \ucc28\ub7c9 GND\uc5d0 \uc9c1\uc811 \uc810\ud37c\xb7\ucf00\uc774\ube14 1m \uc774\ub0b4 \ub2e8\ucd95");
        ok = false;
    } else if (dtec > 5) {
        L("  \u26a0\ufe0f  TEC \uc18c\ub7c9 \uc0c1\uc2b9 \u2192 \uac04\ud5d0\uc801 \ube44\ud2b8 \uc5d0\ub7ec (\ubb3c\ub9ac \uacc4\uce35 \uc810\uac80 \uad8c\uc7a5)");
    } else {
        L("  \u2705 PASS: TEC \uc548\uc815");
    }

    // ──────────────────────────────────────────────────────────
    // [6/6] BUS-OFF 이력 분석
    // ──────────────────────────────────────────────────────────
    L("[6/6] BUS-OFF \uc774\ub825 \ubd84\uc11d");
    uint32_t boCnt  = (uint32_t)bChannelDiag.busoffCount;
    uint32_t boSucc = (uint32_t)bChannelDiag.recoverySuccessCount;
    uint32_t boFail = (uint32_t)bChannelDiag.recoveryFailCount;
    uint32_t lastBo = (uint32_t)bChannelDiag.lastBusoffMs;
    snprintf(buf, sizeof(buf), "  BUS-OFF:%u | \ubcf5\uad6c\uc131\uacf5:%u | \uc2e4\ud328:%u%s",
        boCnt, boSucc, boFail,
        (boCnt > 0 && lastBo > 0) ? " (recent)" : "");
    L(buf);
    if (boCnt > 0 && lastBo > 0) {
        uint32_t now = millis();
        uint32_t agoMs = (now > lastBo) ? (now - lastBo) : 0;
        snprintf(buf, sizeof(buf), "  \ucd5c\uadfc BUS-OFF: %u.%us \uc804", agoMs / 1000, (agoMs / 100) % 10);
        L(buf);
    }
    if (boCnt > 0) {
        uint32_t maxDur = (uint32_t)bChannelDiag.maxRecoveryDurationMs;
        uint32_t coolMs = (uint32_t)bChannelDiag.busoffCooldownMs;
        snprintf(buf, sizeof(buf), "  \uad00\uce21 \ucd5c\ub300 \ubcf5\uad6c \uc18c\uc694: %ums | \ud604\uc7ac \ucfe8\ub2e4\uc6b4: %ums", maxDur, coolMs);
        L(buf);
        if (maxDur > 0) {
            float margin = (float)coolMs / (float)maxDur;
            snprintf(buf, sizeof(buf), "  \ub9c8\uc9c4: %.1f\xc3\x97 %s \xe2\x86\x92 \uad8c\uc7a5\uac12: %ums \uc774\uc0c1",
                margin,
                margin >= 3.0f ? "\xe2\x9c\x85 \ucda9\ubd84" : (margin >= 1.5f ? "\xe2\x9a\xa0\xef\xb8\x8f \ub2e4\uc18c \uc5ec\uc720" : "\xe2\x9c\x97 \uc704\ud5d8"),
                (uint32_t)(maxDur * 2));
            L(buf);
        } else {
            L("  \uce21\uc815 \ub370\uc774\ud130 \uc5c6\uc74c (\ubcf5\uad6c \uc2dc\uc791\ub418\uc5c8\uc73c\ub098 \uc18c\uc694\uc2dc\uac04 \ubbf8\uae30\ub85d)");
        }
    }
    if (boFail > 0) {
        L("  \u274c \ubcf5\uad6c \uc2e4\ud328 \uae30\ub85d \u2192 GND \uc810\uac80 \ud6c4 \uc7ac\ubd80\ud305");
        ok = false;
    } else if (boCnt > 3) {
        L("  \u26a0\ufe0f  \ubc18\ubcf5 BUS-OFF \u2192 \ubb3c\ub9ac \uacc4\uce35 \ubd88\uc548\uc815");
        L("  \ucf54\ub4dc: \ubcf5\uad6c \ud6c4 200ms \uce68\ubb35\uad6c\uac04 \ucd94\uac00 \uac80\ud1a0");
    } else if (boCnt == 0) {
        L("  \u2705 PASS: BUS-OFF \uc5c6\uc74c");
    } else {
        L("  \u2705 PASS: \ubcf5\uad6c \uc131\uacf5");
    }

    // ──────────────────────────────────────────────────────────
    // 종합 결과
    // ──────────────────────────────────────────────────────────
    L(ok ? "\u2705 \uc885\ud569: \ud1b5\uc2e0 \uc815\uc0c1 \u2014 \uc774\uc0c1 \uc5c6\uc74c"
         : "\u274c \uc885\ud569: \uc774\uc288 \ubc1c\uacac \u2014 \uc704 \ucc98\ubc29 \ud655\uc778");
    L("\u2550\u2550 \uc9c4\ub2e8 \uc644\ub8cc \u2550\u2550");

    diagState = DiagState::DONE;
    vTaskDelete(nullptr);
}

// 진단 태스크 시작 (이미 실행 중이면 false 반환)
inline bool canDiagStart() {
    // check-and-set 원자화: 동시 HTTP 요청 경쟁 조건 방지
    bool started = false;
    portENTER_CRITICAL(&diagMux);
    if (diagState != DiagState::RUNNING) {
        diagState = DiagState::RUNNING;
        started = true;
    }
    portEXIT_CRITICAL(&diagMux);
    if (!started) return false;
    diagLog.push("\uc9c4\ub2e8 \ud0dc\uc2a4\ud06c \uc900\ube44 \uc911...", millis());
    BaseType_t r = xTaskCreatePinnedToCore(
        canDiagTaskFn, "canDiag", 4096, nullptr, 1, nullptr, 1);
    if (r != pdPASS) {
        diagState = DiagState::IDLE;
        return false;
    }
    return true;
}
#endif  // NATIVE_BUILD
