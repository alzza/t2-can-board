// include/web_server.h
#pragma once

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)

#include <algorithm>
#include <cstring>
#include <memory>
#include <time.h>
#include <WiFi.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <lwip/sockets.h>
#include <cJSON.h>
#include <Update.h>
#include <driver/twai.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>

#include "shared_types.h"
#include "can_helpers.h"
#include "log_buffer.h"
#include "can_diag.h"
#include "timeseries.h"
#include "../event_log.h"
#include "web/web_ui.h"
#include "version.h"
#include "drivers/twai_driver.h"

// B채널 드라이버 포인터 — webServerInit(drv)로 주입
static TWAIDriver* gWebDriverB = nullptr;

static volatile uint32_t gWebStatusReqCount = 0;
static volatile uint32_t gWebStatusLastMs = 0;
static volatile uint32_t gWebStatusLastDurMs = 0;
static volatile uint32_t gWebStatusMaxDurMs = 0;
static volatile uint32_t gWebNagStatsReqCount = 0;
static volatile uint32_t gWebNagStatsLastMs = 0;
static volatile uint32_t gWebNagStatsLastDurMs = 0;
static volatile uint32_t gWebNagStatsMaxDurMs = 0;
static volatile uint32_t gWebLogsBundleReqCount = 0;
static volatile uint32_t gWebLogsBundleLastMs = 0;
static volatile uint32_t gWebLogsBundleLastDurMs = 0;
static volatile uint32_t gWebLogsBundleMaxDurMs = 0;
static volatile uint32_t gWebApStationCount = 0;
static volatile uint32_t gWebApStationChangeCount = 0;
static volatile uint32_t gWebApStationLastChangeMs = 0;

static inline void webHealthMark(volatile uint32_t &count, volatile uint32_t &lastMs, uint32_t now) {
    count = (uint32_t)count + 1;
    lastMs = now;
}

static inline void webHealthRecordDuration(volatile uint32_t &lastDurMs, volatile uint32_t &maxDurMs, uint32_t startMs) {
    uint32_t durationMs = millis() - startMs;
    lastDurMs = durationMs;
    if (durationMs > (uint32_t)maxDurMs) maxDurMs = durationMs;
}

static inline uint32_t webHealthAgeMs(uint32_t now, uint32_t lastMs) {
    return lastMs ? now - lastMs : 0;
}

static inline uint32_t webSafeAgeMs(uint32_t now, uint32_t lastMs) {
    if (!lastMs) return 0;
    uint32_t ageMs = now - lastMs;
    return (ageMs > 0x7FFFFFFFUL) ? 0 : ageMs;
}

static void formatDurationHms(uint32_t durationMs, char *out, size_t out_n);
static void shortBuildId(const char *buildId, char *out, size_t out_n);

static inline uint8_t webHealthSampleApStations(uint32_t now) {
    uint8_t count = WiFi.softAPgetStationNum();
    uint8_t prev = (uint8_t)gWebApStationCount;
    if (count != prev) {
        gWebApStationCount = count;
        gWebApStationChangeCount = (uint32_t)gWebApStationChangeCount + 1;
        gWebApStationLastChangeMs = now;
        char buf[80];
        snprintf(buf, sizeof(buf), "📡 [WEB] AP stations %u -> %u", (unsigned)prev, (unsigned)count);
        logRing.push(buf, now);
    }
    return count;
}

static const char *AP_SSID = "TeslaCAN";
static const char *AP_PASS = "asdf1234"; // WPA2, 최소 8자이상해야됩니다 아니면 무한 재부팅됩니다.
static constexpr uint8_t kApChannel = 1;
static constexpr char kNvsNamespace[] = "canmod";
static constexpr char kNvsKeyIsaSpeedChime[] = "isa_speed_chime";
static constexpr char kNvsKeyEmergencyVehicleDetection[] = "emerg_veh_det";
static constexpr char kNvsKeyEnhancedAutopilot[] = "enh_autopilot";
static constexpr char kNvsKeyNagKiller[] = "nag_killer";
static constexpr char kNvsKeyTsllc[]        = "tsllc";        // TSLLC (스톱사인/초록불 제어)
static constexpr char kNvsKeyAChTx[]        = "a_ch_tx";      // A채널 1021 수정 송신 마스터
static constexpr char kNvsKeyUlcStalkConfirm[] = "ulc_stalk"; // UI_ulcStalkConfirm bit1
static constexpr char kNvsKeyAlcOffHighway[]   = "alc_offhwy";// UI_alcOffHighwayEnable bit56
static constexpr char kNvsKeyASpiMhz[]      = "a_spi_mhz";    // A MCP2515 SPI MHz: 8 or 10
static constexpr char kNvsKeyAOneShot[]     = "a_oneshot";    // A MCP2515 one-shot mode
static constexpr char kNvsKeyATxGuard[]     = "a_tx_guard";   // A TX guard enable
// NagConfig NVS 키 (모두 15자 이하)
static constexpr char kNvsKeyNagMode[]      = "nag_mode";
static constexpr char kNvsKeyNagProfile[]   = "nag_prof";
static constexpr char kNvsKeyNagId[]        = "nag_id";       // compatibility: always 880
static constexpr char kNvsKeyNagTc[]        = "nag_tc";       // uint8: torqueCount
static constexpr char kNvsKeyNagTb2[]       = "nag_tb2";      // bytes: torqueB2[8]
static constexpr char kNvsKeyNagTb3[]       = "nag_tb3";      // bytes: torqueB3[8]
static constexpr char kNvsKeyNagHo[]        = "nag_ho";       // uint8: hoRatePct
static_assert(sizeof(kNvsKeyNagMode) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagProfile) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagId)   - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagTc)   - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagTb2)  - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagTb3)  - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagHo)   - 1 <= 15, "NVS key too long");
static constexpr char kNvsKeyTheme[] = "theme";
static_assert(sizeof(kNvsKeyTsllc) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyAChTx) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyUlcStalkConfirm) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyAlcOffHighway) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyASpiMhz) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyAOneShot) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyATxGuard) - 1 <= 15, "NVS key too long");
static constexpr char kNvsKeyOtaPending[]    = "ota_pending";    // 0=normal 1=written 2=booting
static constexpr char kNvsKeyOtaFallback[]   = "ota_fallback";   // previous partition label
static constexpr char kNvsKeyOtaExpectPart[] = "ota_expect_pt";  // expected new partition label
static constexpr char kNvsKeyBoCool[]        = "bo_cool";        // BUS-OFF 쿨다운 (ms)
static_assert(sizeof(kNvsKeyBoCool)        - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaPending)    - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaFallback)   - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaExpectPart) - 1 <= 15, "NVS key too long");

// OTA 상태 머신 타이밍 상수
static constexpr uint32_t kOtaConfirmWindowMs  = 60000;   // 신 FW 확인 창 (1분)
static constexpr uint32_t kOtaRollbackWindowMs = 60000;   // 복구 확인 창 (1분)
// OTA 상태 머신 전역 변수
static uint32_t gOtaConfirmDeadlineMs  = 0;   // pending==2 일 때 만료 시각 (millis)
static uint32_t gOtaRollbackDeadlineMs = 0;   // pending==4 일 때 만료 시각 (millis)
static bool     gOtaRecoveryModeActive = false; // CAN 비활성 복구모드 플래그

static_assert(sizeof(kNvsKeyTheme) - 1 <= 15, "NVS key too long");

static_assert(sizeof(kNvsKeyIsaSpeedChime) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyEmergencyVehicleDetection) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyEnhancedAutopilot) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagKiller) - 1 <= 15, "NVS key too long");

#if defined(HW4) && defined(ISA_SPEED_CHIME_SUPPRESS)
static constexpr bool kWebSupportsIsaSpeedChimeSuppress = true;
#else
static constexpr bool kWebSupportsIsaSpeedChimeSuppress = false;
#endif

#if defined(HW4) && defined(EMERGENCY_VEHICLE_DETECTION)
static constexpr bool kWebSupportsEmergencyVehicleDetection = true;
#else
static constexpr bool kWebSupportsEmergencyVehicleDetection = false;
#endif

#if defined(ENHANCED_AUTOPILOT)
static constexpr bool kWebSupportsEnhancedAutopilot = true;
#else
static constexpr bool kWebSupportsEnhancedAutopilot = false;
#endif

#if defined(ENHANCED_AUTOPILOT)
static constexpr bool kWebSupportsTsllc = true;
#else
static constexpr bool kWebSupportsTsllc = false;
#endif

#if defined(ENHANCED_AUTOPILOT)
static constexpr bool kWebSupportsUlcStalkConfirm = true;
static constexpr bool kWebSupportsAlcOffHighway = true;
#else
static constexpr bool kWebSupportsUlcStalkConfirm = false;
static constexpr bool kWebSupportsAlcOffHighway = false;
#endif

#if defined(NAG_KILLER)
static constexpr bool kWebSupportsNagKiller = true;
#else
static constexpr bool kWebSupportsNagKiller = false;
#endif

// --- NVS helpers ---

static bool nvsInit()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err == ESP_OK;
}

static bool nvsReadBool(const char *key, bool fallback)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &handle) != ESP_OK)
        return fallback;
    uint8_t val = 0;
    if (nvs_get_u8(handle, key, &val) != ESP_OK)
    {
        nvs_close(handle);
        return fallback;
    }
    nvs_close(handle);
    return val != 0;
}

// nvsReadStr: reads a short string from NVS, returns fallback on error
static void nvsReadStr(const char *key, char *outBuf, size_t outLen, const char *fallback)
{
    strncpy(outBuf, fallback, outLen - 1);
    outBuf[outLen - 1] = '\0';
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &handle) != ESP_OK)
        return;
    size_t needed = outLen;
    if (nvs_get_str(handle, key, outBuf, &needed) != ESP_OK)
        strncpy(outBuf, fallback, outLen - 1);
    outBuf[outLen - 1] = '\0';
    nvs_close(handle);
}

static bool nvsWriteStr(const char *key, const char *val)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK)
        return false;
    esp_err_t err = nvs_set_str(handle, key, val);
    if (err == ESP_OK)
        err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

static bool nvsWriteBool(const char *key, bool enabled)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: open failed for %s (%ld)\n", key, static_cast<long>(err));
        return false;
    }

    err = nvs_set_u8(handle, key, enabled ? 1 : 0);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: set failed for %s (%ld)\n", key, static_cast<long>(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: commit failed for %s (%ld)\n", key, static_cast<long>(err));
        nvs_close(handle);
        return false;
    }
    nvs_close(handle);
    return true;
}

static uint8_t nvsReadU8(const char *key, uint8_t fallback)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &handle) != ESP_OK)
        return fallback;
    uint8_t val = 0;
    if (nvs_get_u8(handle, key, &val) != ESP_OK)
    {
        nvs_close(handle);
        return fallback;
    }
    nvs_close(handle);
    return val;
}

static bool nvsWriteU8(const char *key, uint8_t value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: open failed for %s (%ld)\n", key, static_cast<long>(err));
        return false;
    }

    err = nvs_set_u8(handle, key, value);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: set failed for %s (%ld)\n", key, static_cast<long>(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: commit failed for %s (%ld)\n", key, static_cast<long>(err));
        nvs_close(handle);
        return false;
    }
    nvs_close(handle);
    return true;
}

static uint8_t sanitizeASpiMhz(uint8_t mhz)
{
    return (mhz == 10) ? 10 : 8;
}

static void loadAExperimentSettings()
{
    if (!nvsInit()) return;
    uint8_t defaultMhz = (kAMcpDefaultSpiFreqHz >= 10000000UL) ? 10 : 8;
    uint8_t spiMhz = sanitizeASpiMhz(nvsReadU8(kNvsKeyASpiMhz, defaultMhz));
    aMcpSpiFreqHz = (uint32_t)spiMhz * 1000000UL;
    aMcpRequestedSpiFreqHz = (uint32_t)spiMhz * 1000000UL;
    aChannelTxRuntime = nvsReadBool(kNvsKeyAChTx, true);
    uiUlcStalkConfirmRuntime = nvsReadBool(kNvsKeyUlcStalkConfirm, kUlcStalkConfirmDefaultEnabled);
    uiAlcOffHighwayEnableRuntime = nvsReadBool(kNvsKeyAlcOffHighway, kAlcOffHighwayEnableDefaultEnabled);
    aMcpOneShotRuntime = nvsReadBool(kNvsKeyAOneShot, kAMcpOneShotDefaultEnabled);
    aTxGuardRuntime = nvsReadBool(kNvsKeyATxGuard, kATxGuardDefaultEnabled);
}

// NagConfig NVS 저장/로드 헬퍼
static void nagCfgSave(const NagConfig &c) {
    NagConfig fixed = c;
    nagCfgDefaultsSmart(fixed);
    fixed.smartProfile = nagSmartProfileClamp(c.smartProfile);
    nvs_handle_t h;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h,  kNvsKeyNagMode, fixed.mode);
    nvs_set_u8(h,  kNvsKeyNagProfile, fixed.smartProfile);
    nvs_set_u16(h, kNvsKeyNagId,   fixed.targetId);
    nvs_set_u8(h,  kNvsKeyNagTc,   fixed.torqueCount);
    nvs_set_blob(h, kNvsKeyNagTb2, fixed.torqueB2, kNagMaxTorqueEntries);
    nvs_set_blob(h, kNvsKeyNagTb3, fixed.torqueB3, kNagMaxTorqueEntries);
    nvs_set_u8(h,  kNvsKeyNagHo,   fixed.hoRatePct);
    nvs_commit(h);
    nvs_close(h);
}

static void nagCfgLoad() {
    NagConfig c;
    nagCfgDefaultsSmart(c);
    nvs_handle_t h;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &h) == ESP_OK) {
        uint8_t savedProfile = kNagSmartProfileDefault;
        nvs_get_u8(h, kNvsKeyNagProfile, &savedProfile);
        c.mode = kNagModeB;
        c.smartProfile = nagSmartProfileClamp(savedProfile);
        nvs_close(h);
    }
    portENTER_CRITICAL(&nagCfgMux);
    nagConfig = c;
    portEXIT_CRITICAL(&nagCfgMux);
    bChannelDiag.nagMode = kNagModeB;
    bChannelDiag.smartProfile = c.smartProfile;
}

// --- Rate limiter ---
static unsigned long lastToggleMs = 0;
static const unsigned long kToggleMinIntervalMs = 100;

static bool rateLimitOk()
{
    unsigned long now = millis();
    if (now - lastToggleMs < kToggleMinIntervalMs)
        return false;
    lastToggleMs = now;
    return true;
}

static void addFeatureState(cJSON *parent, const char *name, bool supported, bool enabled, bool buildEnabled)
{
    cJSON *feature = cJSON_AddObjectToObject(parent, name);
    cJSON_AddBoolToObject(feature, "supported", supported);
    cJSON_AddBoolToObject(feature, "enabled", supported && enabled);
    cJSON_AddBoolToObject(feature, "build_enabled", buildEnabled);
}

static bool parseToggleBody(httpd_req_t *req, bool &enabledOut)
{
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return false;
    }
    body[len] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return false;
    }

    cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    if (!cJSON_IsBool(enabled))
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing enabled");
        return false;
    }

    enabledOut = cJSON_IsTrue(enabled);
    cJSON_Delete(json);
    return true;
}

// ─── 부모/자식 기능 종속성 테이블 ─────────────────────────────────────────────────
// 부모 기능이 OFF 될 때 자식 기능을 자동으로 OFF 시킵니다.
static void enforceChildDeps(Shared<bool> *) {} // (자동 차선변경 제거로 현재 종속 관계 없음)

static esp_err_t featureToggleHandler(httpd_req_t *req, Shared<bool> &target, bool supported,
                                      const char *nvsKey, const char *logName)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    if (!supported)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Feature not available");
        return ESP_FAIL;
    }

    bool enabled = false;
    if (!parseToggleBody(req, enabled))
        return ESP_FAIL;

    if (enabled && ((&target == &enhancedAutopilotRuntime) || (&target == &tsllcRuntime) ||
                    (&target == &uiUlcStalkConfirmRuntime) || (&target == &uiAlcOffHighwayEnableRuntime)) &&
        !(bool)aChannelTxRuntime)
    {
        if (!nvsWriteBool(kNvsKeyAChTx, true))
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to enable A channel TX");
            return ESP_FAIL;
        }
        aChannelTxRuntime = true;
        logRing.push("[Web] A_CHANNEL_TX: ON (dependency)", millis());
    }

    if (!nvsWriteBool(nvsKey, enabled))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist setting");
        return ESP_FAIL;
    }
    target = enabled;
    enforceChildDeps(&target); // 부모 OFF 시 자식 자동 종료
    Serial.printf("Web: %s set to %d\n", logName, enabled);
    char buf[64];
    snprintf(buf, sizeof(buf), "[Web] %s: %s", logName, enabled ? "ON" : "OFF");
    logRing.push(buf, millis());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Theme runtime ("dark" / "light"), NVS-backed, default "dark"
static char themeRuntime[8] = "dark";

static void restartTask(void *param)
{
    (void)param;
    vTaskDelay(pdMS_TO_TICKS(750));
    ESP.restart();
}

static esp_err_t rebootHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    Serial.println("Web: manual reboot requested");
    logRing.push("[Web] 보드 재부팅 요청", millis());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// CAN Sniffer  (실시간 CAN 프레임 모니터링)
//

// --- HTTP handlers ---

static esp_err_t rootHandler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    if (gOtaRecoveryModeActive) {
        httpd_resp_send(req, WEB_RECOVERY_UI_HTML, sizeof(WEB_RECOVERY_UI_HTML) - 1);
    } else {
        httpd_resp_send(req, WEB_UI_HTML, sizeof(WEB_UI_HTML) - 1);
    }
    return ESP_OK;
}

static esp_err_t statusHandler(httpd_req_t *req)
{
    const uint32_t handlerStartMs = millis();
    webHealthMark(gWebStatusReqCount, gWebStatusLastMs, handlerStartMs);

    // Parse log_since from query string
    uint32_t logSince = 0;
    char queryBuf[32];
    if (httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf)) == ESP_OK)
    {
        char val[16];
        if (httpd_query_key_value(queryBuf, "log_since", val, sizeof(val)) == ESP_OK)
        {
            logSince = strtoul(val, nullptr, 10);
        }
    }

    // Read handler state (atomic reads -- no lock needed)
    bool fsdEnabled = appHandler ? (bool)appHandler->FSDEnabled : false;
    bool enablePrint = appHandler ? (bool)appHandler->enablePrint : true;
    bool isaSuppress = kWebSupportsIsaSpeedChimeSuppress ? (bool)isaSpeedChimeSuppressRuntime : false;
    bool emergencyVehicleDetection =
        kWebSupportsEmergencyVehicleDetection ? (bool)emergencyVehicleDetectionRuntime : false;
    bool aChannelTx = (bool)aChannelTxRuntime;
    bool enhancedAutopilot =
        kWebSupportsEnhancedAutopilot ? (aChannelTx && (bool)enhancedAutopilotRuntime) : false;
    bool nagKiller = kWebSupportsNagKiller ? (bool)nagKillerRuntime : false;
    bool tsllcEnabled = kWebSupportsTsllc ? (aChannelTx && (bool)tsllcRuntime) : false;
    bool ulcStalkConfirmEnabled =
        kWebSupportsUlcStalkConfirm ? (aChannelTx && (bool)uiUlcStalkConfirmRuntime) : false;
    bool alcOffHighwayEnabled =
        kWebSupportsAlcOffHighway ? (aChannelTx && (bool)uiAlcOffHighwayEnableRuntime) : false;
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        webHealthRecordDuration(gWebStatusLastDurMs, gWebStatusMaxDurMs, handlerStartMs);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    cJSON_AddBoolToObject(root, "fsd_enabled", fsdEnabled);
    cJSON_AddBoolToObject(root, "isa_speed_chime_suppress", isaSuppress);
    cJSON_AddBoolToObject(root, "emergency_vehicle_detection", emergencyVehicleDetection);
    cJSON_AddBoolToObject(root, "enhanced_autopilot", enhancedAutopilot);
    cJSON_AddBoolToObject(root, "nag_killer", nagKiller);
    cJSON_AddBoolToObject(root, "a_channel_tx", aChannelTx);
    cJSON_AddBoolToObject(root, "tsllc_enabled", tsllcEnabled);
    cJSON_AddBoolToObject(root, "ui_ulc_stalk_confirm_enabled", ulcStalkConfirmEnabled);
    cJSON_AddBoolToObject(root, "ui_alc_off_highway_enable_enabled", alcOffHighwayEnabled);
    cJSON_AddBoolToObject(root, "enable_print", enablePrint);

    cJSON_AddStringToObject(root, "theme", themeRuntime);
    cJSON_AddNumberToObject(root, "uptime_ms", millis());
    cJSON_AddNumberToObject(root, "uptime_s", millis() / 1000);
    cJSON_AddStringToObject(root, "firmware_version", FIRMWARE_VERSION);
    cJSON_AddStringToObject(root, "firmware_build_id", FIRMWARE_BUILD_ID);
    char fwShortBuild[32];
    shortBuildId(FIRMWARE_BUILD_ID, fwShortBuild, sizeof(fwShortBuild));
    cJSON_AddStringToObject(root, "firmware_build_short", fwShortBuild);
    cJSON_AddStringToObject(root, "firmware_build_env", FIRMWARE_BUILD_ENV);
    cJSON_AddStringToObject(root, "firmware_build_at", FIRMWARE_BUILD_AT);
    cJSON_AddStringToObject(root, "firmware_git_sha", FIRMWARE_GIT_SHA);
    cJSON_AddStringToObject(root, "firmware_git_branch", FIRMWARE_GIT_BRANCH);
    cJSON_AddStringToObject(root, "firmware_source_hash", FIRMWARE_SOURCE_HASH);
    cJSON_AddBoolToObject(root, "firmware_git_dirty", FIRMWARE_GIT_DIRTY != 0);
#if defined(HW4)
    cJSON_AddStringToObject(root, "hw_handler", "HW4");
#else
    cJSON_AddStringToObject(root, "hw_handler", "HW3");
#endif
    cJSON_AddNumberToObject(root, "log_head", logRing.currentHead());
    {
        uint32_t webNow = millis();
        uint8_t apStationCount = webHealthSampleApStations(webNow);
        cJSON *web = cJSON_AddObjectToObject(root, "web_health");
        if (web) {
            cJSON_AddNumberToObject(web, "status_req_count", (uint32_t)gWebStatusReqCount);
            cJSON_AddNumberToObject(web, "status_last_age_ms", webHealthAgeMs(webNow, (uint32_t)gWebStatusLastMs));
            cJSON_AddNumberToObject(web, "status_last_duration_ms", (uint32_t)gWebStatusLastDurMs);
            cJSON_AddNumberToObject(web, "status_max_duration_ms", (uint32_t)gWebStatusMaxDurMs);
            cJSON_AddNumberToObject(web, "nag_stats_req_count", (uint32_t)gWebNagStatsReqCount);
            cJSON_AddNumberToObject(web, "nag_stats_last_age_ms", webHealthAgeMs(webNow, (uint32_t)gWebNagStatsLastMs));
            cJSON_AddNumberToObject(web, "nag_stats_last_duration_ms", (uint32_t)gWebNagStatsLastDurMs);
            cJSON_AddNumberToObject(web, "nag_stats_max_duration_ms", (uint32_t)gWebNagStatsMaxDurMs);
            cJSON_AddNumberToObject(web, "logs_bundle_req_count", (uint32_t)gWebLogsBundleReqCount);
            cJSON_AddNumberToObject(web, "logs_bundle_last_age_ms", webHealthAgeMs(webNow, (uint32_t)gWebLogsBundleLastMs));
            cJSON_AddNumberToObject(web, "logs_bundle_last_duration_ms", (uint32_t)gWebLogsBundleLastDurMs);
            cJSON_AddNumberToObject(web, "logs_bundle_max_duration_ms", (uint32_t)gWebLogsBundleMaxDurMs);
            cJSON_AddNumberToObject(web, "free_heap", esp_get_free_heap_size());
            cJSON_AddNumberToObject(web, "min_free_heap", esp_get_minimum_free_heap_size());
            cJSON_AddNumberToObject(web, "ap_station_count", apStationCount);
            cJSON_AddNumberToObject(web, "ap_station_change_count", (uint32_t)gWebApStationChangeCount);
            cJSON_AddNumberToObject(web, "ap_station_last_change_age_ms", webHealthAgeMs(webNow, (uint32_t)gWebApStationLastChangeMs));
        }
    }
    // OTA 상태 머신 전체 필드
    {
        uint8_t otaPending = 0;
        nvs_handle_t nh;
        if (nvs_open(kNvsNamespace, NVS_READONLY, &nh) == ESP_OK) {
            nvs_get_u8(nh, kNvsKeyOtaPending, &otaPending);
            nvs_close(nh);
        }
        cJSON_AddNumberToObject(root, "ota_pending_state", otaPending);
        cJSON_AddBoolToObject(root, "ota_pending_verify",        otaPending == 2);
        cJSON_AddBoolToObject(root, "ota_rollback_confirm_pending", otaPending == 4);
        cJSON_AddBoolToObject(root, "ota_recovery_mode",         otaPending == 5);
        uint32_t now = millis();
        int32_t confirmRem = (otaPending == 2 && gOtaConfirmDeadlineMs > 0)
            ? (int32_t)(gOtaConfirmDeadlineMs - now) : 0;
        if (confirmRem < 0) confirmRem = 0;
        cJSON_AddNumberToObject(root, "ota_confirm_remaining_ms",  confirmRem);
        cJSON_AddNumberToObject(root, "ota_confirm_window_ms",     kOtaConfirmWindowMs);
        int32_t rollbackRem = (otaPending == 4 && gOtaRollbackDeadlineMs > 0)
            ? (int32_t)(gOtaRollbackDeadlineMs - now) : 0;
        if (rollbackRem < 0) rollbackRem = 0;
        cJSON_AddNumberToObject(root, "ota_rollback_remaining_ms", rollbackRem);
        cJSON_AddNumberToObject(root, "ota_rollback_window_ms",    kOtaRollbackWindowMs);
        // 현재 파티션 레이블
        const esp_partition_t *runPart = esp_ota_get_running_partition();
        cJSON_AddStringToObject(root, "ota_current_label", runPart ? runPart->label : "");
        // fallback 레이블
        char fallback[32] = {};
        nvs_handle_t fnh;
        if (nvs_open(kNvsNamespace, NVS_READONLY, &fnh) == ESP_OK) {
            size_t sz = sizeof(fallback);
            nvs_get_str(fnh, kNvsKeyOtaFallback, fallback, &sz);
            nvs_close(fnh);
        }
        cJSON_AddStringToObject(root, "ota_fallback_label", fallback);
    }

    cJSON *features = cJSON_AddObjectToObject(root, "features");

    addFeatureState(features, "isa_speed_chime_suppress",
                    kWebSupportsIsaSpeedChimeSuppress, isaSuppress, kIsaSpeedChimeSuppressBuildEnabled);
    addFeatureState(features, "emergency_vehicle_detection",
                    kWebSupportsEmergencyVehicleDetection, emergencyVehicleDetection,
                    kEmergencyVehicleDetectionBuildEnabled);
    addFeatureState(features, "enhanced_autopilot",
                    kWebSupportsEnhancedAutopilot, enhancedAutopilot,
                    kEnhancedAutopilotBuildEnabled);
    addFeatureState(features, "nag_killer", kWebSupportsNagKiller, nagKiller, kNagKillerBuildEnabled);
    addFeatureState(features, "a_channel_tx", true, aChannelTx, true);
    // TSLLC: 스톱사인/신호등 자동 정지 + 앞차 있을 때 초록불 자동 출발
    addFeatureState(features, "tsllc_enabled", kWebSupportsTsllc, tsllcEnabled, kTsllcBuildEnabled);
    addFeatureState(features, "ui_ulc_stalk_confirm", kWebSupportsUlcStalkConfirm,
                    ulcStalkConfirmEnabled, kUlcStalkConfirmBuildEnabled);
    addFeatureState(features, "ui_alc_off_highway_enable", kWebSupportsAlcOffHighway,
                    alcOffHighwayEnabled, kAlcOffHighwayEnableBuildEnabled);
    addFeatureState(features, "a_spi_8mhz", true, (uint32_t)aMcpRequestedSpiFreqHz <= 8000000UL, true);
    addFeatureState(features, "a_mcp_oneshot", true, (bool)aMcpOneShotRuntime, true);
    addFeatureState(features, "a_tx_guard", true, (bool)aTxGuardRuntime, true);
    addFeatureState(features, "ota", true, false, true);

    // Add log entries since last poll
    // 폴링 1회당 최대 32개로 제한 (logRing kCapacity=256은 logs-bundle 다운로드 전용)
    constexpr int kStatusLogMax = 32;
    LogRingBuffer::Entry logEntries[kStatusLogMax];
    int logCount = logRing.readSince(logSince, logEntries, kStatusLogMax);
    cJSON *logs = cJSON_AddArrayToObject(root, "logs");
    for (int i = 0; i < logCount; i++)
    {
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddStringToObject(entry, "msg", logEntries[i].msg);
        cJSON_AddNumberToObject(entry, "ts", logEntries[i].timestamp_ms);
        cJSON_AddItemToArray(logs, entry);
    }

    // 👇 [추가] A/B 채널 진단 정보 (통합 대시보드)
    cJSON *channels = cJSON_AddObjectToObject(root, "channels");
    
    // A/B 채널 ID별 주기 계산 (ms, 1초마다 갱신)
    static uint32_t idRateLastMs = 0;
    static uint32_t lastFrames1016 = 0;
    static uint32_t lastFrames1021 = 0;
    static uint32_t lastFrames880 = 0;
    static uint32_t lastFrames921 = 0;
    static uint32_t lastFrames923 = 0;
    static uint32_t lastFrames297 = 0;
    static uint32_t period1016Ms = 0;
    static uint32_t period1021Ms = 0;
    static uint32_t period880Ms = 0;
    static uint32_t period921Ms = 0;
    static uint32_t period923Ms = 0;
    static uint32_t period297Ms = 0;
    uint16_t nagTargetId = kNagFixedTargetId;
    {
        uint32_t now = millis();
        uint32_t elapsed = now - idRateLastMs;
        if (elapsed >= 1000) {
            const uint32_t cur1016 = (uint32_t)aChannelDiag.frames1016;
            const uint32_t cur1021 = (uint32_t)aChannelDiag.frames1021;
            const uint32_t cur880 = (uint32_t)bChannelDiag.frames880;
            const uint32_t cur921 = (uint32_t)bChannelDiag.frames921;
            const uint32_t cur923 = (uint32_t)bChannelDiag.frames923;
            const uint32_t cur297 = (uint32_t)bChannelDiag.frames297;

            const uint32_t d1016 = cur1016 - lastFrames1016;
            const uint32_t d1021 = cur1021 - lastFrames1021;
            const uint32_t d880  = cur880  - lastFrames880;
            const uint32_t d921  = cur921  - lastFrames921;
            const uint32_t d923  = cur923  - lastFrames923;
            const uint32_t d297  = cur297  - lastFrames297;

            period1016Ms = (d1016 > 0) ? (elapsed / d1016) : 0;
            period1021Ms = (d1021 > 0) ? (elapsed / d1021) : 0;
            period880Ms  = (d880  > 0) ? (elapsed / d880)  : 0;
            period921Ms  = (d921  > 0) ? (elapsed / d921)  : 0;
            period923Ms  = (d923  > 0) ? (elapsed / d923)  : 0;
            period297Ms  = (d297  > 0) ? (elapsed / d297)  : 0;

            lastFrames1016 = cur1016;
            lastFrames1021 = cur1021;
            lastFrames880 = cur880;
            lastFrames921 = cur921;
            lastFrames923 = cur923;
            lastFrames297 = cur297;
            idRateLastMs = now;
        }
    }

    const uint32_t statusNowMs = millis();

    cJSON *ach = cJSON_AddObjectToObject(channels, "a_channel");
    cJSON_AddNumberToObject(ach, "frames_received", (uint32_t)aChannelDiag.framesReceivedTotal);
    cJSON_AddNumberToObject(ach, "frame_hz", (double)(float)aChannelDiag.frameHz);
    cJSON_AddNumberToObject(ach, "frames_1016", (uint32_t)aChannelDiag.frames1016);
    cJSON_AddNumberToObject(ach, "id_1016_period_ms", (uint32_t)period1016Ms);
    cJSON_AddNumberToObject(ach, "frames_1021", (uint32_t)aChannelDiag.frames1021);
    cJSON_AddNumberToObject(ach, "id_1021_period_ms", (uint32_t)period1021Ms);
    cJSON_AddNumberToObject(ach, "eap_modified", (uint32_t)aChannelDiag.eapModifiedCount);
    cJSON_AddNumberToObject(ach, "ulc_stalk_confirm_modified", (uint32_t)aChannelDiag.ulcStalkConfirmModifiedCount);
    cJSON_AddNumberToObject(ach, "ulc_stalk_confirm_skipped", (uint32_t)aChannelDiag.ulcStalkConfirmSkipCount);
    cJSON_AddNumberToObject(ach, "alc_off_highway_modified", (uint32_t)aChannelDiag.alcOffHighwayModifiedCount);
    cJSON_AddNumberToObject(ach, "alc_off_highway_skipped", (uint32_t)aChannelDiag.alcOffHighwaySkipCount);
    cJSON_AddNumberToObject(ach, "last_frame_id", (uint32_t)aChannelDiag.lastFrameIdReceived);
    cJSON_AddNumberToObject(ach, "last_update_ms", (uint32_t)aChannelDiag.lastStatusUpdateMs);
    cJSON_AddNumberToObject(ach, "last_loop_ms", (uint32_t)aChannelDiag.lastLoopMs);
    cJSON_AddNumberToObject(ach, "core_id", (int32_t)aChannelDiag.loopCoreId);
    cJSON_AddNumberToObject(ach, "spi_freq_hz", (uint32_t)aMcpSpiFreqHz);
    cJSON_AddNumberToObject(ach, "spi_requested_hz", (uint32_t)aMcpRequestedSpiFreqHz);
    cJSON_AddBoolToObject(ach, "spi_reboot_required", (uint32_t)aMcpSpiFreqHz != (uint32_t)aMcpRequestedSpiFreqHz);
    cJSON_AddBoolToObject(ach, "mcp_one_shot", (bool)aMcpOneShotRuntime);
    cJSON_AddBoolToObject(ach, "channel_tx_enabled", (bool)aChannelTxRuntime);
    cJSON_AddBoolToObject(ach, "ui_ulc_stalk_confirm_enabled", ulcStalkConfirmEnabled);
    cJSON_AddBoolToObject(ach, "ui_alc_off_highway_enable_enabled", alcOffHighwayEnabled);
    cJSON_AddBoolToObject(ach, "tx_guard_enabled", (bool)aTxGuardRuntime);
    // MCP2515 에러 플래그 (EFLG 레지스터, 5초 폴링)
    cJSON_AddNumberToObject(ach, "mcp_eflg", (uint32_t)aChannelDiag.mcpEflg);
    cJSON_AddNumberToObject(ach, "mcp_eflg_peak", (uint32_t)aChannelDiag.mcpEflgPeak);
    cJSON_AddNumberToObject(ach, "mcp_txbo_count", (uint32_t)aChannelDiag.mcpTxBoCount);
    cJSON_AddNumberToObject(ach, "mcp_recovery_attempt", (uint32_t)aChannelDiag.mcpRecoveryAttemptCount);
    cJSON_AddNumberToObject(ach, "mcp_recovery_success", (uint32_t)aChannelDiag.mcpRecoverySuccessCount);
    cJSON_AddNumberToObject(ach, "mcp_recovery_fail", (uint32_t)aChannelDiag.mcpRecoveryFailCount);
    cJSON_AddNumberToObject(ach, "mcp_busoff_since_ms", (uint32_t)aChannelDiag.mcpBusOffSinceMs);
    cJSON_AddNumberToObject(ach, "mcp_last_recovery_ms", (uint32_t)aChannelDiag.mcpLastRecoveryMs);
    // A채널 송수신 진단 카운터 (5초 폴링 + handler 송신 결과)
    cJSON_AddNumberToObject(ach, "tx_ok",   (uint32_t)aChannelDiag.aTxOk);
    cJSON_AddNumberToObject(ach, "tx_fail", (uint32_t)aChannelDiag.aTxFail);
    cJSON_AddNumberToObject(ach, "tec",      (uint32_t)(uint8_t)aChannelDiag.aTec);
    cJSON_AddNumberToObject(ach, "rec",      (uint32_t)(uint8_t)aChannelDiag.aRec);
    cJSON_AddNumberToObject(ach, "tec_peak", (uint32_t)(uint8_t)aChannelDiag.aTecPeak);
    cJSON_AddNumberToObject(ach, "merrf",    (uint32_t)aChannelDiag.aMerrfCount);
    cJSON_AddNumberToObject(ach, "rx_ovr",   (uint32_t)aChannelDiag.aRxOvrCount);
    cJSON_AddNumberToObject(ach, "rec_peak",         (uint32_t)(uint8_t)aChannelDiag.aRecPeak);
    cJSON_AddNumberToObject(ach, "last_frame_rx_ms", (uint32_t)aChannelDiag.lastFrameRxMs);
    cJSON_AddNumberToObject(ach, "last_tx_ms",       (uint32_t)aChannelDiag.lastTxMs);
    cJSON_AddNumberToObject(ach, "eflg_event_count", (uint32_t)aChannelDiag.mcpEflgEventCount);
    uint32_t aGuardUntil = (uint32_t)aChannelDiag.aTxGuardUntilMs;
    uint32_t aNow = millis();
    bool aGuardActive = aTxGuardActive(aNow);
    cJSON_AddBoolToObject(ach, "tx_guard_active", aGuardActive);
    cJSON_AddNumberToObject(ach, "tx_guard_remaining_ms", aGuardActive ? aGuardUntil - aNow : 0);
    cJSON_AddNumberToObject(ach, "tx_guard_count", (uint32_t)aChannelDiag.aTxGuardCount);
    cJSON_AddNumberToObject(ach, "tx_guard_skip", (uint32_t)aChannelDiag.aTxGuardSkipCount);
    cJSON_AddStringToObject(ach, "tx_guard_reason", aTxGuardReasonName((uint8_t)aChannelDiag.aTxGuardLastReason));
    const uint32_t aLastFrameMs = (uint32_t)aChannelDiag.lastFrameRxMs;
    const uint32_t aLastLoopMs = (uint32_t)aChannelDiag.lastLoopMs;
    const uint32_t aFrameAgeMs = webSafeAgeMs(statusNowMs, aLastFrameMs);
    const uint32_t aLoopAgeMs = webSafeAgeMs(statusNowMs, aLastLoopMs);
    const bool aFresh = (aLastFrameMs > 0) && (aFrameAgeMs <= 2000);
    const bool aTaskAlive = (aLastLoopMs > 0) && (aLoopAgeMs <= 2000);
    const bool aDriverOk = (appHandler != nullptr);
    const bool aConnected = aDriverOk && aFresh && (((uint8_t)aChannelDiag.mcpEflg & 0x20) == 0);
    cJSON_AddBoolToObject(ach, "driver_ok", aDriverOk);
    cJSON_AddBoolToObject(ach, "connected", aConnected);
    cJSON_AddBoolToObject(ach, "fresh", aFresh);
    cJSON_AddBoolToObject(ach, "task_alive", aTaskAlive);
    cJSON_AddNumberToObject(ach, "frame_age_ms", aFrameAgeMs);
    cJSON_AddNumberToObject(ach, "loop_age_ms", aLoopAgeMs);
    
    cJSON *bch = cJSON_AddObjectToObject(channels, "b_channel");
    cJSON_AddNumberToObject(bch, "frames_received", (uint32_t)bChannelDiag.framesReceivedTotal);
    cJSON_AddNumberToObject(bch, "frame_hz", (double)(float)bChannelDiag.frameHz);
    cJSON_AddNumberToObject(bch, "filtered_hz", (double)(float)bChannelDiag.filteredHz);
    cJSON_AddNumberToObject(bch, "frames_880", (uint32_t)bChannelDiag.frames880);
    cJSON_AddNumberToObject(bch, "frames_target", (uint32_t)bChannelDiag.frames880);
    cJSON_AddNumberToObject(bch, "frames_921", (uint32_t)bChannelDiag.frames921);
    cJSON_AddNumberToObject(bch, "frames_923", (uint32_t)bChannelDiag.frames923);
    cJSON_AddNumberToObject(bch, "frames_297", (uint32_t)bChannelDiag.frames297);
    cJSON_AddNumberToObject(bch, "target_id", (uint32_t)nagTargetId);
    cJSON_AddNumberToObject(bch, "id_880_period_ms", (uint32_t)period880Ms);
    cJSON_AddNumberToObject(bch, "id_target_period_ms", (uint32_t)period880Ms);
    cJSON_AddNumberToObject(bch, "id_921_period_ms", (uint32_t)period921Ms);
    cJSON_AddNumberToObject(bch, "id_923_period_ms", (uint32_t)period923Ms);
    cJSON_AddNumberToObject(bch, "id_297_period_ms", (uint32_t)period297Ms);
    cJSON_AddNumberToObject(bch, "das_hands_state", (uint32_t)bChannelDiag.dasHandsOnStateRx);
    cJSON_AddNumberToObject(bch, "das_source_id", (uint32_t)bChannelDiag.dasStatusSourceId);
    cJSON_AddNumberToObject(bch, "last_das_status_rx_ms", (uint32_t)bChannelDiag.lastDasStatusRxMs);
    cJSON_AddNumberToObject(bch, "nag_mode", (uint32_t)kNagModeB);
    cJSON_AddNumberToObject(bch, "smart_profile", (uint32_t)nagSmartProfileClamp((uint8_t)bChannelDiag.smartProfile));
    cJSON_AddNumberToObject(bch, "echo_count", (uint32_t)bChannelDiag.echoCount);
    cJSON_AddNumberToObject(bch, "echo_drop_late", (uint32_t)bChannelDiag.echoDroppedLate);
    cJSON_AddNumberToObject(bch, "skip_runtime_or_inactive", (uint32_t)bChannelDiag.skipRuntimeOrInactive);
    cJSON_AddNumberToObject(bch, "skip_ap_state", (uint32_t)bChannelDiag.skipApState);
    cJSON_AddNumberToObject(bch, "skip_hands_on", (uint32_t)bChannelDiag.skipHandsOn);
    cJSON_AddNumberToObject(bch, "skip_das_state", (uint32_t)bChannelDiag.skipDasState);
    cJSON_AddNumberToObject(bch, "twai_state_code", (uint32_t)bChannelDiag.twaiStateCode);
    cJSON_AddNumberToObject(bch, "last_frame_id", (uint32_t)bChannelDiag.frameIdReceived);
    cJSON_AddNumberToObject(bch, "last_update_ms", (uint32_t)bChannelDiag.lastStatusUpdateMs);
    cJSON_AddNumberToObject(bch, "last_frame_rx_ms", (uint32_t)bChannelDiag.lastFrameRxMs);
    cJSON_AddNumberToObject(bch, "last_loop_ms", (uint32_t)bChannelDiag.lastLoopMs);
    cJSON_AddNumberToObject(bch, "core_id", (int32_t)bChannelDiag.taskCoreId);
    cJSON_AddNumberToObject(bch, "busoff_count", (uint32_t)bChannelDiag.busoffCount);
    cJSON_AddNumberToObject(bch, "recovery_attempt_count", (uint32_t)bChannelDiag.recoveryAttemptCount);
    cJSON_AddNumberToObject(bch, "recovery_success_count", (uint32_t)bChannelDiag.recoverySuccessCount);
    cJSON_AddNumberToObject(bch, "recovery_fail_count", (uint32_t)bChannelDiag.recoveryFailCount);
    cJSON_AddNumberToObject(bch, "last_busoff_ms", (uint32_t)bChannelDiag.lastBusoffMs);
    cJSON_AddNumberToObject(bch, "last_recovery_start_ms", (uint32_t)bChannelDiag.lastRecoveryStartMs);
    cJSON_AddNumberToObject(bch, "last_recovery_done_ms", (uint32_t)bChannelDiag.lastRecoveryDoneMs);
    cJSON_AddNumberToObject(bch, "last_recovery_duration_ms", (uint32_t)bChannelDiag.lastRecoveryDurationMs);
    cJSON_AddNumberToObject(bch, "twai_rx_err_peak", (uint32_t)bChannelDiag.twaiRxErrPeak);
    cJSON_AddNumberToObject(bch, "twai_tx_err_peak", (uint32_t)bChannelDiag.twaiTxErrPeak);
    // B채널 심층 진단: TWAI 누적 카운터 (UI 타일 + 가설 판별용)
    cJSON_AddNumberToObject(bch, "arb_lost",   (uint32_t)bChannelDiag.bArbLost);
    cJSON_AddNumberToObject(bch, "bus_error",  (uint32_t)bChannelDiag.bBusError);
    cJSON_AddNumberToObject(bch, "tx_failed",  (uint32_t)bChannelDiag.bTxFailed);
    cJSON_AddNumberToObject(bch, "rx_missed",  (uint32_t)bChannelDiag.bRxMissed);

    const uint32_t bLastFrameMs = (uint32_t)bChannelDiag.lastFrameRxMs;
    const uint32_t bLastLoopMs = (uint32_t)bChannelDiag.lastLoopMs;
    const uint32_t bFrameAgeMs = webSafeAgeMs(statusNowMs, bLastFrameMs);
    const uint32_t bLoopAgeMs = webSafeAgeMs(statusNowMs, bLastLoopMs);
    const bool bFresh = (bLastFrameMs > 0) && (bFrameAgeMs <= 2000);
    const bool bTaskAlive = (bLastLoopMs > 0) && (bLoopAgeMs <= 2000);
    const uint32_t twaiStateCode = (uint32_t)bChannelDiag.twaiStateCode;
    const bool bDriverOk = gWebDriverB && gWebDriverB->isDriverOK();
    const bool bConnected = (bool)bChannelDiag.nagTaskCreated &&
                            bDriverOk &&
                            ((bool)bChannelDiag.twaiConnected || twaiStateCode == 1 || twaiStateCode == 3) &&
                            twaiStateCode != 2 &&
                            bFresh;
    cJSON_AddBoolToObject(bch, "driver_initialized", (bool)bChannelDiag.driverBInitialized);
    cJSON_AddBoolToObject(bch, "driver_ok", bDriverOk);
    cJSON_AddNumberToObject(bch, "driver_install_err", gWebDriverB ? gWebDriverB->getLastInstallErr() : -1);
    cJSON_AddNumberToObject(bch, "driver_start_err", gWebDriverB ? gWebDriverB->getLastStartErr() : -1);
    cJSON_AddBoolToObject(bch, "can_task_created", (bool)bChannelDiag.nagTaskCreated);
    cJSON_AddBoolToObject(bch, "connected", bConnected);
    cJSON_AddBoolToObject(bch, "fresh", bFresh);
    cJSON_AddBoolToObject(bch, "task_alive", bTaskAlive);
    cJSON_AddNumberToObject(bch, "frame_age_ms", bFrameAgeMs);
    cJSON_AddNumberToObject(bch, "loop_age_ms", bLoopAgeMs);

    const uint32_t aFramesReceived = (uint32_t)aChannelDiag.framesReceivedTotal;
    const uint32_t bFramesReceived = (uint32_t)bChannelDiag.framesReceivedTotal;
    const uint32_t totalFramesReceived = aFramesReceived + bFramesReceived;

    // CAN bus diagnostics
    twai_status_info_t twaiStatus;
    cJSON *can = cJSON_AddObjectToObject(root, "can");
    if (twai_get_status_info(&twaiStatus) == ESP_OK)
    {
        const char *stateStr = "UNKNOWN";
        switch (twaiStatus.state)
        {
        case TWAI_STATE_STOPPED:
            stateStr = "STOPPED";
            break;
        case TWAI_STATE_RUNNING:
            stateStr = "RUNNING";
            break;
        case TWAI_STATE_BUS_OFF:
            stateStr = "BUS_OFF";
            break;
        case TWAI_STATE_RECOVERING:
            stateStr = "RECOVERING";
            break;
        }
        cJSON_AddStringToObject(can, "state", stateStr);
        cJSON_AddNumberToObject(can, "rx_errors", twaiStatus.rx_error_counter);
        cJSON_AddNumberToObject(can, "tx_errors", twaiStatus.tx_error_counter);
        cJSON_AddNumberToObject(can, "bus_errors", twaiStatus.bus_error_count);
        cJSON_AddNumberToObject(can, "rx_missed", twaiStatus.rx_missed_count);
        cJSON_AddNumberToObject(can, "rx_queued", twaiStatus.msgs_to_rx);
    }
    else
    {
        cJSON_AddStringToObject(can, "state", "NO_DRIVER");
    }
    cJSON_AddNumberToObject(can, "frames_received", totalFramesReceived);
    cJSON_AddNumberToObject(can, "frames_received_a", aFramesReceived);
    cJSON_AddNumberToObject(can, "frames_received_b", bFramesReceived);
    cJSON_AddNumberToObject(can, "frames_sent",
                            appHandler ? (uint32_t)appHandler->framesSent : 0);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        webHealthRecordDuration(gWebStatusLastDurMs, gWebStatusMaxDurMs, handlerStartMs);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    esp_err_t sendRet = httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    webHealthRecordDuration(gWebStatusLastDurMs, gWebStatusMaxDurMs, handlerStartMs);
    return sendRet;
}

static esp_err_t isaSpeedChimeSuppressHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, isaSpeedChimeSuppressRuntime,
                                kWebSupportsIsaSpeedChimeSuppress,
                                kNvsKeyIsaSpeedChime, "ISA_SPEED_CHIME_SUPPRESS");
}

static esp_err_t emergencyVehicleDetectionHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, emergencyVehicleDetectionRuntime,
                                kWebSupportsEmergencyVehicleDetection,
                                kNvsKeyEmergencyVehicleDetection, "EMERGENCY_VEHICLE_DETECTION");
}

static esp_err_t enhancedAutopilotHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, enhancedAutopilotRuntime,
                                kWebSupportsEnhancedAutopilot,
                                kNvsKeyEnhancedAutopilot, "ENHANCED_AUTOPILOT");
}

static esp_err_t nagKillerHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, nagKillerRuntime, kWebSupportsNagKiller, kNvsKeyNagKiller, "NAG_KILLER");
}


static esp_err_t tsllcHandler(httpd_req_t *req)
{
    // TSLLC 토글: 스톱사인/신호등 자동 정지 + 앞차 있을 때 초록불 자동 출발 (ID 1021 Mux0 주입)
    return featureToggleHandler(req, tsllcRuntime, kWebSupportsTsllc, kNvsKeyTsllc, "TSLLC");
}

static esp_err_t uiUlcStalkConfirmHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, uiUlcStalkConfirmRuntime, kWebSupportsUlcStalkConfirm,
                                kNvsKeyUlcStalkConfirm, "UI_ulcStalkConfirm");
}

static esp_err_t uiAlcOffHighwayHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, uiAlcOffHighwayEnableRuntime, kWebSupportsAlcOffHighway,
                                kNvsKeyAlcOffHighway, "UI_alcOffHighwayEnable");
}

static esp_err_t aChannelTxHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, aChannelTxRuntime, true, kNvsKeyAChTx, "A_CHANNEL_TX");
}

static void applyAExperimentToDriver()
{
    if (appDriver) appDriver->applyRuntimeSettings();
}

static esp_err_t aSpi8MhzHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    bool enabled = false;
    if (!parseToggleBody(req, enabled)) return ESP_FAIL;
    uint8_t spiMhz = enabled ? 8 : 10;
    if (!nvsWriteU8(kNvsKeyASpiMhz, spiMhz)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist setting");
        return ESP_FAIL;
    }
    aMcpRequestedSpiFreqHz = (uint32_t)spiMhz * 1000000UL;
    char buf[96];
    snprintf(buf, sizeof(buf), "[Web] A SPI clock target: %uMHz (reboot required)", (unsigned)spiMhz);
    logRing.push(buf, millis());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t aOneShotHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    bool enabled = false;
    if (!parseToggleBody(req, enabled)) return ESP_FAIL;
    if (!nvsWriteBool(kNvsKeyAOneShot, enabled)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist setting");
        return ESP_FAIL;
    }
    aMcpOneShotRuntime = enabled;
    applyAExperimentToDriver();
    char buf[80];
    snprintf(buf, sizeof(buf), "[Web] A MCP2515 mode: %s", enabled ? "One-Shot" : "Normal");
    logRing.push(buf, millis());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t aTxGuardHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    bool enabled = false;
    if (!parseToggleBody(req, enabled)) return ESP_FAIL;
    if (!nvsWriteBool(kNvsKeyATxGuard, enabled)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist setting");
        return ESP_FAIL;
    }
    aTxGuardRuntime = enabled;
    if (!enabled) aChannelDiag.aTxGuardUntilMs = 0;
    char buf[80];
    snprintf(buf, sizeof(buf), "[Web] A TX guard: %s", enabled ? "ON" : "OFF");
    logRing.push(buf, millis());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ─── GET /api/nag-config  ────────────────────────────────────────────────────
// 현재 NagConfig를 JSON으로 반환 (v2 /api/config 에 해당)
static void addNagProfileJson(cJSON *root, const NagSmartProfileSettings &profile)
{
    cJSON_AddNumberToObject(root, "smartProfile", profile.id);
    cJSON_AddStringToObject(root, "profileLabel", profile.label);
    cJSON_AddStringToObject(root, "profileSummary", profile.summary);
    cJSON_AddNumberToObject(root, "state1GraceMs", profile.state1GraceMs);
    cJSON_AddNumberToObject(root, "state2DelayMs", profile.state2DelayMs);
    cJSON_AddNumberToObject(root, "strongDelayMs", profile.strongDelayMs);
    cJSON_AddNumberToObject(root, "strongRampMs", profile.strongRampMs);
    cJSON_AddNumberToObject(root, "state2MildMinRawDelta", profile.state2MildMinRawDelta);
    cJSON_AddNumberToObject(root, "state2MildMaxRawDelta", profile.state2MildMaxRawDelta);
    cJSON_AddNumberToObject(root, "state2MildMinNm", (double)profile.state2MildMinRawDelta * 0.01);
    cJSON_AddNumberToObject(root, "state2MildMaxNm", (double)profile.state2MildMaxRawDelta * 0.01);
    cJSON_AddNumberToObject(root, "state2BurstMs", profile.state2BurstMs);
    cJSON_AddNumberToObject(root, "state2PauseMs", profile.state2PauseMs);
    cJSON_AddNumberToObject(root, "strongBurstMs", profile.strongBurstMs);
    cJSON_AddNumberToObject(root, "strongPauseMs", profile.strongPauseMs);
}

static esp_err_t nagConfigGetHandler(httpd_req_t *req)
{
    NagConfig c;
    portENTER_CRITICAL(&nagCfgMux);
    c = nagConfig;
    portEXIT_CRITICAL(&nagCfgMux);
    c.mode = kNagModeB;
    c.smartProfile = nagSmartProfileClamp(c.smartProfile);
    const NagSmartProfileSettings &profile = nagSmartProfileSettings(c.smartProfile);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "mode",       c.mode);
    cJSON_AddStringToObject(root, "modeStr",    "SMART");
    addNagProfileJson(root, profile);
    cJSON_AddNumberToObject(root, "targetId",   c.targetId);
    cJSON_AddNumberToObject(root, "hoRatePct",  c.hoRatePct);
    cJSON *arr = cJSON_AddArrayToObject(root, "torque");
    for (uint8_t i = 0; i < c.torqueCount; i++) {
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(entry, "b2", c.torqueB2[i]);
        cJSON_AddNumberToObject(entry, "b3", c.torqueB3[i]);
        uint16_t raw = static_cast<uint16_t>(((c.torqueB2[i] & 0x0F) << 8) | c.torqueB3[i]);
        cJSON_AddNumberToObject(entry, "nm", raw * 0.01f - 20.5f);
        cJSON_AddItemToArray(arr, entry);
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM"); return ESP_FAIL; }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return ESP_OK;
}

// ─── GET /api/nag-stats  ─────────────────────────────────────────────────────
// 실시간 B채널 나그킬러 stats (v2 /api/stats 에 해당)
static esp_err_t nagStatsGetHandler(httpd_req_t *req)
{
    const uint32_t handlerStartMs = millis();
    webHealthMark(gWebNagStatsReqCount, gWebNagStatsLastMs, handlerStartMs);
    uint32_t nowMs = handlerStartMs;
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        webHealthRecordDuration(gWebNagStatsLastDurMs, gWebNagStatsMaxDurMs, handlerStartMs);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    cJSON_AddNumberToObject(root, "rx",       (uint32_t)bChannelDiag.frames880);
    cJSON_AddNumberToObject(root, "echo",     (uint32_t)bChannelDiag.echoCount);
    cJSON_AddNumberToObject(root, "txFail",   (uint32_t)bChannelDiag.txFail);
    cJSON_AddNumberToObject(root, "latUs",    (uint32_t)bChannelDiag.echoLatUs);
    cJSON_AddNumberToObject(root, "ho",       (uint8_t)bChannelDiag.realHo);
    cJSON_AddNumberToObject(root, "torqueNm", (double)(float)bChannelDiag.realTorqueNm);
    cJSON_AddNumberToObject(root, "busoffCount", (uint32_t)bChannelDiag.busoffCount);
    cJSON_AddNumberToObject(root, "dasHandsState", (uint32_t)bChannelDiag.dasHandsOnStateRx);
    cJSON_AddNumberToObject(root, "dasSourceId", (uint32_t)bChannelDiag.dasStatusSourceId);
    cJSON_AddNumberToObject(root, "frames921", (uint32_t)bChannelDiag.frames921);
    cJSON_AddNumberToObject(root, "frames923", (uint32_t)bChannelDiag.frames923);
    cJSON_AddNumberToObject(root, "tecNow",   (uint32_t)bChannelDiag.twaiTxErrNow);
    cJSON_AddNumberToObject(root, "recNow",   (uint32_t)bChannelDiag.twaiRxErrNow);
    cJSON_AddNumberToObject(root, "tecPeak",  (uint32_t)bChannelDiag.twaiTxErrPeak);
    // TWAI 상태 문자열 (0=init,1=running,2=bus_off,3=recovering)
    const char *csStr[] = {"init", "running", "bus_off", "recovering"};
    uint8_t cs = (uint8_t)bChannelDiag.twaiStateCode;
    cJSON_AddStringToObject(root, "canState", (cs < 4) ? csStr[cs] : "unknown");
    cJSON_AddNumberToObject(root, "uptimeS",  millis() / 1000);

    uint8_t smartProfile = nagSmartProfileClamp((uint8_t)bChannelDiag.smartProfile);
    const NagSmartProfileSettings &profile = nagSmartProfileSettings(smartProfile);
    cJSON_AddNumberToObject(root, "mode",     kNagModeB);
    cJSON_AddStringToObject(root, "modeStr",  "SMART");
    addNagProfileJson(root, profile);
    cJSON_AddNumberToObject(root, "targetId", kNagFixedTargetId);
    // Mode B 진단
    cJSON_AddNumberToObject(root, "dasApState",  (uint8_t)bChannelDiag.dasAutopilotStateRx);
    cJSON_AddNumberToObject(root, "steerAngleDeg", (double)(float)bChannelDiag.steeringAngleDeg);
    cJSON_AddNumberToObject(root, "frames297",   (uint32_t)bChannelDiag.frames297);
    cJSON_AddNumberToObject(root, "modeBPhase",  (uint8_t)bChannelDiag.modeBPhase);
    cJSON_AddNumberToObject(root, "modeBInjects",(uint32_t)bChannelDiag.modeBInjectCount);
    cJSON_AddNumberToObject(root, "modeBLastNm", (double)(float)bChannelDiag.modeBLastTorqueNm);
    cJSON_AddNumberToObject(root, "modeBStateAgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.modeBStateEnterMs));
    cJSON_AddNumberToObject(root, "modeBPhaseAgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.modeBPhaseEnterMs));
    cJSON_AddNumberToObject(root, "modeBFirstEchoDelayMs", (uint32_t)bChannelDiag.modeBFirstEchoDelayMs);
    // BUS-OFF 복구 모드 상태
    bool isSoftMode = gWebDriverB ? gWebDriverB->getSoftRecovery() : false;
    cJSON_AddBoolToObject(root, "boSoftMode", isSoftMode);
    uint32_t softFallback = gWebDriverB ? gWebDriverB->getSoftRecoveryFallbackCount() : 0;
    cJSON_AddNumberToObject(root, "boSoftFallback", softFallback);
    // [v4.4 실험 토글] Single Shot TX / BUS-OFF stop skip
    bool isSS  = gWebDriverB ? gWebDriverB->getSingleShotTx()  : false;
    bool isBOS = gWebDriverB ? gWebDriverB->getBusOffStopSkip() : false;
    cJSON_AddBoolToObject(root, "singleShotTx",   isSS);
    cJSON_AddBoolToObject(root, "busOffStopSkip",  isBOS);
    // ID 921 미수신 상태에서 에코 발사 횟수 (TEC 상승 원인 추적용)
    cJSON_AddNumberToObject(root, "nagFiredNoDas", (uint32_t)bChannelDiag.nagFiredNoDas);
    cJSON_AddNumberToObject(root, "skipApState", (uint32_t)bChannelDiag.skipApState);
    cJSON_AddNumberToObject(root, "echoDropLate", (uint32_t)bChannelDiag.echoDroppedLate);
    cJSON_AddNumberToObject(root, "nagLastDecision", (uint8_t)bChannelDiag.nagLastDecision);
    cJSON_AddStringToObject(root, "nagLastDecisionText", nagDecisionName((uint8_t)bChannelDiag.nagLastDecision));
    cJSON_AddNumberToObject(root, "last880AgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.last880RxMs));
    cJSON_AddNumberToObject(root, "last921AgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.last921RxMs));
    cJSON_AddNumberToObject(root, "last923AgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.last923RxMs));
    cJSON_AddNumberToObject(root, "lastDasStatusAgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.lastDasStatusRxMs));
    cJSON_AddNumberToObject(root, "last297AgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.last297RxMs));
    cJSON_AddNumberToObject(root, "lastEchoAgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.lastEchoTxMs));

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        webHealthRecordDuration(gWebNagStatsLastDurMs, gWebNagStatsMaxDurMs, handlerStartMs);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    esp_err_t sendRet = httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    webHealthRecordDuration(gWebNagStatsLastDurMs, gWebNagStatsMaxDurMs, handlerStartMs);
    return sendRet;
}

// ─── POST /api/nag-profile?p=0|1|2|3  ───────────────────────────────────────
static esp_err_t nagProfileHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char queryBuf[32] = {};
    httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf));
    char pBuf[4] = {};
    uint8_t newProfile = kNagSmartProfileDefault;
    if (httpd_query_key_value(queryBuf, "p", pBuf, sizeof(pBuf)) == ESP_OK) {
        newProfile = nagSmartProfileClamp(static_cast<uint8_t>(atoi(pBuf)));
    }

    NagConfig nc;
    portENTER_CRITICAL(&nagCfgMux);
    nc = nagConfig;
    portEXIT_CRITICAL(&nagCfgMux);
    nc.mode = kNagModeB;
    nc.smartProfile = newProfile;
    portENTER_CRITICAL(&nagCfgMux);
    nagConfig = nc;
    portEXIT_CRITICAL(&nagCfgMux);
    nagCfgSave(nc);
    bChannelDiag.nagMode = kNagModeB;
    bChannelDiag.smartProfile = newProfile;
    eventLogPush(EV_NAG_MODE, (uint16_t)bChannelDiag.twaiTxErrNow,
                 (uint16_t)bChannelDiag.twaiRxErrNow, (uint32_t)newProfile);

    const NagSmartProfileSettings &profile = nagSmartProfileSettings(newProfile);
    char logBuf[72];
    snprintf(logBuf, sizeof(logBuf), "🔧 [NAG] 스마트 프로파일 → %s", profile.label);
    logRing.push(logBuf, millis());

    return nagConfigGetHandler(req);
}

// ─── POST /api/nag-mode?m=0|1  ───────────────────────────────────────────────
// legacy endpoint: 이전 모드 토글 호출은 항상 스마트 토크로 고정한다.
static esp_err_t nagModeHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    NagConfig nc;
    portENTER_CRITICAL(&nagCfgMux);
    nc = nagConfig;
    portEXIT_CRITICAL(&nagCfgMux);
    nc.mode = kNagModeB;
    nc.smartProfile = nagSmartProfileClamp(nc.smartProfile);
    portENTER_CRITICAL(&nagCfgMux);
    nagConfig = nc;
    portEXIT_CRITICAL(&nagCfgMux);
    nagCfgSave(nc);
    bChannelDiag.nagMode = kNagModeB;
    bChannelDiag.smartProfile = nc.smartProfile;
    eventLogPush(EV_NAG_MODE, (uint16_t)bChannelDiag.twaiTxErrNow,
                 (uint16_t)bChannelDiag.twaiRxErrNow, (uint32_t)nc.smartProfile);

    logRing.push("🔧 [NAG] /api/nag-mode 호환 호출 → 스마트 토크 유지", millis());

    return nagConfigGetHandler(req);  // 변경된 설정 반환
}

// ─── POST /api/nag-update  ───────────────────────────────────────────────────
// targetId/토크 테이블 변경 요청은 무시하고 880 고정. profile 변경은 /api/nag-profile 사용.
static esp_err_t nagUpdateHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    logRing.push("🔧 [NAG] /api/nag-update — ID 880 고정 유지 (profile은 /api/nag-profile 사용)", millis());
    return nagConfigGetHandler(req);
}

// ─── POST /api/nag-reset  ────────────────────────────────────────────────────
// 스마트 토크 기본 프로파일로 리셋
static esp_err_t nagResetHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    NagConfig nc;
    nagCfgDefaultsSmart(nc);
    portENTER_CRITICAL(&nagCfgMux);
    nagConfig = nc;
    portEXIT_CRITICAL(&nagCfgMux);
    nagCfgSave(nc);
    bChannelDiag.nagMode = kNagModeB;
    bChannelDiag.smartProfile = nc.smartProfile;
    logRing.push("🔧 [NAG] 설정 리셋 → 스마트 기본값", millis());
    return nagConfigGetHandler(req);
}

// ─── POST /api/twai-ss-tx?v=0|1  ─────────────────────────────────────────────
// 4/10 정상 기준에서는 Single Shot TX를 사용하지 않는다. API는 호환용으로만 유지.
static esp_err_t twaiSsTxHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char queryBuf[32];
    int v = -1;
    if (httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf)) == ESP_OK) {
        char val[8];
        if (httpd_query_key_value(queryBuf, "v", val, sizeof(val)) == ESP_OK)
            v = atoi(val);
    }
    if (v < 0) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "v=0|1 required", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    (void)v;
    httpd_resp_send(req, "single_shot=DISABLED", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ─── POST /api/twai-busoff-stop?v=0|1  ───────────────────────────────────────
// v4.4 상태 머신 준수: BUS-OFF 상태에서는 twai_stop()을 호출하지 않는다.
static esp_err_t twaiBusOffStopHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char queryBuf[32];
    int v = -1;
    if (httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf)) == ESP_OK) {
        char val[8];
        if (httpd_query_key_value(queryBuf, "v", val, sizeof(val)) == ESP_OK)
            v = atoi(val);
    }
    if (v < 0) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "v=0|1 required", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    (void)v;
    httpd_resp_send(req, "busoff_stop_skip=ENABLED_FIXED", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ─── BUS-OFF 이벤트 로그 API ────────────────────────────────────────────────────

// ─── Wall-clock 시간 동기화 (브라우저 기반) ─────────────────────────────────
// ESP32-S3 단독 AP 모드(NTP 불가)에서 로그에 실제 날짜/시각을 찍기 위해
// 브라우저가 페이지 로드 시 epoch_ms를 POST하여 baseline을 동기화한다.
//   wallEpochMsAtBoot = browser_now_ms - millis_at_post
// 이후 임의 millis() 값을 epoch_ms로 변환 가능: epoch = wallEpochMsAtBoot + millis()
static int64_t wallEpochMsAtBoot = 0;   // 0 = 미동기화

// 로그 표기용 시간대 고정 (KST, UTC+9)
static void initLogTimezoneKst()
{
    static bool initialized = false;
    if (initialized) return;
    setenv("TZ", "KST-9", 1);
    tzset();
    initialized = true;
}

// POST /api/time?ms=<browser_epoch_ms>
static esp_err_t timeSyncHandler(httpd_req_t *req) {
    char query[64] = {0};
    httpd_req_get_url_query_str(req, query, sizeof(query) - 1);
    char val[32] = {0};
    if (httpd_query_key_value(query, "ms", val, sizeof(val)) == ESP_OK) {
        int64_t epoch = strtoll(val, nullptr, 10);
        if (epoch > 0) {
            wallEpochMsAtBoot = epoch - (int64_t)millis();
        }
    }
    httpd_resp_set_type(req, "application/json");
    char body[96];
    snprintf(body, sizeof(body), "{\"ok\":true,\"baseline_ms\":%lld}", (long long)wallEpochMsAtBoot);
    httpd_resp_sendstr(req, body);
    return ESP_OK;
}

// timestamp_ms (millis 기준) → "YYYY-MM-DD HH:MM:SS.mmm" 변환.
// baseline 미동기화면 "[ts_ms ms]" 형식으로 폴백.
static void formatLogTimestamp(uint32_t ts_ms, char *out, size_t out_n) {
    if (wallEpochMsAtBoot == 0) {
        snprintf(out, out_n, "[%u ms]", (unsigned)ts_ms);
        return;
    }
    int64_t epoch_ms = wallEpochMsAtBoot + (int64_t)ts_ms;
    time_t sec = (time_t)(epoch_ms / 1000);
    int ms = (int)(epoch_ms % 1000);
    struct tm tm_v;
    localtime_r(&sec, &tm_v);
    snprintf(out, out_n, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        tm_v.tm_year + 1900, tm_v.tm_mon + 1, tm_v.tm_mday,
        tm_v.tm_hour, tm_v.tm_min, tm_v.tm_sec, ms);
}

static void formatDurationHms(uint32_t durationMs, char *out, size_t out_n) {
    uint32_t seconds = durationMs / 1000UL;
    uint32_t h = seconds / 3600UL;
    uint32_t m = (seconds % 3600UL) / 60UL;
    uint32_t s = seconds % 60UL;
    snprintf(out, out_n, "%02u:%02u:%02u", (unsigned)h, (unsigned)m, (unsigned)s);
}

static void shortBuildId(const char *buildId, char *out, size_t out_n) {
    if (!out || out_n == 0) return;
    out[0] = '\0';
    if (!buildId) return;
    size_t hyphenCount = 0;
    size_t i = 0;
    for (; buildId[i] && i + 1 < out_n; ++i) {
        if (buildId[i] == '-') {
            hyphenCount++;
            if (hyphenCount == 2) break;
        }
        out[i] = buildId[i];
    }
    out[i] = '\0';
}

// POST /api/user-marker?type=ap_warning
// 운전자가 차량 화면 경고를 본 순간을 사후 분석 로그에 찍는 수동 마커.
static esp_err_t userMarkerHandler(httpd_req_t *req) {
    uint32_t now = millis();
    uint32_t detail = kUserMarkerApWarning;
    // 이벤트 링이 alert 폭주로 밀려도 timeseries의 dUserMark가 10분 동안 기준점을 보존한다.
    userMarkerCount = (uint32_t)userMarkerCount + 1;
    userMarkerLastMs = now;
    userMarkerLastDetail = detail;

    uint16_t tec = (uint16_t)(uint32_t)bChannelDiag.twaiTxErrNow;
    uint16_t rec = (uint16_t)(uint32_t)bChannelDiag.twaiRxErrNow;
    eventLogPush(EV_USER_MARK, tec, rec, detail);
    const NagSmartProfileSettings &profile = nagSmartProfileSettings((uint8_t)bChannelDiag.smartProfile);

    char msg[256];
    snprintf(msg, sizeof(msg),
        "[USER-MARK] AP_WARNING Profile=%s AP=%u Phase=%u 880=%u 921=%u 923=%u 297=%u HO=%u DAS=0x%02X Angle=%.1fdeg Real=%.2fNm MB=%.2fNm E=%u D=%u Last=%s TEC=%u/REC=%u",
        profile.label,
        (unsigned)(uint8_t)bChannelDiag.dasAutopilotStateRx,
        (unsigned)(uint8_t)bChannelDiag.modeBPhase,
        (unsigned)bChannelDiag.frames880,
        (unsigned)bChannelDiag.frames921,
        (unsigned)bChannelDiag.frames923,
        (unsigned)bChannelDiag.frames297,
        (unsigned)(uint8_t)bChannelDiag.realHo,
        (unsigned)(uint8_t)bChannelDiag.dasHandsOnStateRx,
        (double)(float)bChannelDiag.steeringAngleDeg,
        (double)(float)bChannelDiag.realTorqueNm,
        (double)(float)bChannelDiag.modeBLastTorqueNm,
        (unsigned)bChannelDiag.echoCount,
        (unsigned)bChannelDiag.echoDroppedLate,
        nagDecisionName((uint8_t)bChannelDiag.nagLastDecision),
        (unsigned)tec,
        (unsigned)rec);
    logRing.push(msg, now);

    httpd_resp_set_type(req, "application/json");
    char body[96];
    snprintf(body, sizeof(body), "{\"ok\":true,\"timestamp_ms\":%u,\"count\":%u}",
        (unsigned)now, (unsigned)(uint32_t)userMarkerCount);
    httpd_resp_sendstr(req, body);
    return ESP_OK;
}

// GET /api/logs-bundle — 통합 로그 번들 (런타임 + BUS-OFF + 스냅샷 + 시계열 + 이벤트)
static esp_err_t logsBundleHandler(httpd_req_t *req) {
    const uint32_t handlerStartMs = millis();
    webHealthMark(gWebLogsBundleReqCount, gWebLogsBundleLastMs, handlerStartMs);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    // 다운로드 파일명: 동기화된 시각 있으면 YYYYMMDD_HHMMSS, 없으면 uptime
    char fname[80];
    if (wallEpochMsAtBoot != 0) {
        time_t sec = (time_t)((wallEpochMsAtBoot + (int64_t)millis()) / 1000);
        struct tm tm_v;
        localtime_r(&sec, &tm_v);
        snprintf(fname, sizeof(fname),
            "attachment; filename=\"canmod_%04d%02d%02d_%02d%02d%02d.txt\"",
            tm_v.tm_year + 1900, tm_v.tm_mon + 1, tm_v.tm_mday,
            tm_v.tm_hour, tm_v.tm_min, tm_v.tm_sec);
    } else {
        snprintf(fname, sizeof(fname),
            "attachment; filename=\"canmod_uptime%u.txt\"", (unsigned)millis());
    }
    httpd_resp_set_hdr(req, "Content-Disposition", fname);
    char line[640];
    char tsBuf[40];

    // 메타 정보 (Generated at = wall-clock, Boot at = wall-clock baseline)
    formatLogTimestamp(millis(), tsBuf, sizeof(tsBuf));
    uint32_t uptimeMs = millis();
    char uptimeHms[16];
    char fwShortBuild[32];
    formatDurationHms(uptimeMs, uptimeHms, sizeof(uptimeHms));
    shortBuildId(FIRMWARE_BUILD_ID, fwShortBuild, sizeof(fwShortBuild));
    snprintf(line, sizeof(line),
        "=== CanMod 통합 로그 ===\r\nGenerated: %s\r\nFirmware: %s\r\nBuild: %s\r\nEnv: %s\r\nBuiltAt: %s\r\nGit: %s/%s dirty=%u source=%s\r\nUptime: %u ms (%s)\r\nBUS-OFF 쿨다운: %u ms\r\n\r\n",
        tsBuf,
        FIRMWARE_VERSION,
        fwShortBuild,
        FIRMWARE_BUILD_ENV,
        FIRMWARE_BUILD_AT,
        FIRMWARE_GIT_BRANCH,
        FIRMWARE_GIT_SHA,
        (unsigned)(FIRMWARE_GIT_DIRTY != 0),
        FIRMWARE_SOURCE_HASH,
        (unsigned)uptimeMs,
        uptimeHms,
        (uint32_t)bChannelDiag.busoffCooldownMs);
    httpd_resp_sendstr_chunk(req, line);

    // 섹션 1: 런타임 로그 (logRing 전체) — 스트리밍 (스택 절약, kCapacity=256 대응)
    httpd_resp_sendstr_chunk(req, "=== [1] 런타임 로그 ===\r\n");
    {
        uint32_t h = logRing.currentHead();
        uint32_t oldest = (h > LogRingBuffer::kCapacity) ? (h - LogRingBuffer::kCapacity) : 0;
        if (h == 0) {
            httpd_resp_sendstr_chunk(req, "(로그 없음)\r\n");
        } else {
            for (uint32_t i = oldest; i < h; i++) {
                const LogRingBuffer::Entry &e = logRing.at(i);
                formatLogTimestamp(e.timestamp_ms, tsBuf, sizeof(tsBuf));
                snprintf(line, sizeof(line), "[%s] %s\r\n", tsBuf, e.msg);
                httpd_resp_sendstr_chunk(req, line);
            }
        }
    }

    // 섹션 2: BUS-OFF 이벤트 로그
    httpd_resp_sendstr_chunk(req, "\r\n=== [2] BUS-OFF 이벤트 로그 ===\r\n");
    httpd_resp_sendstr_chunk(req, "seq,wall_time,timestamp_ms,tec,rec,recovery_dur_ms,since_last_ms,recovered\r\n");
    uint32_t h = busOffLog.count();
    if (h == 0) {
        httpd_resp_sendstr_chunk(req, "(BUS-OFF 없음)\r\n");
    } else {
        uint32_t oldest = (h > (uint32_t)BusOffEventLog::kCapacity) ? (h - BusOffEventLog::kCapacity) : 0;
        for (uint32_t i = oldest; i < h; i++) {
            const BusOffEvent& ev = busOffLog.at(i);
            formatLogTimestamp(ev.timestampMs, tsBuf, sizeof(tsBuf));
            snprintf(line, sizeof(line), "%u,%s,%u,%u,%u,%u,%u,%u\r\n",
                (unsigned)ev.seqNum, tsBuf, (unsigned)ev.timestampMs,
                (unsigned)ev.tec, (unsigned)ev.rec,
                (unsigned)ev.recoveryDurMs, (unsigned)ev.sinceLastMs, (unsigned)ev.recovered);
            httpd_resp_sendstr_chunk(req, line);
        }
    }

    // 섹션 3: 채널 상태 스냅샷
    httpd_resp_sendstr_chunk(req, "\r\n=== [3] 채널 상태 스냅샷 ===\r\n");
    snprintf(line, sizeof(line), "A채널: RX=%u 1016=%u 1021=%u EAP=%u ULC=%u/스킵=%u OffHW=%u/스킵=%u\r\n",
        (unsigned)aChannelDiag.framesReceivedTotal,
        (unsigned)aChannelDiag.frames1016,
        (unsigned)aChannelDiag.frames1021,
        (unsigned)aChannelDiag.eapModifiedCount,
        (unsigned)aChannelDiag.ulcStalkConfirmModifiedCount,
        (unsigned)aChannelDiag.ulcStalkConfirmSkipCount,
        (unsigned)aChannelDiag.alcOffHighwayModifiedCount,
        (unsigned)aChannelDiag.alcOffHighwaySkipCount);
    httpd_resp_sendstr_chunk(req, line);
    // A채널 진단 카운터 (TEC/REC/MERRF/RX-OVR/EFLG/TX OK·Fail)
    uint32_t aGuardUntil = (uint32_t)aChannelDiag.aTxGuardUntilMs;
    uint32_t aNow = millis();
    bool aGuardActive = aTxGuardActive(aNow);
    snprintf(line, sizeof(line),
        "A진단: TX OK=%u Fail=%u | TEC=%u/peak=%u REC=%u | MERRF=%u | RX-OVR=%u | EFLG=0x%02X/peak=0x%02X | BUS-OFF=%u | Cfg=SPI%lu->%lu/OS%s/Guard%s | Guard=%s/%ums/%s skip=%u count=%u\r\n",
        (unsigned)aChannelDiag.aTxOk,
        (unsigned)aChannelDiag.aTxFail,
        (unsigned)(uint8_t)aChannelDiag.aTec,
        (unsigned)(uint8_t)aChannelDiag.aTecPeak,
        (unsigned)(uint8_t)aChannelDiag.aRec,
        (unsigned)aChannelDiag.aMerrfCount,
        (unsigned)aChannelDiag.aRxOvrCount,
        (unsigned)(uint8_t)aChannelDiag.mcpEflg,
        (unsigned)(uint8_t)aChannelDiag.mcpEflgPeak,
        (unsigned)aChannelDiag.mcpTxBoCount,
        (unsigned long)(uint32_t)aMcpSpiFreqHz,
        (unsigned long)(uint32_t)aMcpRequestedSpiFreqHz,
        (bool)aMcpOneShotRuntime ? "ON" : "OFF",
        (bool)aTxGuardRuntime ? "ON" : "OFF",
        aGuardActive ? "ON" : "OFF",
        (unsigned)(aGuardActive ? aGuardUntil - aNow : 0),
        aTxGuardReasonName((uint8_t)aChannelDiag.aTxGuardLastReason),
        (unsigned)aChannelDiag.aTxGuardSkipCount,
        (unsigned)aChannelDiag.aTxGuardCount);
    httpd_resp_sendstr_chunk(req, line);
    const char* twS = bChannelDiag.twaiStateCode == 1 ? "OK" :
                      bChannelDiag.twaiStateCode == 2 ? "BUS_OFF" : "INIT";
        snprintf(line, sizeof(line),
            "A진단2: REC=%u/peak=%u | LastRx=%lums ago | LastTx=%lums ago | EFLG이벤트=%u\r\n",
            (unsigned)(uint8_t)aChannelDiag.aRec,
            (unsigned)(uint8_t)aChannelDiag.aRecPeak,
            (unsigned long)(aNow - (uint32_t)aChannelDiag.lastFrameRxMs),
            (unsigned long)(aNow - (uint32_t)aChannelDiag.lastTxMs),
            (unsigned)aChannelDiag.mcpEflgEventCount);
        httpd_resp_sendstr_chunk(req, line);
    snprintf(line, sizeof(line),
        "B채널: RX=%u Filt=%u Echo=%u TxFail=%u TEC=%u REC=%u TECpeak=%u 880=%u 921=%u 923=%u 297=%u DAS=%u@%u Profile=%s TWAI=%s InitErr=%d/%d\r\n",
        (unsigned)bChannelDiag.framesReceivedTotal,
        (unsigned)bChannelDiag.framesFilteredInTotal,
        (unsigned)bChannelDiag.echoCount,
        (unsigned)bChannelDiag.txFail,
        (unsigned)bChannelDiag.twaiTxErrNow,
        (unsigned)bChannelDiag.twaiRxErrNow,
        (unsigned)bChannelDiag.twaiTxErrPeak,
        (unsigned)bChannelDiag.frames880,
        (unsigned)bChannelDiag.frames921,
        (unsigned)bChannelDiag.frames923,
        (unsigned)bChannelDiag.frames297,
        (unsigned)bChannelDiag.dasHandsOnStateRx,
        (unsigned)bChannelDiag.dasStatusSourceId,
        nagSmartProfileSettings((uint8_t)bChannelDiag.smartProfile).label,
        twS,
        gWebDriverB ? gWebDriverB->getLastInstallErr() : -1,
        gWebDriverB ? gWebDriverB->getLastStartErr() : -1);
    httpd_resp_sendstr_chunk(req, line);
    // B채널 심층 진단: TWAI 누적 카운터 + 에코 품질 + 스킵 사유
    snprintf(line, sizeof(line),
        "B심층: ArbLost=%u BusErr=%u TxFailed=%u RxMissed=%u | EchoLat=%uus EchoDrop=%u | Skip RT=%u/AP=%u/HO=%u/DAS=%u\r\n",
        (unsigned)bChannelDiag.bArbLost,
        (unsigned)bChannelDiag.bBusError,
        (unsigned)bChannelDiag.bTxFailed,
        (unsigned)bChannelDiag.bRxMissed,
        (unsigned)bChannelDiag.echoLatUs,
        (unsigned)bChannelDiag.echoDroppedLate,
        (unsigned)bChannelDiag.skipRuntimeOrInactive,
        (unsigned)bChannelDiag.skipApState,
        (unsigned)bChannelDiag.skipHandsOn,
        (unsigned)bChannelDiag.skipDasState);
    httpd_resp_sendstr_chunk(req, line);
    {
        uint32_t now = millis();
        uint32_t age880 = (uint32_t)bChannelDiag.last880RxMs ? (now - (uint32_t)bChannelDiag.last880RxMs) : 0;
        uint32_t age921 = (uint32_t)bChannelDiag.last921RxMs ? (now - (uint32_t)bChannelDiag.last921RxMs) : 0;
        uint32_t age923 = (uint32_t)bChannelDiag.last923RxMs ? (now - (uint32_t)bChannelDiag.last923RxMs) : 0;
        uint32_t age297 = (uint32_t)bChannelDiag.last297RxMs ? (now - (uint32_t)bChannelDiag.last297RxMs) : 0;
        uint32_t ageEcho = (uint32_t)bChannelDiag.lastEchoTxMs ? (now - (uint32_t)bChannelDiag.lastEchoTxMs) : 0;
        snprintf(line, sizeof(line),
            "B나그판정: 880=%u(age=%ums) 921=%u(age=%ums) 923=%u(age=%ums) 297=%u(age=%ums) Echo=%u(age=%ums) | Profile=%s AP=%u Phase=%u HO=%u Torque=%.2fNm DAS=0x%02X@%u Last=%s\r\n",
            (unsigned)bChannelDiag.frames880, (unsigned)age880,
            (unsigned)bChannelDiag.frames921, (unsigned)age921,
            (unsigned)bChannelDiag.frames923, (unsigned)age923,
            (unsigned)bChannelDiag.frames297, (unsigned)age297,
            (unsigned)bChannelDiag.echoCount, (unsigned)ageEcho,
            nagSmartProfileSettings((uint8_t)bChannelDiag.smartProfile).label,
            (unsigned)(uint8_t)bChannelDiag.dasAutopilotStateRx,
            (unsigned)(uint8_t)bChannelDiag.modeBPhase,
            (unsigned)(uint8_t)bChannelDiag.realHo,
            (double)(float)bChannelDiag.realTorqueNm,
            (unsigned)(uint8_t)bChannelDiag.dasHandsOnStateRx,
            (unsigned)bChannelDiag.dasStatusSourceId,
            nagDecisionName((uint8_t)bChannelDiag.nagLastDecision));
        httpd_resp_sendstr_chunk(req, line);
        snprintf(line, sizeof(line),
            "B차단사유: OFF=%u AP_BLOCK=%u HandsOn=%u DAS_IDLE=%u LateDrop=%u NoDAS_Echo=%u\r\n",
            (unsigned)bChannelDiag.skipRuntimeOrInactive,
            (unsigned)bChannelDiag.skipApState,
            (unsigned)bChannelDiag.skipHandsOn,
            (unsigned)bChannelDiag.skipDasState,
            (unsigned)bChannelDiag.echoDroppedLate,
            (unsigned)bChannelDiag.nagFiredNoDas);
        httpd_resp_sendstr_chunk(req, line);
    }
    snprintf(line, sizeof(line),
        "복구: 시도=%u 성공=%u 실패=%u 마지막=%ums 최대소요=%ums\r\n",
        (unsigned)bChannelDiag.recoveryAttemptCount,
        (unsigned)bChannelDiag.recoverySuccessCount,
        (unsigned)bChannelDiag.recoveryFailCount,
        (unsigned)bChannelDiag.lastRecoveryDurationMs,
        (unsigned)bChannelDiag.maxRecoveryDurationMs);
    httpd_resp_sendstr_chunk(req, line);
    {
        uint32_t now = millis();
        uint32_t markerCount = (uint32_t)userMarkerCount;
        uint32_t markerLast = (uint32_t)userMarkerLastMs;
        uint32_t markerAge = markerLast ? (now - markerLast) : 0;
        snprintf(line, sizeof(line),
            "사용자마커: count=%u last_age=%ums detail=%u\r\n",
            (unsigned)markerCount, (unsigned)markerAge,
            (unsigned)(uint32_t)userMarkerLastDetail);
        httpd_resp_sendstr_chunk(req, line);
    }
    {
        uint32_t now = millis();
        uint8_t apStationCount = webHealthSampleApStations(now);
        snprintf(line, sizeof(line),
            "Web진단: status=%u(age=%ums dur=%u/%ums) nagStats=%u(age=%ums dur=%u/%ums) logsBundle=%u(age=%ums dur=%u/%ums) heap=%u/%u apSta=%u apChg=%u(age=%ums)\r\n",
            (unsigned)(uint32_t)gWebStatusReqCount,
            (unsigned)webHealthAgeMs(now, (uint32_t)gWebStatusLastMs),
            (unsigned)(uint32_t)gWebStatusLastDurMs,
            (unsigned)(uint32_t)gWebStatusMaxDurMs,
            (unsigned)(uint32_t)gWebNagStatsReqCount,
            (unsigned)webHealthAgeMs(now, (uint32_t)gWebNagStatsLastMs),
            (unsigned)(uint32_t)gWebNagStatsLastDurMs,
            (unsigned)(uint32_t)gWebNagStatsMaxDurMs,
            (unsigned)(uint32_t)gWebLogsBundleReqCount,
            (unsigned)webHealthAgeMs(now, (uint32_t)gWebLogsBundleLastMs),
            (unsigned)(uint32_t)gWebLogsBundleLastDurMs,
            (unsigned)(uint32_t)gWebLogsBundleMaxDurMs,
            (unsigned)esp_get_free_heap_size(),
            (unsigned)esp_get_minimum_free_heap_size(),
            (unsigned)apStationCount,
            (unsigned)(uint32_t)gWebApStationChangeCount,
            (unsigned)webHealthAgeMs(now, (uint32_t)gWebApStationLastChangeMs));
        httpd_resp_sendstr_chunk(req, line);
    }

    // 섹션 4: 10분 시계열 로그 (5초 × 120 샘플)
    httpd_resp_sendstr_chunk(req, "\r\n=== [4] 10분 시계열 로그 ===\r\n");
    size_t tsN = 0;
    size_t tsSnapHead = 0;
    uint32_t tsSnapResetMs = 0;
    uint32_t tsSnapRecStartMs = 0;
    bool tsSnapRecording = false;
    timeseriesSnapshot(tsN, tsSnapHead, tsSnapResetMs, tsSnapRecStartMs, tsSnapRecording);
    snprintf(line, sizeof(line),
        "# reset_at_ms=%u rec_start_ms=%u rec=%s samples=%u interval_s=5\r\n",
        (unsigned)tsSnapResetMs, (unsigned)tsSnapRecStartMs,
        tsSnapRecording ? "ON" : "OFF", (unsigned)tsN);
    httpd_resp_sendstr_chunk(req, line);
    httpd_resp_sendstr_chunk(req,
        "wall_time,timestamp_ms,busoff,tec,rec,arbLost,busErr,txFail,echo,f880,f921,f923,ho,dasState,nagMode,smartProfile,dasSource,echoDrop,skipOff,skipAP,skipHO,skipDAS,noDAS,userMark,d880,d921,d923,dEcho,dDrop,dSkipOff,dSkipAP,dSkipHO,dSkipDAS,dNoDAS,dUserMark,lastDecision,intervalDecision,f297,apState,modeBPhase,steerDeg,realTorqueNm,modeBInject,modeBLastNm,age880Ms,ageDasMs,age297Ms,ageEchoMs,modeBStateAgeMs,modeBPhaseAgeMs,modeBFirstEchoDelayMs,modeBDelayTargetMs,d297,dModeBInject\r\n");
    {
        size_t start = (tsN < TS_CAP) ? 0 : tsSnapHead;
        if (tsN == 0) {
            httpd_resp_sendstr_chunk(req, "(시계열 없음)\r\n");
        } else {
            for (size_t i = 0; i < tsN; ++i) {
                TsSample s;
                timeseriesCopyAt(start + i, s);
                formatLogTimestamp(s.t_ms, tsBuf, sizeof(tsBuf));
                snprintf(line, sizeof(line), "%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%.1f,%.2f,%u,%.2f,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
                    tsBuf, (unsigned)s.t_ms,
                    (unsigned)s.busoff, (unsigned)s.tec, (unsigned)s.rec,
                    (unsigned)s.arbLost, (unsigned)s.busErr, (unsigned)s.txFail,
                    (unsigned)s.echoCnt, (unsigned)s.f880, (unsigned)s.f921, (unsigned)s.f923,
                    (unsigned)s.handsOn, (unsigned)s.dasState,
                    (unsigned)s.nagMode, (unsigned)s.smartProfile, (unsigned)s.dasSourceId,
                    (unsigned)s.echoDrop, (unsigned)s.skipRuntime,
                    (unsigned)s.skipAp, (unsigned)s.skipHandsOn, (unsigned)s.skipDas,
                    (unsigned)s.noDasEcho, (unsigned)s.userMark,
                    (unsigned)s.d880, (unsigned)s.d921, (unsigned)s.d923,
                    (unsigned)s.dEcho, (unsigned)s.dDrop, (unsigned)s.dSkipRuntime,
                    (unsigned)s.dSkipAp, (unsigned)s.dSkipHandsOn, (unsigned)s.dSkipDas,
                    (unsigned)s.dNoDasEcho, (unsigned)s.dUserMark,
                    (unsigned)s.lastDecision,
                    (unsigned)s.intervalDecision,
                    (unsigned)s.f297, (unsigned)s.apState, (unsigned)s.modeBPhase,
                    (double)s.steerDeg, (double)s.realTorqueNm,
                    (unsigned)s.modeBInject, (double)s.modeBLastNm,
                    (unsigned)s.age880Ms, (unsigned)s.ageDasMs, (unsigned)s.age297Ms,
                    (unsigned)s.ageEchoMs, (unsigned)s.modeBStateAgeMs, (unsigned)s.modeBPhaseAgeMs,
                    (unsigned)s.modeBFirstEchoDelayMs, (unsigned)s.modeBDelayTargetMs,
                    (unsigned)s.d297, (unsigned)s.dModeBInject);
                httpd_resp_sendstr_chunk(req, line);
            }
        }
    }

    // 섹션 5: 밀리초 이벤트 로그 (BUS-OFF/Recovery/TWAI alert)
    httpd_resp_sendstr_chunk(req, "\r\n=== [5] 밀리초 이벤트 로그 ===\r\n");
    httpd_resp_sendstr_chunk(req,
        "# type: 0=BUSOFF 1=REC_OK 2=REC_FAIL 3=REC_SOFT 4=ERR_PASS 5=ARB_LOST 6=BUS_ERR 7=TX_FAIL 8=RX_FULL 9=TX_BACKOFF 10=USER_MARK 11=NAG_MODE 12=MODEB_STATE 13=MODEB_PHASE 14=MODEB_FIRST_ECHO\r\n");
    httpd_resp_sendstr_chunk(req, "# marker detail: 1=AP_WARNING | NAG_MODE detail: smartProfile 0=default 1=A 2=B 3=C | MODEB_STATE detail: ap<<16|oldHo<<8|newHo | MODEB_PHASE detail: phase<<24|ap<<16|ho<<8|decision | FIRST_ECHO detail: delay_ms\r\n");
    httpd_resp_sendstr_chunk(req, "wall_time,timestamp_ms,type,typeName,tec,rec,detail\r\n");
    {
        size_t evtN = 0;
        size_t evtSnapHead = 0;
        eventLogSnapshot(evtN, evtSnapHead);
        size_t start = (evtN < EVT_CAP) ? 0 : evtSnapHead;
        if (evtN == 0) {
            httpd_resp_sendstr_chunk(req, "(이벤트 없음)\r\n");
        } else {
            for (size_t i = 0; i < evtN; ++i) {
                CanEvent e;
                eventLogCopyAt(start + i, e);
                formatLogTimestamp(e.t_ms, tsBuf, sizeof(tsBuf));
                snprintf(line, sizeof(line), "%s,%u,%u,%s,%u,%u,%u\r\n",
                    tsBuf, (unsigned)e.t_ms, (unsigned)e.type,
                    eventTypeName(e.type),
                    (unsigned)e.tec, (unsigned)e.rec, (unsigned)e.detail);
                httpd_resp_sendstr_chunk(req, line);
            }
        }
    }

    webHealthRecordDuration(gWebLogsBundleLastDurMs, gWebLogsBundleMaxDurMs, handlerStartMs);
    httpd_resp_sendstr_chunk(req, nullptr);
    return ESP_OK;
}

// DELETE /api/busoff-log — BUS-OFF 이벤트 로그 초기화
static esp_err_t busoffLogClearHandler(httpd_req_t *req) {
    busOffLog = BusOffEventLog{};  // head=0, entries 전체 0으로 초기화
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// GET /api/busoff-log — BUS-OFF 이벤트 로그 JSON
static esp_err_t busoffLogHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    if (!root) { httpd_resp_send_500(req); return ESP_OK; }  // heap 부족 시 crash 방지
    cJSON_AddNumberToObject(root, "count", busOffLog.count());
    cJSON_AddNumberToObject(root, "cooldown_ms", (uint32_t)bChannelDiag.busoffCooldownMs);
    cJSON *arr = cJSON_AddArrayToObject(root, "events");
    uint32_t h = busOffLog.count();
    uint32_t oldest = (h > (uint32_t)BusOffEventLog::kCapacity) ? (h - BusOffEventLog::kCapacity) : 0;
    for (uint32_t i = oldest; i < h; i++) {
        const BusOffEvent& ev = busOffLog.at(i);
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "seq",      ev.seqNum);
        cJSON_AddNumberToObject(item, "ts",       ev.timestampMs);
        cJSON_AddNumberToObject(item, "tec",      ev.tec);
        cJSON_AddNumberToObject(item, "rec",      ev.rec);
        cJSON_AddNumberToObject(item, "dur_ms",   ev.recoveryDurMs);
        cJSON_AddNumberToObject(item, "since_ms", ev.sinceLastMs);
        cJSON_AddBoolToObject(item,   "ok",       ev.recovered != 0);
        cJSON_AddItemToArray(arr, item);
    }
    char *s = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!s) { httpd_resp_send_500(req); return ESP_OK; }
    httpd_resp_sendstr(req, s);
    free(s);
    return ESP_OK;
}

// GET /api/busoff-log-dl — BUS-OFF 이벤트 로그 CSV 다운로드
static esp_err_t busoffLogDlHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"busoff_log.csv\"");
    httpd_resp_sendstr_chunk(req, "seq,timestamp_ms,tec,rec,recovery_dur_ms,since_last_ms,recovered\r\n");
    uint32_t h = busOffLog.count();
    uint32_t oldest = (h > (uint32_t)BusOffEventLog::kCapacity) ? (h - BusOffEventLog::kCapacity) : 0;
    char row[128];
    for (uint32_t i = oldest; i < h; i++) {
        const BusOffEvent& ev = busOffLog.at(i);
        snprintf(row, sizeof(row), "%u,%u,%u,%u,%u,%u,%u\r\n",
            (unsigned)ev.seqNum, (unsigned)ev.timestampMs,
            (unsigned)ev.tec,    (unsigned)ev.rec,
            (unsigned)ev.recoveryDurMs, (unsigned)ev.sinceLastMs,
            (unsigned)ev.recovered);
        httpd_resp_sendstr_chunk(req, row);
    }
    httpd_resp_sendstr_chunk(req, nullptr);
    return ESP_OK;
}

// POST /api/busoff-cooldown {"ms":N} — 쿨다운 런타임 변경 + NVS 저장
static esp_err_t busoffCooldownHandler(httpd_req_t *req) {
    char buf[64] = {};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) { httpd_resp_send_500(req); return ESP_OK; }
    cJSON *root = cJSON_Parse(buf);
    if (!root) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad json"); return ESP_OK; }
    cJSON *msItem = cJSON_GetObjectItem(root, "ms");
    if (!msItem || !cJSON_IsNumber(msItem)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing ms");
        return ESP_OK;
    }
    uint32_t ms = (uint32_t)cJSON_GetNumberValue(msItem);
    cJSON_Delete(root);
    if (ms < 300)   ms = 300;    // 하한: 300ms 미만은 재진입 폭발 위험
    if (ms > 10000) ms = 10000;  // 상한: 너무 길면 에코 누락
    bChannelDiag.busoffCooldownMs = ms;
    nvs_handle_t bh;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &bh) == ESP_OK) {
        nvs_set_u32(bh, kNvsKeyBoCool, ms);
        nvs_commit(bh);
        nvs_close(bh);
    }
    httpd_resp_set_type(req, "application/json");
    char resp[48];
    snprintf(resp, sizeof(resp), "{\"ok\":true,\"ms\":%u}", (unsigned)ms);
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

// POST /api/busoff-mode {"soft":true/false}
// v4.4 상태 머신 준수: soft recovery 우선, 실패 시 hard reinstall fallback.
static esp_err_t busoffModeHandler(httpd_req_t *req) {
    char buf[64] = {};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) { httpd_resp_send_500(req); return ESP_OK; }
    cJSON *root = cJSON_Parse(buf);
    if (!root) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad json"); return ESP_OK; }
    cJSON *softItem = cJSON_GetObjectItem(root, "soft");
    if (!softItem) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing soft");
        return ESP_OK;
    }
    bool soft = cJSON_IsTrue(softItem);
    cJSON_Delete(root);
    (void)soft;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true,\"soft\":true,\"fixed\":true}");
    return ESP_OK;
}

// ─── CAN 자가 진단 API ────────────────────────────────────────────────────────

// POST /api/can-diag/start — 진단 태스크 시작
static esp_err_t canDiagStartHandler(httpd_req_t *req) {
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    bool ok = canDiagStart();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "started", ok);
    cJSON_AddStringToObject(root, "msg", ok ? "진단 시작됨 (~19초 소요)" : "이미 실행 중");
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM"); return ESP_FAIL; }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return ESP_OK;
}

// GET /api/can-diag/log?since=N — 진단 로그 폴링 (since 이후 항목만 반환)
static esp_err_t canDiagLogHandler(httpd_req_t *req) {
    char qbuf[32]; uint32_t since = 0;
    if (httpd_req_get_url_query_str(req, qbuf, sizeof(qbuf)) == ESP_OK) {
        char val[12];
        if (httpd_query_key_value(qbuf, "since", val, sizeof(val)) == ESP_OK)
            since = (uint32_t)atoi(val);
    }
    LogRingBuffer::Entry entries[32];
    int n = diagLog.readSince(since, entries, 32);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "head",  (int)diagLog.currentHead());
    cJSON_AddNumberToObject(root, "state", (uint8_t)diagState);  // 0=idle,1=running,2=done
    cJSON *arr = cJSON_AddArrayToObject(root, "lines");
    for (int i = 0; i < n; i++) {
        cJSON *e = cJSON_CreateObject();
        cJSON_AddNumberToObject(e, "ms",  entries[i].timestamp_ms);
        cJSON_AddStringToObject(e, "msg", entries[i].msg);
        cJSON_AddItemToArray(arr, e);
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM"); return ESP_FAIL; }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return ESP_OK;
}


static void applyEmergencyDisableAllFeatures()
{
    // 현재 상태를 NVS에 백업 (나중에 원복용)
    nvsWriteBool("bk_isa", isaSpeedChimeSuppressRuntime);
    nvsWriteBool("bk_emerg", emergencyVehicleDetectionRuntime);
    nvsWriteBool("bk_eap", enhancedAutopilotRuntime);
    nvsWriteBool("bk_nag", nagKillerRuntime);
    nvsWriteBool("bk_tsllc", tsllcRuntime);
    nvsWriteBool("bk_ui_ulc", uiUlcStalkConfirmRuntime);
    nvsWriteBool("bk_alc_off", uiAlcOffHighwayEnableRuntime);
    nvsWriteBool("bk_a_ch_tx", aChannelTxRuntime);

    isaSpeedChimeSuppressRuntime = false;
    emergencyVehicleDetectionRuntime = false;
    enhancedAutopilotRuntime = false;
    nagKillerRuntime = false;
    tsllcRuntime = false;
    uiUlcStalkConfirmRuntime = false;
    uiAlcOffHighwayEnableRuntime = false;
    aChannelTxRuntime = false;

    nvsWriteBool(kNvsKeyIsaSpeedChime, false);
    nvsWriteBool(kNvsKeyEmergencyVehicleDetection, false);
    nvsWriteBool(kNvsKeyEnhancedAutopilot, false);
    nvsWriteBool(kNvsKeyNagKiller, false);
    nvsWriteBool(kNvsKeyTsllc, false);
    nvsWriteBool(kNvsKeyUlcStalkConfirm, false);
    nvsWriteBool(kNvsKeyAlcOffHighway, false);
    nvsWriteBool(kNvsKeyAChTx, false);

}

static void applyEmergencyRestoreAllFeatures()
{
    isaSpeedChimeSuppressRuntime = nvsReadBool("bk_isa", false);
    emergencyVehicleDetectionRuntime = nvsReadBool("bk_emerg", false);
    enhancedAutopilotRuntime = nvsReadBool("bk_eap", false);
    nagKillerRuntime = nvsReadBool("bk_nag", false);
    tsllcRuntime = nvsReadBool("bk_tsllc", false);
    uiUlcStalkConfirmRuntime = nvsReadBool("bk_ui_ulc", false);
    uiAlcOffHighwayEnableRuntime = nvsReadBool("bk_alc_off", false);
    aChannelTxRuntime = nvsReadBool("bk_a_ch_tx", false);

    nvsWriteBool(kNvsKeyIsaSpeedChime, isaSpeedChimeSuppressRuntime);
    nvsWriteBool(kNvsKeyEmergencyVehicleDetection, emergencyVehicleDetectionRuntime);
    nvsWriteBool(kNvsKeyEnhancedAutopilot, enhancedAutopilotRuntime);
    nvsWriteBool(kNvsKeyNagKiller, nagKillerRuntime);
    nvsWriteBool(kNvsKeyTsllc, tsllcRuntime);
    nvsWriteBool(kNvsKeyUlcStalkConfirm, uiUlcStalkConfirmRuntime);
    nvsWriteBool(kNvsKeyAlcOffHighway, uiAlcOffHighwayEnableRuntime);
    nvsWriteBool(kNvsKeyAChTx, aChannelTxRuntime);
}

static esp_err_t emergencyDisableHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    applyEmergencyDisableAllFeatures();
    logRing.push("🚨 [EMERGENCY] 긴급 기능해제 실행: 모든 런타임 기능 OFF", millis());
    Serial.println("[EMERGENCY] All runtime features disabled by web request");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"message\":\"emergency_disabled\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t emergencyRestoreHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    applyEmergencyRestoreAllFeatures();
    logRing.push("✅ [RESTORE] 기능 원복 실행: 이전 설정 복원", millis());
    Serial.println("[RESTORE] All runtime features restored from backup");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"message\":\"emergency_restored\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t enablePrintHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    if (cJSON_IsBool(enabled) && appHandler)
    {
        appHandler->enablePrint = cJSON_IsTrue(enabled);
    }
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t setThemeHandler(httpd_req_t *req)
{
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';
    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *t = cJSON_GetObjectItem(json, "theme");
    if (cJSON_IsString(t))
    {
        const char *val = t->valuestring;
        if (strcmp(val, "dark") == 0 || strcmp(val, "light") == 0)
        {
            strncpy(themeRuntime, val, sizeof(themeRuntime) - 1);
            themeRuntime[sizeof(themeRuntime) - 1] = '\0';
            nvsWriteStr(kNvsKeyTheme, themeRuntime);
        }
    }
    cJSON_Delete(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}



static esp_err_t otaHandler(httpd_req_t *req)
{
    if (req->content_len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing firmware payload");
        return ESP_FAIL;
    }

    char contentType[64] = {};
    if (httpd_req_get_hdr_value_str(req, "Content-Type", contentType, sizeof(contentType)) == ESP_OK &&
        strstr(contentType, "multipart/form-data") != nullptr)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Upload raw firmware .bin, not multipart/form-data");
        return ESP_FAIL;
    }

    if (!Update.begin(req->content_len))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, Update.errorString());
        return ESP_FAIL;
    }

    int remaining = req->content_len;
    int timeoutCount = 0;
    uint8_t buffer[1024];
    while (remaining > 0)
    {
        int received = httpd_req_recv(req, reinterpret_cast<char *>(buffer),
                                      std::min(remaining, (int)sizeof(buffer)));
        if (received == HTTPD_SOCK_ERR_TIMEOUT)
        {
            if (++timeoutCount >= 5)
            {
                Update.abort();
                httpd_resp_set_status(req, "408 Request Timeout");
                httpd_resp_send(req, "Upload timed out", HTTPD_RESP_USE_STRLEN);
                return ESP_FAIL;
            }
            continue;
        }
        if (received <= 0)
        {
            Update.abort();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload failed");
            return ESP_FAIL;
        }
        timeoutCount = 0;
        if (Update.write(buffer, received) != (size_t)received)
        {
            Update.abort();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, Update.errorString());
            return ESP_FAIL;
        }
        remaining -= received;
    }

    if (!Update.end(true))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, Update.errorString());
        return ESP_FAIL;
    }

    // ── OTA 롤백 정보 NVS 저장 ──────────────────────────────────────────────
    // 재부팅 전: 현재(구) 파티션 레이블 저장 + ota_pending=1 기록
    // 새 펌웨어 첫 부팅 시 setup() 초입에서 2로 변경, 정상 부팅 완료 시 0으로 클리어
    // 만약 새 펌웨어가 크래시로 부팅 완료를 못하면 다음 부팅 시 ota_pending==2를 발견 → 롤백
    {
        const esp_partition_t *running = esp_ota_get_running_partition();
        const esp_partition_t *nextPart = esp_ota_get_next_update_partition(NULL);
        if (running) {
            nvs_handle_t wh;
            if (nvs_open(kNvsNamespace, NVS_READWRITE, &wh) == ESP_OK) {
                nvs_set_u8(wh,  kNvsKeyOtaPending,  1);
                nvs_set_str(wh, kNvsKeyOtaFallback, running->label);
                nvs_set_str(wh, kNvsKeyOtaExpectPart, nextPart ? nextPart->label : "");
                nvs_commit(wh);
                nvs_close(wh);
                char buf[80];
                snprintf(buf, sizeof(buf), "[OTA] 롤백 대기 설정: fallback=%s expect=%s",
                         running->label, nextPart ? nextPart->label : "?");
                Serial.println(buf);
                logRing.push(buf, millis());
            }
        }
    }

    Serial.println("Web: OTA upload complete, restarting");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

// ─── POST /api/ota-confirm ───────────────────────────────────────────────────
// 신 펌웨어 정상 동작 확인 → pending=0, fallback="" 클리어
static esp_err_t otaConfirmHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    nvs_handle_t nh;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &nh) == ESP_OK) {
        nvs_set_u8(nh,  kNvsKeyOtaPending,  0);
        nvs_set_str(nh, kNvsKeyOtaFallback, "");
        nvs_commit(nh);
        nvs_close(nh);
    }
    gOtaConfirmDeadlineMs = 0;
    logRing.push("[OTA] 사용자 확인 완료 — pending=0", millis());
    Serial.println("[OTA] 사용자 확인 완료 — pending=0");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ─── POST /api/ota-rollback ──────────────────────────────────────────────────
// 이전 파티션으로 롤백 → fallback 파티션 부팅 설정 후 재부팅
static esp_err_t otaRollbackHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char fallback[32] = {};
    nvs_handle_t nh;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &nh) == ESP_OK) {
        size_t sz = sizeof(fallback);
        nvs_get_str(nh, kNvsKeyOtaFallback, fallback, &sz);
        nvs_set_u8(nh, kNvsKeyOtaPending, 3);
        nvs_commit(nh);
        nvs_close(nh);
    }
    if (strlen(fallback) > 0) {
        const esp_partition_t *prev = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, fallback);
        if (prev) {
            esp_ota_set_boot_partition(prev);
        }
    }
    char buf[80];
    snprintf(buf, sizeof(buf), "[OTA] 사용자 롤백 요청 → fallback=%s", fallback);
    logRing.push(buf, millis());
    Serial.println(buf);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot_rb", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

// ─── POST /api/ota-recovery-confirm ─────────────────────────────────────────
// 복구 파티션 정상 확인 → pending=0, 복구모드 해제
static esp_err_t otaRecoveryConfirmHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    nvs_handle_t nh;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &nh) == ESP_OK) {
        nvs_set_u8(nh,  kNvsKeyOtaPending,  0);
        nvs_set_str(nh, kNvsKeyOtaFallback, "");
        nvs_commit(nh);
        nvs_close(nh);
    }
    gOtaRollbackDeadlineMs = 0;
    logRing.push("[OTA] 복구 확인 완료 — pending=0", millis());
    Serial.println("[OTA] 복구 확인 완료 — pending=0");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ─── POST /api/ota-enter-recovery ───────────────────────────────────────────
// OTA 복구모드 강제 진입 → pending=5, 재부팅
static esp_err_t otaEnterRecoveryHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    nvs_handle_t nh;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &nh) == ESP_OK) {
        nvs_set_u8(nh, kNvsKeyOtaPending, 5);
        nvs_commit(nh);
        nvs_close(nh);
    }
    logRing.push("[OTA] 복구모드 강제 진입 — 재부팅", millis());
    Serial.println("[OTA] 복구모드 강제 진입 — 재부팅");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot_rec", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

// ─────────────────────────────────────────────────────────────────────────────

// Captive portal: redirect connectivity checks to root
static esp_err_t captiveRedirectHandler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// --- DNS captive portal task ---

static void dnsTask(void *param)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        Serial.println("DNS: socket failed");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        Serial.println("DNS: bind failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf[512];
    struct sockaddr_in client;
    socklen_t clientLen;

    while (true)
    {
        clientLen = sizeof(client);
        int len = recvfrom(sock, buf, sizeof(buf), 0,
                           (struct sockaddr *)&client, &clientLen);
        if (len < 12)
            continue;

        // Build DNS response: copy query, set response flags, append answer
        buf[2] = 0x81; // QR=1, Opcode=0, AA=1, TC=0, RD=1
        buf[3] = 0x80; // RA=1, RCODE=0
        buf[6] = 0x00; // ANCOUNT = 1
        buf[7] = 0x01;

        // Append answer after the query section
        int pos = len;
        buf[pos++] = 0xC0; // Name pointer to offset 12 (query name)
        buf[pos++] = 0x0C;
        buf[pos++] = 0x00;
        buf[pos++] = 0x01; // Type A
        buf[pos++] = 0x00;
        buf[pos++] = 0x01; // Class IN
        buf[pos++] = 0x00;
        buf[pos++] = 0x00;
        buf[pos++] = 0x00;
        buf[pos++] = 0x3C; // TTL 60s
        buf[pos++] = 0x00;
        buf[pos++] = 0x04; // RDLENGTH 4
        buf[pos++] = 192;
        buf[pos++] = 168; // 192.168.4.1
        buf[pos++] = 4;
        buf[pos++] = 1;

        sendto(sock, buf, pos, 0,
               (struct sockaddr *)&client, clientLen);
    }
}

// --- Public init function ---

static httpd_handle_t webServer = NULL;

// timeseries CSV 메타 라이터: 드라이버 토글 + 기능 토글 상태를 '#' 주석 라인으로 주입
static void timeseriesMetaWrite(httpd_req_t* req) {
    char line[420];
    bool ssTx = gWebDriverB ? gWebDriverB->getSingleShotTx() : false;
    bool soft = gWebDriverB ? gWebDriverB->getSoftRecovery() : false;
    bool boSk = gWebDriverB ? gWebDriverB->getBusOffStopSkip() : false;
    uint32_t cool = gWebDriverB ? gWebDriverB->getCooldownMs() : 0;
    const NagSmartProfileSettings &profile = nagSmartProfileSettings((uint8_t)bChannelDiag.smartProfile);
    snprintf(line, sizeof(line),
        "# nag_killer=%s  smart_profile=%u(%s)  ss_tx=%s  busoff_mode=%s  busoff_stop_skip=%s  cooldown_ms=%u\n",
        (bool)nagKillerRuntime ? "ON" : "OFF",
        (unsigned)profile.id,
        profile.label,
        ssTx ? "ON" : "OFF",
        soft ? "soft" : "hard",
        boSk ? "ON" : "OFF",
        (unsigned)cool);
    httpd_resp_sendstr_chunk(req, line);
    snprintf(line, sizeof(line),
        "# a_tx=%s  tsllc=%s  UI_ulcStalkConfirm=%s  UI_alcOffHighwayEnable=%s  a_spi=%lu  a_spi_req=%lu  a_oneshot=%u  a_guard=%u  version=%s  build=%s  env=%s  built_at=%s  git=%s/%s  dirty=%u  source=%s\n",
        (bool)aChannelTxRuntime ? "ON" : "OFF",
        (bool)tsllcRuntime ? "ON" : "OFF",
        (bool)uiUlcStalkConfirmRuntime ? "ON" : "OFF",
        (bool)uiAlcOffHighwayEnableRuntime ? "ON" : "OFF",
        (unsigned long)(uint32_t)aMcpSpiFreqHz,
        (unsigned long)(uint32_t)aMcpRequestedSpiFreqHz,
        (unsigned)((bool)aMcpOneShotRuntime),
        (unsigned)((bool)aTxGuardRuntime),
        FIRMWARE_VERSION,
        FIRMWARE_BUILD_ID,
        FIRMWARE_BUILD_ENV,
        FIRMWARE_BUILD_AT,
        FIRMWARE_GIT_BRANCH,
        FIRMWARE_GIT_SHA,
        (unsigned)(FIRMWARE_GIT_DIRTY != 0),
        FIRMWARE_SOURCE_HASH);
    httpd_resp_sendstr_chunk(req, line);
}

// ─── OTA 워치독 태스크 ───────────────────────────────────────────────────────
// pending==2: 신 FW 확인 창 만료 시 자동 롤백
// pending==4: 복구 확인 창 만료 시 복구모드(pending=5) 진입
static void otaWatchdogTask(void *param)
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        uint32_t now = millis();

        if (gOtaConfirmDeadlineMs > 0 && now >= gOtaConfirmDeadlineMs) {
            // 확인 창 만료 → 자동 롤백
            gOtaConfirmDeadlineMs = 0;
            char fallback[32] = {};
            nvs_handle_t nh;
            if (nvs_open(kNvsNamespace, NVS_READWRITE, &nh) == ESP_OK) {
                size_t sz = sizeof(fallback);
                nvs_get_str(nh, kNvsKeyOtaFallback, fallback, &sz);
                nvs_set_u8(nh, kNvsKeyOtaPending, 3);
                nvs_commit(nh);
                nvs_close(nh);
            }
            if (strlen(fallback) > 0) {
                const esp_partition_t *prev = esp_partition_find_first(
                    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, fallback);
                if (prev) esp_ota_set_boot_partition(prev);
            }
            logRing.push("[OTA] 확인 창 만료 → 자동 롤백 재부팅", millis());
            Serial.println("[OTA] 확인 창 만료 → 자동 롤백 재부팅");
            vTaskDelay(pdMS_TO_TICKS(200));
            esp_restart();
        }

        if (gOtaRollbackDeadlineMs > 0 && now >= gOtaRollbackDeadlineMs) {
            // 복구 확인 창 만료 → 복구모드 진입
            gOtaRollbackDeadlineMs = 0;
            nvs_handle_t nh;
            if (nvs_open(kNvsNamespace, NVS_READWRITE, &nh) == ESP_OK) {
                nvs_set_u8(nh, kNvsKeyOtaPending, 5);
                nvs_commit(nh);
                nvs_close(nh);
            }
            gOtaRecoveryModeActive = true;
            logRing.push("[OTA] 복구 확인 창 만료 → 복구모드 진입", millis());
            Serial.println("[OTA] 복구 확인 창 만료 → 복구모드 진입");
            // 워치독 자신은 종료 (복구모드 유지, 재부팅 안 함)
            vTaskDelete(NULL);
        }
    }
}

static void webServerInit(TWAIDriver* drv = nullptr)
{
    initLogTimezoneKst();
    gWebDriverB = drv;  // B채널 드라이버 포인터 주입
    tsMetaWriter = timeseriesMetaWrite;  // CSV 메타 주입
    // NVS: load persisted runtime feature switches
    if (nvsInit())
    {
        isaSpeedChimeSuppressRuntime =
            nvsReadBool(kNvsKeyIsaSpeedChime, kIsaSpeedChimeSuppressDefaultEnabled);
        emergencyVehicleDetectionRuntime =
            nvsReadBool(kNvsKeyEmergencyVehicleDetection, kEmergencyVehicleDetectionDefaultEnabled);
        enhancedAutopilotRuntime =
            nvsReadBool(kNvsKeyEnhancedAutopilot, kEnhancedAutopilotDefaultEnabled);
        nagKillerRuntime = nvsReadBool(kNvsKeyNagKiller, kNagKillerDefaultEnabled);
        tsllcRuntime = nvsReadBool(kNvsKeyTsllc, kTsllcDefaultEnabled);
        loadAExperimentSettings();
        nvsReadStr(kNvsKeyTheme, themeRuntime, sizeof(themeRuntime), "dark");
        nagCfgLoad();  // NagConfig (smartProfile, targetId, compatibility fields) NVS 복원

        // BUS-OFF 쿨다운 NVS 복원
        {
            nvs_handle_t bnh;
            if (nvs_open(kNvsNamespace, NVS_READONLY, &bnh) == ESP_OK) {
                uint32_t cool = 1000;
                nvs_get_u32(bnh, kNvsKeyBoCool, &cool);
                if (cool < 300)   cool = 300;
                if (cool > 10000) cool = 10000;
                bChannelDiag.busoffCooldownMs = cool;
                Serial.println("NVS: BUSOFF_MODE = hard (fixed)");
                nvs_close(bnh);
            }
        }

        Serial.printf("NVS: ISA_SPEED_CHIME_SUPPRESS = %d\n", (bool)isaSpeedChimeSuppressRuntime);
        Serial.printf("NVS: EMERGENCY_VEHICLE_DETECTION = %d\n",
                      (bool)emergencyVehicleDetectionRuntime);
        Serial.printf("NVS: ENHANCED_AUTOPILOT = %d\n",
                      (bool)enhancedAutopilotRuntime);
        Serial.printf("NVS: NAG_KILLER = %d\n", (bool)nagKillerRuntime);
        Serial.printf("NVS: A_TX = %d, UI_ulcStalkConfirm = %d, UI_alcOffHighwayEnable = %d, A_SPI = %lu Hz, A_ONESHOT = %d, A_TX_GUARD = %d\n",
              (int)(bool)aChannelTxRuntime,
                  (int)(bool)uiUlcStalkConfirmRuntime,
                  (int)(bool)uiAlcOffHighwayEnableRuntime,
                  (unsigned long)(uint32_t)aMcpSpiFreqHz,
                  (int)(bool)aMcpOneShotRuntime,
                  (int)(bool)aTxGuardRuntime);
    }
    else
    {
        Serial.println("NVS: init failed, using defaults");
    }

    // OTA 상태 머신: pending 값을 읽어 타이머 데드라인 설정 + 워치독 시작
    {
        uint8_t otaPending = 0;
        nvs_handle_t onh;
        if (nvs_open(kNvsNamespace, NVS_READONLY, &onh) == ESP_OK) {
            nvs_get_u8(onh, kNvsKeyOtaPending, &otaPending);
            nvs_close(onh);
        }
        if (otaPending == 2) {
            gOtaConfirmDeadlineMs = millis() + kOtaConfirmWindowMs;
            Serial.printf("[OTA] 신 FW 확인 창 시작: %lu ms 남음\n", (unsigned long)kOtaConfirmWindowMs);
            xTaskCreatePinnedToCore(otaWatchdogTask, "ota_wd", 3072, NULL, 2, NULL, 0);
        } else if (otaPending == 4) {
            gOtaRollbackDeadlineMs = millis() + kOtaRollbackWindowMs;
            Serial.printf("[OTA] 복구 확인 창 시작: %lu ms 남음\n", (unsigned long)kOtaRollbackWindowMs);
            xTaskCreatePinnedToCore(otaWatchdogTask, "ota_wd", 3072, NULL, 2, NULL, 0);
        }
        // pending==5 → gOtaRecoveryModeActive 는 otaBootCheck() 가 이미 설정
    }
    // WIFI_AP_STA의 STA 백그라운드 채널 스캔이 TWAI ACK 타이밍을
    // 방해해 TX 에러를 유발하는 것을 방지한다.
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS, kApChannel);
    delay(100);
    Serial.printf("WiFi AP \"%s\" ch%u started (pass: %s): ", AP_SSID, (unsigned)kApChannel, AP_PASS);
    Serial.println(WiFi.softAPIP());

    // DNS captive portal on Core 0
    xTaskCreatePinnedToCore(dnsTask, "dns", 4096, NULL, 2, NULL, 0);

    // HTTP server on Core 0
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0;
    config.max_uri_handlers = 57;
    config.lru_purge_enable = true;
    config.stack_size = 16384;

    if (httpd_start(&webServer, &config) != ESP_OK)
    {
        Serial.println("HTTP: server start failed");
        return;
    }

    // Routes
    httpd_uri_t uriRoot = {
        .uri = "/", .method = HTTP_GET, .handler = rootHandler, .user_ctx = NULL};
    httpd_uri_t uriStatus = {
        .uri = "/api/status", .method = HTTP_GET, .handler = statusHandler, .user_ctx = NULL};
    httpd_uri_t uriIsaSpeedChime = {
        .uri = "/api/isa-speed-chime-suppress", .method = HTTP_POST, .handler = isaSpeedChimeSuppressHandler, .user_ctx = NULL};
    httpd_uri_t uriEmergencyVehicleDetection = {
        .uri = "/api/emergency-vehicle-detection", .method = HTTP_POST, .handler = emergencyVehicleDetectionHandler, .user_ctx = NULL};
    httpd_uri_t uriEnhancedAutopilot = {
        .uri = "/api/enhanced-autopilot", .method = HTTP_POST, .handler = enhancedAutopilotHandler, .user_ctx = NULL};
    httpd_uri_t uriNagKiller = {
        .uri = "/api/nag-killer", .method = HTTP_POST, .handler = nagKillerHandler, .user_ctx = NULL};
    httpd_uri_t uriTsllc = {
        .uri = "/api/tsllc", .method = HTTP_POST, .handler = tsllcHandler, .user_ctx = NULL};
    httpd_uri_t uriUlcStalkConfirm = {
        .uri = "/api/ui-ulc-stalk-confirm", .method = HTTP_POST, .handler = uiUlcStalkConfirmHandler, .user_ctx = NULL};
    httpd_uri_t uriAlcOffHighway = {
        .uri = "/api/ui-alc-off-highway-enable", .method = HTTP_POST, .handler = uiAlcOffHighwayHandler, .user_ctx = NULL};
    httpd_uri_t uriAChannelTx = {
        .uri = "/api/a-channel-tx", .method = HTTP_POST, .handler = aChannelTxHandler, .user_ctx = NULL};
    httpd_uri_t uriASpi8Mhz = {
        .uri = "/api/a-spi-8mhz", .method = HTTP_POST, .handler = aSpi8MhzHandler, .user_ctx = NULL};
    httpd_uri_t uriAOneShot = {
        .uri = "/api/a-oneshot", .method = HTTP_POST, .handler = aOneShotHandler, .user_ctx = NULL};
    httpd_uri_t uriATxGuard = {
        .uri = "/api/a-tx-guard", .method = HTTP_POST, .handler = aTxGuardHandler, .user_ctx = NULL};

    httpd_uri_t uriEmergencyDisable = {
        .uri = "/api/emergency-disable", .method = HTTP_POST, .handler = emergencyDisableHandler, .user_ctx = NULL};
    httpd_uri_t uriEmergencyRestore = {
        .uri = "/api/emergency-restore", .method = HTTP_POST, .handler = emergencyRestoreHandler, .user_ctx = NULL};
    httpd_uri_t uriEnablePrint = {
        .uri = "/api/enable-print", .method = HTTP_POST, .handler = enablePrintHandler, .user_ctx = NULL};

    httpd_uri_t uriSetTheme = {
        .uri = "/api/set-theme", .method = HTTP_POST, .handler = setThemeHandler, .user_ctx = NULL};
    httpd_uri_t uriOta = {
        .uri = "/api/ota", .method = HTTP_POST, .handler = otaHandler, .user_ctx = NULL};
    httpd_uri_t uriReboot = {
        .uri = "/api/reboot", .method = HTTP_POST, .handler = rebootHandler, .user_ctx = NULL};
    httpd_uri_t uriGenerate204 = {
        .uri = "/generate_204", .method = HTTP_GET, .handler = captiveRedirectHandler, .user_ctx = NULL};
    httpd_uri_t uriHotspot = {
        .uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = captiveRedirectHandler, .user_ctx = NULL};
    // Nag v2 Config/Stats API
    httpd_uri_t uriNagConfigGet = {
        .uri = "/api/nag-config", .method = HTTP_GET, .handler = nagConfigGetHandler, .user_ctx = NULL};
    httpd_uri_t uriNagStatsGet = {
        .uri = "/api/nag-stats", .method = HTTP_GET, .handler = nagStatsGetHandler, .user_ctx = NULL};
    httpd_uri_t uriNagMode = {
        .uri = "/api/nag-mode", .method = HTTP_POST, .handler = nagModeHandler, .user_ctx = NULL};
    httpd_uri_t uriNagProfile = {
        .uri = "/api/nag-profile", .method = HTTP_POST, .handler = nagProfileHandler, .user_ctx = NULL};
    httpd_uri_t uriNagUpdate = {
        .uri = "/api/nag-update", .method = HTTP_POST, .handler = nagUpdateHandler, .user_ctx = NULL};
    httpd_uri_t uriNagReset = {
        .uri = "/api/nag-reset", .method = HTTP_POST, .handler = nagResetHandler, .user_ctx = NULL};
    // CAN 자가 진단 API
    httpd_uri_t uriCanDiagStart = {
        .uri = "/api/can-diag/start", .method = HTTP_POST, .handler = canDiagStartHandler, .user_ctx = NULL};
    httpd_uri_t uriCanDiagLog = {
        .uri = "/api/can-diag/log", .method = HTTP_GET, .handler = canDiagLogHandler, .user_ctx = NULL};
    httpd_uri_t uriTimeseriesCsv = {
        .uri = "/api/timeseries.csv", .method = HTTP_GET, .handler = timeseriesCsvHandler, .user_ctx = NULL};
    httpd_uri_t uriTimeseriesReset = {
        .uri = "/api/timeseries/reset", .method = HTTP_POST, .handler = timeseriesResetHandler, .user_ctx = NULL};
    httpd_uri_t uriTimeseriesRec = {
        .uri = "/api/timeseries/rec", .method = HTTP_POST, .handler = timeseriesRecHandler, .user_ctx = NULL};
    httpd_uri_t uriTimeseriesStatus = {
        .uri = "/api/timeseries/status", .method = HTTP_GET, .handler = timeseriesStatusHandler, .user_ctx = NULL};
    httpd_uri_t uriEventsCsv = {
        .uri = "/api/events.csv", .method = HTTP_GET, .handler = eventLogCsvHandler, .user_ctx = NULL};
    httpd_uri_t uriLogsBundle = {
        .uri = "/api/logs-bundle", .method = HTTP_GET, .handler = logsBundleHandler, .user_ctx = NULL};
    // 브라우저 → 디바이스 wall-clock 동기화 (단독 AP 모드에서 NTP 대체)
    httpd_uri_t uriTimeSync = {
        .uri = "/api/time", .method = HTTP_POST, .handler = timeSyncHandler, .user_ctx = NULL};
    httpd_uri_t uriUserMarker = {
        .uri = "/api/user-marker", .method = HTTP_POST, .handler = userMarkerHandler, .user_ctx = NULL};
    // BUS-OFF 이벤트 로그 API
    httpd_uri_t uriBusOffLog = {
        .uri = "/api/busoff-log", .method = HTTP_GET, .handler = busoffLogHandler, .user_ctx = NULL};
    httpd_uri_t uriBusOffLogClear = {
        .uri = "/api/busoff-log", .method = HTTP_DELETE, .handler = busoffLogClearHandler, .user_ctx = NULL};
    httpd_uri_t uriBusOffLogDl = {
        .uri = "/api/busoff-log-dl", .method = HTTP_GET, .handler = busoffLogDlHandler, .user_ctx = NULL};
    httpd_uri_t uriBusOffCooldown = {
        .uri = "/api/busoff-cooldown", .method = HTTP_POST, .handler = busoffCooldownHandler, .user_ctx = NULL};
    httpd_uri_t uriBusOffMode = {
        .uri = "/api/busoff-mode", .method = HTTP_POST, .handler = busoffModeHandler, .user_ctx = NULL};
    httpd_uri_t uriTwaiSsTx = {
        .uri = "/api/twai-ss-tx", .method = HTTP_POST, .handler = twaiSsTxHandler, .user_ctx = NULL};
    httpd_uri_t uriTwaiBusOffStop = {
        .uri = "/api/twai-busoff-stop", .method = HTTP_POST, .handler = twaiBusOffStopHandler, .user_ctx = NULL};

    httpd_register_uri_handler(webServer, &uriRoot);
    httpd_register_uri_handler(webServer, &uriStatus);
    httpd_register_uri_handler(webServer, &uriIsaSpeedChime);
    httpd_register_uri_handler(webServer, &uriEmergencyVehicleDetection);
    httpd_register_uri_handler(webServer, &uriEnhancedAutopilot);
    httpd_register_uri_handler(webServer, &uriNagKiller);
    httpd_register_uri_handler(webServer, &uriTsllc);
    httpd_register_uri_handler(webServer, &uriUlcStalkConfirm);
    httpd_register_uri_handler(webServer, &uriAlcOffHighway);
    httpd_register_uri_handler(webServer, &uriAChannelTx);
    httpd_register_uri_handler(webServer, &uriASpi8Mhz);
    httpd_register_uri_handler(webServer, &uriAOneShot);
    httpd_register_uri_handler(webServer, &uriATxGuard);
    httpd_register_uri_handler(webServer, &uriEmergencyDisable);
    httpd_register_uri_handler(webServer, &uriEmergencyRestore);
    httpd_register_uri_handler(webServer, &uriEnablePrint);

    httpd_register_uri_handler(webServer, &uriSetTheme);
    httpd_register_uri_handler(webServer, &uriOta);
    httpd_register_uri_handler(webServer, &uriReboot);
    httpd_register_uri_handler(webServer, &uriGenerate204);
    httpd_register_uri_handler(webServer, &uriHotspot);
    httpd_register_uri_handler(webServer, &uriNagConfigGet);
    httpd_register_uri_handler(webServer, &uriNagStatsGet);
    httpd_register_uri_handler(webServer, &uriNagMode);
    httpd_register_uri_handler(webServer, &uriNagProfile);
    httpd_register_uri_handler(webServer, &uriNagUpdate);
    httpd_register_uri_handler(webServer, &uriNagReset);
    httpd_register_uri_handler(webServer, &uriCanDiagStart);
    httpd_register_uri_handler(webServer, &uriCanDiagLog);
    httpd_register_uri_handler(webServer, &uriTimeseriesCsv);
    httpd_register_uri_handler(webServer, &uriTimeseriesReset);
    httpd_register_uri_handler(webServer, &uriTimeseriesRec);
    httpd_register_uri_handler(webServer, &uriTimeseriesStatus);
    httpd_register_uri_handler(webServer, &uriEventsCsv);
    httpd_register_uri_handler(webServer, &uriLogsBundle);
    httpd_register_uri_handler(webServer, &uriTimeSync);
    httpd_register_uri_handler(webServer, &uriUserMarker);
    httpd_register_uri_handler(webServer, &uriBusOffLog);
    httpd_register_uri_handler(webServer, &uriBusOffLogClear);
    httpd_register_uri_handler(webServer, &uriBusOffLogDl);
    httpd_register_uri_handler(webServer, &uriBusOffCooldown);
    httpd_register_uri_handler(webServer, &uriBusOffMode);
    httpd_register_uri_handler(webServer, &uriTwaiSsTx);
    httpd_register_uri_handler(webServer, &uriTwaiBusOffStop);

    // OTA 상태 머신 라우트 (복구모드 여부와 관계없이 항상 등록)
    httpd_uri_t uriOtaConfirm = {
        .uri = "/api/ota-confirm", .method = HTTP_POST, .handler = otaConfirmHandler, .user_ctx = NULL};
    httpd_uri_t uriOtaRollback = {
        .uri = "/api/ota-rollback", .method = HTTP_POST, .handler = otaRollbackHandler, .user_ctx = NULL};
    httpd_uri_t uriOtaRecoveryConfirm = {
        .uri = "/api/ota-recovery-confirm", .method = HTTP_POST, .handler = otaRecoveryConfirmHandler, .user_ctx = NULL};
    httpd_uri_t uriOtaEnterRecovery = {
        .uri = "/api/ota-enter-recovery", .method = HTTP_POST, .handler = otaEnterRecoveryHandler, .user_ctx = NULL};
    httpd_register_uri_handler(webServer, &uriOtaConfirm);
    httpd_register_uri_handler(webServer, &uriOtaRollback);
    httpd_register_uri_handler(webServer, &uriOtaRecoveryConfirm);
    httpd_register_uri_handler(webServer, &uriOtaEnterRecovery);

    Serial.println("Web dashboard ready at http://192.168.4.1/");
}

#endif // DRIVER_TWAI && !NATIVE_BUILD
