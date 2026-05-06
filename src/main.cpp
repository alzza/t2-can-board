/**
 * @file main.cpp
 * @note Dual-channel runtime integration (A=MCP2515, B=TWAI)
 *
 * Runtime Overview
 * - MCU: ESP32-S3 (LILYGO T2-CAN), Arduino-ESP32 2.0.17 (= ESP-IDF v4.4.x)
 * - CAN-A: MCP2515 over SPI    — A/B 통합 폴링(`nagKillerTask`, Core 1, prio 10)
 * - CAN-B: ESP32-S3 내장 TWAI  — 같은 `nagKillerTask` 본문에서 처리 (Core 1)
 * - Pins : TWAI TX=GPIO7 / RX=GPIO6, MCP2515 SPI=GPIO10/11/12/13, RST=GPIO9
 * - Bitrate: 500 kbps (A/B 공통)
 * - WiFi : AP-only (`TeslaCAN`, ch=kApChannel) — STA 비활성으로 TWAI ACK 안정화
 * - Web  : esp_http_server (Core 0) + cJSON, NVS 영속화, OTA(롤백 지원)
 *
 * ┌── Architecture ─────────────────────────────────────────────────────────┐
 * │                                                                          │
 * │  CAN Bus A ──► MCP2515(SPI) ──► appLoop()  [Core 1, in nagKillerTask]   │
 * │                                  ├─ HW3Handler(ID 1021)                  │
 * │                                  │   ├─ Mux 0 → TSLLC bit38/39 inject    │
 * │                                  │   └─ Mux 1 → EAP bit19/46 inject      │
 * │                                  ├─ EFLG/TEC/REC/MERRF 1초 폴링          │
 * │                                                                          │
 * │  CAN Bus B ──► TWAI(GPIO7/6) ──► nagKillerTask()    [Core 1, prio 10]   │
 * │                                  ├─ TWAI ISR(IRAM 요청)도 Core 1         │
 * │                                  ├─ SW filter: 880 / 921                │
 * │                                  ├─ NagHandler                          │
 * │                                  │   ├─ Mode A: Stealth PRNG            │
 * │                                  │   · checksum: sum + 0x73 & 0xFF      │
 * │                                  ├─ BUS-OFF 복구: hard re-install only  │
 * │                                  └─ TEC≥96 조기 경고 / BUS-OFF 이벤트로그│
 * │                                                                          │
 * │  esp_http_server (Core 0) ──► Web Dashboard (web_ui.h, single-file SPA) │
 * │   GET  /                  → 대시보드                                    │
 * │   GET  /api/status        → 통합 상태 JSON (3s polling)                 │
 * │   GET  /api/nag-config|stats / POST /api/nag-mode|update|reset         │
 * │   POST /api/enhanced-autopilot | /api/tsllc | /api/nag-killer          │
 * │   POST /api/busoff-mode|cooldown   GET /api/busoff-log[-dl] DELETE     │
 * │   POST /api/twai-ss-tx | /api/twai-busoff-stop  (v4.4 실험 토글)       │
 * │   POST /api/can-diag/start  GET /api/can-diag/log  (자가 진단)         │
 * │   GET  /api/logs-bundle   POST /api/time  (wall-clock 동기화)          │
 * │   POST /api/ota | /api/reboot | /api/emergency-disable|restore         │
 * │                                                                          │
 * │  NVS namespace "canmod": 토글/NagConfig/테마/BUS-OFF 모드 영속화        │
 * └──────────────────────────────────────────────────────────────────────────┘
 */

#include <Arduino.h>
#include "app.h"
#include "t2can_pins.h"
#include "drivers/mcp2515_driver.h"
#include "drivers/twai_driver.h"
#include "handlers.h"
#include "event_log.h"

static std::unique_ptr<TWAIDriver> driverB;
static NagHandler nagHandlerB;

TaskHandle_t nagTaskHandle = nullptr;

// [v4.4 ALERT] B채널 alert 폴링 태스크 — 20ms 주기, Core 0, prio 1
// nagKillerTask(prio 10)와 분리되어 핫패스 영향 없음
static TWAIDriver* gAlertDrv = nullptr;
static void canAlertTask(void*) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(20));  // [결함 3 수정] burst 정확도 5배
        if (!gAlertDrv) continue;
        uint16_t tec = 0, rec = 0;
        uint32_t a = gAlertDrv->pollAlerts(tec, rec);
        if (!a) continue;
        // BUS_OFF는 driver 내부에서 직접 push되므로 여기선 alert 비트로만 부가 기록
        if (a & TWAI_ALERT_ERR_PASS)       eventLogPush(EV_ALERT_ERR_PASS, tec, rec, a);
        if (a & TWAI_ALERT_ARB_LOST)       eventLogPush(EV_ALERT_ARB_LOST, tec, rec, a);
        if (a & TWAI_ALERT_BUS_ERROR)      eventLogPush(EV_ALERT_BUS_ERR,  tec, rec, a);
        if (a & TWAI_ALERT_TX_FAILED)      eventLogPush(EV_ALERT_TX_FAIL,  tec, rec, a);
        if (a & TWAI_ALERT_RX_QUEUE_FULL)  eventLogPush(EV_ALERT_RX_FULL,  tec, rec, a);
    }
}



void nagKillerTask(void* pvParameters) {
    uint32_t lastStatusTime = 0;
    uint32_t lastDebugTime = 0;
    uint32_t bHzFrames = 0;
    uint32_t lastHzMs  = millis();

    logRing.push("🔵 [B-CH] Task 시작 - 감시 시작", millis());
    Serial.println("🔵 [B-CH] Task 시작 - 감시 시작");
    bChannelDiag.nagTaskCreated = true;
    bChannelDiag.taskCoreId = xPortGetCoreID();

    uint32_t lastTecWarnMs = 0;  // TEC 경고 로그 쓰로틀용
    uint32_t prevBusoffForLog = 0; // BUS-OFF 이벤트 로그용 직전값
    uint32_t prevRecovFail    = 0; // 복구 실패 카운터 직전값
    uint32_t lastBusoffEventMs = 0; // 이전 BUS-OFF 시각 (시간간격 계산용)
    uint32_t prevCooldown = 1000;  // 쿨다운 변경 감지용
    uint32_t prevLog880 = 0;
    uint32_t prevLog921 = 0;
    uint32_t prevLogEcho = 0;
    uint32_t prevLogDrop = 0;
    uint32_t prevLogSkipRuntime = 0;
    uint32_t prevLogSkipHandsOn = 0;
    uint32_t prevLogSkipDas = 0;
    uint32_t prevLogNoDas = 0;

    while (true) {
        bChannelDiag.lastLoopMs = millis();

        // ── [통합 폴링] A채널(MCP2515/SPI) 매 iter 처리 ─────────────────────
        // 같은 코어(Core 1)에서 A/B를 모두 폴링해 ISR-task 코어 일치 + Core 0
        // (WiFi/HTTP) 부하 격리. vTaskDelay(1ms) 양보로 loopTask와 공존 가능.
#if defined(BOARD_T2CAN) && !defined(NATIVE_BUILD)
        if (!gOtaRecoveryModeActive) {
            appLoop<MCP2515Driver>();
        }
#endif

        if (driverB) {
            // TWAI 상태 추적
            bChannelDiag.twaiConnected = driverB->isDriverOK();
            if (driverB->isDriverOK()) {
                bChannelDiag.twaiStateCode = driverB->isBusOffState() ? 2 : 1;
            } else {
                bChannelDiag.twaiStateCode = 0;
            }

            // 쿨다운 런타임 설정 변경 시 드라이버에 전달
            {
                uint32_t newCool = (uint32_t)bChannelDiag.busoffCooldownMs;
                if (newCool != prevCooldown) {
                    driverB->setCooldownMs(newCool);
                    prevCooldown = newCool;
                }
            }

            uint32_t curBo = driverB->getBusoffCount();
            bChannelDiag.busoffCount = curBo;
            // send() void 반환이므로 드라이버 내부 suppressed 카운터를 txFail로 노출
            bChannelDiag.txFail = driverB->getTxSuppressedCount();

            // BUS-OFF 이벤트 자동 로그 (복구 완료 후 카운터 증가 감지하면 기록)
            if (curBo > prevBusoffForLog) {
                uint32_t curFail = driverB->getRecoveryFailCount();
                BusOffEvent ev;
                ev.timestampMs   = driverB->getLastBusOffMs();
                ev.seqNum        = curBo;
                ev.tec           = driverB->getBusOffTec();
                ev.rec           = driverB->getBusOffRec();
                ev.recoveryDurMs = driverB->getLastRecoveryDurationMs();
                // millis() overflow(49.7일) 안전: unsigned 뺄셈 wrap-around 활용
                ev.sinceLastMs   = (lastBusoffEventMs > 0)
                                    ? (uint32_t)(ev.timestampMs - lastBusoffEventMs) : 0;
                ev.recovered     = (curFail == prevRecovFail) ? 1 : 0;
                ev.pad[0] = ev.pad[1] = ev.pad[2] = 0;
                busOffLog.push(ev);
                lastBusoffEventMs = ev.timestampMs;
                prevRecovFail     = curFail;
                prevBusoffForLog  = curBo;
            }

            // 복구 카운터 동기화 (이전에는 미연결 → 대시보드에 항상 0/0/0으로 표시되던 문제 수정)
            bChannelDiag.recoveryAttemptCount   = driverB->getRecoveryAttemptCount();
            bChannelDiag.recoverySuccessCount   = driverB->getRecoverySuccessCount();
            bChannelDiag.recoveryFailCount      = driverB->getRecoveryFailCount();
            bChannelDiag.lastRecoveryDurationMs = driverB->getLastRecoveryDurationMs();
            bChannelDiag.maxRecoveryDurationMs  = driverB->getMaxRecoveryDurationMs();
            bChannelDiag.lastBusoffMs           = driverB->getLastBusOffMs();

            // 에러 카운터 상시 샘플링 (TEC/REC 피크 추적 + 심층 진단)
            if (driverB->isDriverOK()) {
                twai_status_info_t twSt = {};
                if (twai_get_status_info(&twSt) == ESP_OK) {
                    if (twSt.tx_error_counter > (uint32_t)bChannelDiag.twaiTxErrPeak)
                        bChannelDiag.twaiTxErrPeak = twSt.tx_error_counter;
                    if (twSt.rx_error_counter > (uint32_t)bChannelDiag.twaiRxErrPeak)
                        bChannelDiag.twaiRxErrPeak = twSt.rx_error_counter;
                    bChannelDiag.twaiTxErrNow = twSt.tx_error_counter;
                    bChannelDiag.twaiRxErrNow = twSt.rx_error_counter;
                    // 심층 진단 카운터 (드라이버 누적값 그대로 노출)
                    bChannelDiag.bArbLost   = twSt.arb_lost_count;
                    bChannelDiag.bBusError  = twSt.bus_error_count;
                    bChannelDiag.bTxFailed  = twSt.tx_failed_count;
                    bChannelDiag.bRxMissed  = twSt.rx_missed_count;
                    // TEC >= 96: BUS-OFF 전 조기 경고 (2초 쓰로틀)
                    if (twSt.tx_error_counter >= 96 && millis() - lastTecWarnMs > 2000) {
                        lastTecWarnMs = millis();
                        char tecBuf[80];
                        snprintf(tecBuf, sizeof(tecBuf),
                            "⚠️ [B-CH] TEC=%u REC=%u (에러 수동->BUS-OFF 임박)",
                            (unsigned)twSt.tx_error_counter,
                            (unsigned)twSt.rx_error_counter);
                        logRing.push(tecBuf, millis());
                        Serial.println(tecBuf);
                    }
                }
            }

            CanFrame frame;
            int rxLimit = 30; // 무한 루프(WDT Panic) 방지 제한
            while (driverB->read(frame) && rxLimit > 0) {
                bChannelDiag.lastTwaiOk = true;
                bChannelDiag.frameIdReceived = frame.id;
                bChannelDiag.framesReceivedTotal++;
                bChannelDiag.lastFrameRxMs = millis();

                // SW 필터: 880(EPAS3P) / 921(DAS_status) / 297(SCCM_steer, Mode B용)
                if ((frame.id == kNagFixedTargetId || frame.id == 921 || frame.id == 297) && frame.dlc >= 4) {
                    bChannelDiag.framesFilteredInTotal++;
                    if (frame.id == kNagFixedTargetId) bChannelDiag.frames880++;
                    else if (frame.id == 921)          bChannelDiag.frames921++;
                    else if (frame.id == 297)          {}  // frames297 는 handler 내부에서 증가
                    // [legacy 제거] echo deadline(_rxMs) 가설은 무효화됨 → handler가 수신 시각을 별도로 추적할 필요 없음
                    nagHandlerB.handleMessage(frame, *driverB);
                    bHzFrames++;
                } else {
                    bChannelDiag.framesFilteredOutTotal++;
                }
                rxLimit--;
            }
        }


        // B채널 Hz 계산 (1초마다)
        {
            uint32_t nowHz = millis();
            uint32_t elapsed = nowHz - lastHzMs;
            if (elapsed >= 1000) {
                bChannelDiag.frameHz = bHzFrames * 1000.0f / (float)(elapsed > 0 ? elapsed : 1);
                bHzFrames = 0;
                lastHzMs = nowHz;
            }
        }

        // 5초 간격 A/B 채널 상태 출력
        if (millis() - lastStatusTime > 5000) {
            char aBuf[200];
            // 진단 신호 한 줄에 통합: TX OK/Fail · TEC/peak · REC · MERRF · RX-OVR · EFLG
            //  · MERRF↑+TxFail=0 → ACK 부재(H1) 또는 동일 ID 충돌(H2)
            //  · TxFail↑           → 송신 큐 포화/하드 실패(H3)
            //  · RX-OVR↑ 누적     → A루프 폴링 부족
            snprintf(aBuf, sizeof(aBuf),
                "🟢 [A-CH] RX:%u | 1021:%u | mod:%u | TX:OK=%u/Fail=%u | TEC=%u/peak=%u | REC=%u | MERRF=%u | RX-OVR=%u | EFLG=0x%02X",
                (unsigned)aChannelDiag.framesReceivedTotal,
                (unsigned)aChannelDiag.frames1021,
                (unsigned)aChannelDiag.eapModifiedCount,
                (unsigned)aChannelDiag.aTxOk,
                (unsigned)aChannelDiag.aTxFail,
                (unsigned)(uint8_t)aChannelDiag.aTec,
                (unsigned)(uint8_t)aChannelDiag.aTecPeak,
                (unsigned)(uint8_t)aChannelDiag.aRec,
                (unsigned)aChannelDiag.aMerrfCount,
                (unsigned)aChannelDiag.aRxOvrCount,
                (unsigned)(uint8_t)aChannelDiag.mcpEflg);
            logRing.push(aBuf, millis());
            Serial.println(aBuf);

            const char* twaiStr =
                bChannelDiag.twaiStateCode == 1 ? "OK" :
                bChannelDiag.twaiStateCode == 2 ? "BUS_OFF" : "INIT";
            char bBuf[160];
            snprintf(bBuf, sizeof(bBuf),
                "🔵 [B-CH] RX:%u|Filt:%u|Echo:%u|TxFail:%u|TEC:%u/REC:%u|921:%u|DAS:%u|TWAI:%s",
                (unsigned)bChannelDiag.framesReceivedTotal,
                (unsigned)bChannelDiag.framesFilteredInTotal,
                (unsigned)bChannelDiag.echoCount,
                (unsigned)bChannelDiag.txFail,
                (unsigned)bChannelDiag.twaiTxErrNow,
                (unsigned)bChannelDiag.twaiRxErrNow,
                (unsigned)bChannelDiag.frames921,
                (unsigned)bChannelDiag.dasHandsOnStateRx,
                twaiStr);
            logRing.push(bBuf, millis());
            Serial.println(bBuf);

            uint32_t cur880 = (uint32_t)bChannelDiag.frames880;
            uint32_t cur921 = (uint32_t)bChannelDiag.frames921;
            uint32_t curEcho = (uint32_t)bChannelDiag.echoCount;
            uint32_t curDrop = (uint32_t)bChannelDiag.echoDroppedLate;
            uint32_t curSkipRuntime = (uint32_t)bChannelDiag.skipRuntimeOrInactive;
            uint32_t curSkipHandsOn = (uint32_t)bChannelDiag.skipHandsOn;
            uint32_t curSkipDas = (uint32_t)bChannelDiag.skipDasState;
            uint32_t curNoDas = (uint32_t)bChannelDiag.nagFiredNoDas;
            uint32_t d880 = cur880 - prevLog880;
            uint32_t d921 = cur921 - prevLog921;
            uint32_t dEcho = curEcho - prevLogEcho;
            uint32_t dDrop = curDrop - prevLogDrop;
            uint32_t dSkipRuntime = curSkipRuntime - prevLogSkipRuntime;
            uint32_t dSkipHandsOn = curSkipHandsOn - prevLogSkipHandsOn;
            uint32_t dSkipDas = curSkipDas - prevLogSkipDas;
            uint32_t dNoDas = curNoDas - prevLogNoDas;
            bool nagRuntimeOn = (bool)nagHandlerB.nagKillerActive && (bool)nagKillerRuntime;
            uint8_t intervalDecision = nagIntervalDecision(d880, d921, dEcho, dDrop,
                dSkipRuntime, dSkipHandsOn, dSkipDas, nagRuntimeOn);

            // Verdict는 5초 구간 요약이다. Last는 아래 B-GATE에서 마지막 실제 핸들러 분기로 따로 남긴다.
            char bNag[200];
            snprintf(bNag, sizeof(bNag),
                "🥷 [B-NAG] 5s 880:+%u 921:+%u Echo:+%u Drop:+%u | HO=%u DAS=0x%02X Verdict=%s",
                (unsigned)d880,
                (unsigned)d921,
                (unsigned)dEcho,
                (unsigned)dDrop,
                (unsigned)(uint8_t)bChannelDiag.realHo,
                (unsigned)(uint8_t)bChannelDiag.dasHandsOnStateRx,
                nagDecisionName(intervalDecision));
            logRing.push(bNag, millis());
            Serial.println(bNag);

            char bGate[160];
            snprintf(bGate, sizeof(bGate),
                "🚦 [B-GATE] Skip OFF/HO/DAS:+%u/%u/%u | NoDAS Echo:+%u | Last=%s",
                (unsigned)dSkipRuntime,
                (unsigned)dSkipHandsOn,
                (unsigned)dSkipDas,
                (unsigned)dNoDas,
                nagDecisionName((uint8_t)bChannelDiag.nagLastDecision));
            logRing.push(bGate, millis());
            Serial.println(bGate);

            prevLog880 = cur880;
            prevLog921 = cur921;
            prevLogEcho = curEcho;
            prevLogDrop = curDrop;
            prevLogSkipRuntime = curSkipRuntime;
            prevLogSkipHandsOn = curSkipHandsOn;
            prevLogSkipDas = curSkipDas;
            prevLogNoDas = curNoDas;

            // ── B-CH 심층 진단 라인 (TWAI 카운터 + 에코 품질 + 스킵 사유) ──
            // ArbLost↑    : 동일 ID 충돌 (다른 노드가 같은 ID로 송신)
            // BusErr↑     : Bit/Stuff/CRC/Form/ACK 에러 (배선/상대노드 부재)
            // TxFailed↑   : single-shot 모드 송신 실패 (충돌 후 재전송 금지)
            // RxMissed↑   : RX 큐 오버런 (B루프 폴링 부족)
            // EchoLat     : 마지막 에코 지연 (µs). 6000µs 초과 시 ECU 충돌 위험
            // EchoDrop    : 6ms 초과로 의도적 드롭된 에코 (정상 보호 동작)
            // SkipRT/HO/DAS : 핸들러 진입 후 송신 스킵 사유별 누적
            char bDeep[200];
            snprintf(bDeep, sizeof(bDeep),
                "🔬 [B-DEEP] ArbLost:%u|BusErr:%u|TxFailed:%u|RxMissed:%u|EchoLat:%uus|EchoDrop:%u|Skip RT:%u/HO:%u/DAS:%u",
                (unsigned)bChannelDiag.bArbLost,
                (unsigned)bChannelDiag.bBusError,
                (unsigned)bChannelDiag.bTxFailed,
                (unsigned)bChannelDiag.bRxMissed,
                (unsigned)bChannelDiag.echoLatUs,
                (unsigned)bChannelDiag.echoDroppedLate,
                (unsigned)bChannelDiag.skipRuntimeOrInactive,
                (unsigned)bChannelDiag.skipHandsOn,
                (unsigned)bChannelDiag.skipDasState);
            logRing.push(bDeep, millis());
            Serial.println(bDeep);

            aChannelDiag.lastStatusUpdateMs = millis();
            bChannelDiag.lastStatusUpdateMs = millis();
            lastStatusTime = millis();
        }

        // DEBUG 모드 상세 진단 (kDebugNagKillerEnabled 활성화 시)
        if (kDebugNagKillerEnabled && millis() - lastDebugTime > 5000) {
            char dbg[160];
            snprintf(dbg, sizeof(dbg),
                "🔍 [DBG] B-Other:%u | B-LastID:%u | TWAI-Code:%u | TaskOK:%d | DriverOK:%d",
                (unsigned)bChannelDiag.framesFilteredOutTotal,
                (unsigned)bChannelDiag.frameIdReceived,
                (unsigned)bChannelDiag.twaiStateCode,
                (int)bChannelDiag.nagTaskCreated,
                (int)bChannelDiag.driverBInitialized);
            logRing.push(dbg, millis());
            Serial.println(dbg);
            lastDebugTime = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ── OTA 부팅 체크 ─────────────────────────────────────────────────────────────
// setup() 초입에서 호출. ota_pending 값을 읽어 상태 전이 처리.
// pending 상태 전이표:
//   1 → 2  : 신 FW 첫 부팅, 확인 창 시작
//   2 → 3  : 확인 창 중 재부팅 → 자동 롤백 설정 후 재부팅
//   3 → 4  : 복구 파티션 첫 부팅, 복구 확인 창 시작
//   4 → 5  : 복구 확인 창 중 재부팅 → 복구모드 진입
//   5      : 복구모드 유지
#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
static void otaBootCheck()
{
    nvs_flash_init();  // idempotent
    nvs_handle_t nh;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &nh) != ESP_OK) return;
    uint8_t pending = 0;
    nvs_get_u8(nh, kNvsKeyOtaPending, &pending);

    if (pending == 1) {
        nvs_set_u8(nh, kNvsKeyOtaPending, 2);
        nvs_commit(nh);
        nvs_close(nh);
        Serial.println("[OTA] 신 펌웨어 첫 부팅 → pending=2 (3분 확인 시작)");
    } else if (pending == 2) {
        // 확인 창 중 재부팅 → 자동 롤백
        char fallback[32] = {};
        size_t sz = sizeof(fallback);
        nvs_get_str(nh, kNvsKeyOtaFallback, fallback, &sz);
        nvs_set_u8(nh, kNvsKeyOtaPending, 3);
        nvs_commit(nh);
        nvs_close(nh);
        Serial.printf("[OTA] 확인 중 재부팅 → 롤백 fallback=%s\n", fallback);
        if (strlen(fallback) > 0) {
            const esp_partition_t *prev = esp_partition_find_first(
                ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, fallback);
            if (prev) {
                esp_ota_set_boot_partition(prev);
                Serial.println("[OTA] 롤백 파티션 설정 완료 → 재부팅");
                delay(200);
                esp_restart();
            }
        }
    } else if (pending == 3) {
        nvs_set_u8(nh, kNvsKeyOtaPending, 4);
        nvs_commit(nh);
        nvs_close(nh);
        Serial.println("[OTA] 복구 파티션 부팅 → pending=4 (60초 확인 시작)");
    } else if (pending == 4) {
        nvs_set_u8(nh, kNvsKeyOtaPending, 5);
        nvs_commit(nh);
        nvs_close(nh);
        gOtaRecoveryModeActive = true;
        Serial.println("[OTA] 복구 확인 전 재부팅 → 복구모드 (pending=5)");
    } else if (pending == 5) {
        nvs_close(nh);
        gOtaRecoveryModeActive = true;
        Serial.println("[OTA] 복구모드 유지 (pending=5)");
    } else {
        nvs_close(nh);
    }
}
#endif

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n==================================================");
    Serial.println("  [V16-WEB LOG EDITION] Tesla Nag Killer (EAP Base)");
    Serial.printf("  >> Firmware %s | %s | env=%s\n", FIRMWARE_VERSION, FIRMWARE_BUILD_ID, FIRMWARE_BUILD_ENV);
    Serial.printf("  >> Built %s | git %s/%s dirty=%u source=%s\n",
                  FIRMWARE_BUILD_AT,
                  FIRMWARE_GIT_BRANCH,
                  FIRMWARE_GIT_SHA,
                  (unsigned)(FIRMWARE_GIT_DIRTY != 0),
                  FIRMWARE_SOURCE_HASH);
    Serial.println("==================================================");
    Serial.println("  >> Runtime self-diagnostics disabled in this build");
    Serial.println("==================================================");

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
    // NVS 최초 1회 초기화: 플래시 직후 canmod 네임스페이스를 깨끗하게 시작
    {
        nvs_handle_t initHandle;
        if (nvs_open(kNvsNamespace, NVS_READWRITE, &initHandle) == ESP_OK) {
            uint8_t initDone = 0;
            if (nvs_get_u8(initHandle, "nvs_init_ok", &initDone) == ESP_ERR_NVS_NOT_FOUND) {
                nvs_erase_all(initHandle);
                nvs_set_u8(initHandle, "nvs_init_ok", 1);
                nvs_commit(initHandle);
                Serial.println("[NVS] 최초 부팅: NVS 초기화 완료");
            }
            nvs_close(initHandle);
        }
    }
    otaBootCheck();  // OTA 상태 머신 부팅 시 전이 처리
#endif

#if defined(BOARD_T2CAN)
#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
    loadAExperimentSettings();
#endif

    if (!gOtaRecoveryModeActive) {
    // A채널: MCP2515 (SPI) 초기화
    pinMode(T2CAN_RST_PIN, OUTPUT);
    digitalWrite(T2CAN_RST_PIN, HIGH); delay(50);
    digitalWrite(T2CAN_RST_PIN, LOW);  delay(100);
    digitalWrite(T2CAN_RST_PIN, HIGH); delay(100);

    SPI.begin(T2CAN_SCK, T2CAN_MISO, T2CAN_MOSI, -1);

    auto tempDriverA = std::make_unique<MCP2515Driver>(T2CAN_CS);
    appSetup<MCP2515Driver>(std::move(tempDriverA), "[A-CH] MCP2515 Ready");

    // B채널: TWAI 초기화
    driverB = std::make_unique<TWAIDriver>((gpio_num_t)T2CAN_TX, (gpio_num_t)T2CAN_RX);
    Serial.println("[B-CH] TWAI 드라이버 초기화 시도...");

    if (driverB && driverB->init()) {
        bChannelDiag.driverBInitialized = true;
        Serial.println("🟢✅ [B-CH] TWAI 드라이버 초기화 성공");
        logRing.push("🟢✅ [B-CH] TWAI 드라이버 초기화 성공", millis());

        Serial.println("[B-CH] NagKiller Task 생성 시도...");
        // Core 1 핀: TWAI ISR(설치된 코어=Core 1)과 task 코어 일치 + A채널
        // appLoop 통합 폴링. WiFi/HTTP는 Core 0 단독.
        BaseType_t result = xTaskCreatePinnedToCore(
            nagKillerTask, "NagTask", 8192, nullptr, 10, &nagTaskHandle, 1);

        if (result == pdPASS) {
            bChannelDiag.nagTaskCreated = true;
            Serial.println("🟢✅ [B-CH] NagKiller Task 생성 성공");
            logRing.push("🟢✅ [B-CH] NagKiller Task 생성 성공", millis());
        } else {
            char errBuf[100];
            snprintf(errBuf, sizeof(errBuf),
                "❌ [B-CH] Task 생성 실패! 결과: %d", (int)result);
            Serial.println(errBuf);
            logRing.push(errBuf, millis());
        }
    } else {
        Serial.println("❌ [B-CH] TWAI 드라이버 초기화 실패!");
        logRing.push("❌ [B-CH] TWAI 드라이버 초기화 실패!", millis());
        Serial.printf("   driverB exists: %d, init: %s\n",
            (int)!!driverB, driverB ? "결과 체크" : "nullptr");
    }
    } // end if (!gOtaRecoveryModeActive)

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
    Serial.println("[WEB] 모든 런타임 초기화 완료. 웹 서버 시작");
    logRing.push("[WEB] 모든 런타임 초기화 완료. 웹 서버 시작", millis());
    webServerInit(gOtaRecoveryModeActive ? nullptr : driverB.get());
    if (!gOtaRecoveryModeActive) {
        timeseriesStart();  // 5초 간격 시계열 수집 (최근 30분)
        // [v4.4 ALERT] alert 폴링 태스크 시작 (20ms 주기, Core 0, prio 1)
        gAlertDrv = driverB.get();
        xTaskCreatePinnedToCore(canAlertTask, "alert", 2048, nullptr, 1, nullptr, 0);
        // TWAI는 accept-all로 두고 드라이버/태스크 소프트 필터에서 880/921만 처리한다.
        if (driverB) {
            // 항상 3개 등록: 880 + 921 + 297(Mode B 조향각)
            uint32_t bFiltIds[3] = {kNagFixedTargetId, 921u, 297u};
            driverB->setFilters(bFiltIds, 3);
            char fBuf[64];
            snprintf(fBuf, sizeof(fBuf), "[B-CH] SW 필터 기준: ID %u / 921 / 297", (unsigned)kNagFixedTargetId);
            logRing.push(fBuf, millis());
            Serial.println(fBuf);
        }
    } else {
        Serial.println("[OTA] 복구모드: CAN 비활성, 웹 서버만 시작됨");
    }
#endif
#endif
}

void loop() {
    // 모든 작업은 RTOS 태스크에서 처리. loopTask 즉시 삭제.
    vTaskDelete(NULL);
}



