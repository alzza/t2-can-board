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
#include <esp_task_wdt.h>
#include <freertos/task.h>

#include "shared_types.h"
#include "can_helpers.h"
#include "ota_boot_policy.h"
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
static volatile uint32_t gWebDownloadBusyUntilMs = 0;
static volatile uint32_t gWebApStationCount = 0;
static volatile uint32_t gWebApStationChangeCount = 0;
static volatile uint32_t gWebApStationLastChangeMs = 0;

struct NagMarkerSnapshot {
    uint32_t tMs;
    uint32_t detail;
    uint32_t txAttempts;
    uint32_t txSuccess;
    uint32_t echoConfirmed;
    uint32_t txLatencyUs;
    uint32_t echoLatencyUs;
    uint32_t echoDrop;
    uint16_t dasSourceId;
    uint8_t mode;
    uint8_t readiness;
    uint8_t decision;
    uint8_t apState;
    uint8_t phase;
    uint8_t realHo;
    uint8_t dasState;
    uint8_t txHo;
    float realTorqueNm;
    float txTorqueNm;
    char raw880[24];
    char rawDas[24];
};

static constexpr size_t kNagMarkerSnapshotCapacity = 8;
static NagMarkerSnapshot gNagMarkerSnapshots[kNagMarkerSnapshotCapacity] = {};
static uint32_t gNagMarkerSnapshotHead = 0;
static portMUX_TYPE gNagMarkerSnapshotMux = portMUX_INITIALIZER_UNLOCKED;

static void nagMarkerSnapshotPush(const NagMarkerSnapshot &snapshot) {
    portENTER_CRITICAL(&gNagMarkerSnapshotMux);
    gNagMarkerSnapshots[gNagMarkerSnapshotHead % kNagMarkerSnapshotCapacity] = snapshot;
    ++gNagMarkerSnapshotHead;
    portEXIT_CRITICAL(&gNagMarkerSnapshotMux);
}

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

static inline void formatCanPayload(uint32_t low, uint32_t high, char *out, size_t outSize) {
    if (!out || outSize == 0) return;
    snprintf(out, outSize, "%02X %02X %02X %02X %02X %02X %02X %02X",
        (unsigned)(low & 0xFFU), (unsigned)((low >> 8) & 0xFFU),
        (unsigned)((low >> 16) & 0xFFU), (unsigned)((low >> 24) & 0xFFU),
        (unsigned)(high & 0xFFU), (unsigned)((high >> 8) & 0xFFU),
        (unsigned)((high >> 16) & 0xFFU), (unsigned)((high >> 24) & 0xFFU));
}

static inline void formatCanPayloadStable(const Shared<uint32_t> &sequence,
                                          const Shared<uint32_t> &lowWord,
                                          const Shared<uint32_t> &highWord,
                                          char *out, size_t outSize) {
    uint32_t before = 0;
    uint32_t after = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    do {
        before = sequence.load(std::memory_order_acquire);
        low = lowWord.load(std::memory_order_relaxed);
        high = highWord.load(std::memory_order_relaxed);
        after = sequence.load(std::memory_order_acquire);
    } while ((before & 1U) != 0U || before != after);
    formatCanPayload(low, high, out, outSize);
}

static inline bool webDownloadBusy(uint32_t now) {
    uint32_t until = (uint32_t)gWebDownloadBusyUntilMs;
    return (until != 0) && ((uint32_t)(until - now) < 0x80000000UL);
}

static inline void logsBundleSerialTrace(const char *stage, uint32_t startMs) {
    // 다운로드 단계 추적은 Web health 카운터와 전체 로그 파일로 확인한다.
    // 정상 다운로드마다 Serial을 채우지 않도록 3-A 정책에서는 출력하지 않는다.
    (void)stage;
    (void)startMs;
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
static constexpr char kNvsKeySummonUnlock[] = "summon_unlock";
static constexpr char kNvsKeySummonConditionLimit[] = "eu_cond";
static constexpr char kNvsKeyNagKiller[] = "nag_killer";
static constexpr char kNvsKeyTsllc[]        = "tsllc";        // TSLLC (스톱사인/초록불 제어)
static constexpr char kNvsKeyAChTx[]        = "a_ch_tx";      // A채널 1021 수정 송신 마스터
static constexpr char kNvsKeyASpiMhz[]      = "a_spi_mhz";    // A MCP2515 SPI MHz: 8 or 10
static constexpr char kNvsKeyAOneShot[]     = "a_oneshot";    // A MCP2515 one-shot mode
static constexpr char kNvsKeyATxGuard[]     = "a_tx_guard";   // A TX guard enable
// NagConfig NVS 키 (15자 이하)
static constexpr char kNvsKeyNagMode[]      = "nag_mode";
static constexpr char kNvsKeyNagApOnly[]    = "nag_ap_only";
static_assert(sizeof(kNvsKeyNagMode) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagApOnly) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeySummonConditionLimit) - 1 <= 15, "NVS key too long");
static constexpr char kNvsKeyTheme[] = "theme";
static_assert(sizeof(kNvsKeyTsllc) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyAChTx) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyASpiMhz) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyAOneShot) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyATxGuard) - 1 <= 15, "NVS key too long");
static constexpr char kNvsKeyOtaPending[]    = "ota_pending";    // 0=normal 1=written 2=booting
static constexpr char kNvsKeyOtaFallback[]   = "ota_fallback";   // previous partition label
static constexpr char kNvsKeyOtaExpectPart[] = "ota_expect_pt";  // expected new partition label
static constexpr char kNvsKeyOtaUploadAt[]   = "ota_up_at";      // browser/device upload timestamp
static constexpr char kNvsKeyOtaFbVersion[]  = "ota_fb_ver";
static constexpr char kNvsKeyOtaFbBuildId[]  = "ota_fb_bid";
static constexpr char kNvsKeyOtaFbBuiltAt[]  = "ota_fb_bat";
static constexpr char kNvsKeyOtaFbEnv[]      = "ota_fb_env";
static constexpr char kNvsKeyOtaFbGitSha[]   = "ota_fb_sha";
static constexpr char kNvsKeyOtaFbSource[]   = "ota_fb_src";
static constexpr char kNvsKeyOtaNewVersion[] = "ota_new_ver";
static constexpr char kNvsKeyOtaNewBuildId[] = "ota_new_bid";
static constexpr char kNvsKeyOtaNewBuiltAt[] = "ota_new_bat";
static constexpr char kNvsKeyOtaNewEnv[]     = "ota_new_env";
static constexpr char kNvsKeyOtaNewGitSha[]  = "ota_new_sha";
static constexpr char kNvsKeyOtaNewSource[]  = "ota_new_src";
static constexpr char kNvsKeyBoCool[]        = "bo_cool";        // BUS-OFF 쿨다운 (ms)
static_assert(sizeof(kNvsKeyBoCool)        - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaPending)    - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaFallback)   - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaExpectPart) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaUploadAt)   - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaFbVersion)  - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaFbBuildId)  - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaFbBuiltAt)  - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaFbEnv)      - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaFbGitSha)   - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaFbSource)   - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaNewVersion) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaNewBuildId) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaNewBuiltAt) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaNewEnv)     - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaNewGitSha)  - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyOtaNewSource)  - 1 <= 15, "NVS key too long");

// OTA 상태 머신 타이밍 상수
static constexpr uint32_t kOtaConfirmWindowMs  = 60000;   // 신 FW 확인 창 (1분)
static constexpr uint32_t kOtaRollbackWindowMs = 60000;   // 복구 확인 창 (1분)
// OTA 상태 머신 전역 변수
static uint32_t gOtaConfirmDeadlineMs  = 0;   // pending==2 일 때 만료 시각 (millis)
static uint32_t gOtaRollbackDeadlineMs = 0;   // pending==4 일 때 만료 시각 (millis)
static uint8_t  gOtaBootPendingState   = 0;   // setup에서 확정한 현재 OTA 상태
static bool     gOtaRecoveryModeActive = false; // CAN 비활성 복구모드 플래그
static char     gCanBootBlockReason[96] = {};   // 복구 UI/API에 표시할 CAN 차단 사유

static_assert(sizeof(kNvsKeyTheme) - 1 <= 15, "NVS key too long");

static_assert(sizeof(kNvsKeyIsaSpeedChime) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyEmergencyVehicleDetection) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeySummonUnlock) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeySummonConditionLimit) - 1 <= 15, "NVS key too long");
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

#if defined(SUMMON_UNLOCK)
static constexpr bool kWebSupportsSummonUnlock = true;
#else
static constexpr bool kWebSupportsSummonUnlock = false;
#endif

#if defined(SUMMON_UNLOCK)
static constexpr bool kWebSupportsTsllc = true;
#else
static constexpr bool kWebSupportsTsllc = false;
#endif

#if defined(NAG_KILLER)
static constexpr bool kWebSupportsNagKiller = true;
#else
static constexpr bool kWebSupportsNagKiller = false;
#endif

// --- NVS helpers ---

static bool nvsInit(bool *storageErased = nullptr)
{
    if (storageErased) *storageErased = false;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        if (nvs_flash_erase() != ESP_OK) return false;
        if (storageErased) *storageErased = true;
        err = nvs_flash_init();
    }
    return err == ESP_OK;
}

static void nvsGetStrFromHandle(nvs_handle_t handle, const char *key, char *outBuf, size_t outLen)
{
    if (!outBuf || outLen == 0) return;
    outBuf[0] = '\0';
    size_t needed = outLen;
    if (nvs_get_str(handle, key, outBuf, &needed) != ESP_OK) outBuf[0] = '\0';
    outBuf[outLen - 1] = '\0';
}

static esp_err_t otaWriteFirmwareMeta(nvs_handle_t handle,
                                      const char *versionKey,
                                      const char *buildIdKey,
                                      const char *builtAtKey,
                                      const char *envKey,
                                      const char *gitShaKey,
                                      const char *sourceKey)
{
    esp_err_t err = nvs_set_str(handle, versionKey, FIRMWARE_VERSION);
    if (err == ESP_OK) err = nvs_set_str(handle, buildIdKey, FIRMWARE_BUILD_ID);
    if (err == ESP_OK) err = nvs_set_str(handle, builtAtKey, FIRMWARE_BUILD_AT);
    if (err == ESP_OK) err = nvs_set_str(handle, envKey, FIRMWARE_BUILD_ENV);
    if (err == ESP_OK) err = nvs_set_str(handle, gitShaKey, FIRMWARE_GIT_SHA);
    if (err == ESP_OK) err = nvs_set_str(handle, sourceKey, FIRMWARE_SOURCE_HASH);
    return err;
}

static esp_err_t otaWriteFallbackFirmwareMeta(nvs_handle_t handle)
{
    return otaWriteFirmwareMeta(handle, kNvsKeyOtaFbVersion, kNvsKeyOtaFbBuildId,
                                kNvsKeyOtaFbBuiltAt, kNvsKeyOtaFbEnv,
                                kNvsKeyOtaFbGitSha, kNvsKeyOtaFbSource);
}

static esp_err_t otaWriteNewFirmwareMeta(nvs_handle_t handle)
{
    return otaWriteFirmwareMeta(handle, kNvsKeyOtaNewVersion, kNvsKeyOtaNewBuildId,
                                kNvsKeyOtaNewBuiltAt, kNvsKeyOtaNewEnv,
                                kNvsKeyOtaNewGitSha, kNvsKeyOtaNewSource);
}

static esp_err_t otaCopyNvsString(nvs_handle_t handle, const char *srcKey, const char *dstKey)
{
    char value[96] = {};
    size_t needed = sizeof(value);
    esp_err_t err = nvs_get_str(handle, srcKey, value, &needed);
    if (err == ESP_ERR_NVS_NOT_FOUND) value[0] = '\0';
    else if (err != ESP_OK) return err;
    value[sizeof(value) - 1] = '\0';
    return nvs_set_str(handle, dstKey, value);
}

static esp_err_t otaCopyNewMetaToFallback(nvs_handle_t handle)
{
    esp_err_t err = otaCopyNvsString(handle, kNvsKeyOtaNewVersion, kNvsKeyOtaFbVersion);
    if (err == ESP_OK) err = otaCopyNvsString(handle, kNvsKeyOtaNewBuildId, kNvsKeyOtaFbBuildId);
    if (err == ESP_OK) err = otaCopyNvsString(handle, kNvsKeyOtaNewBuiltAt, kNvsKeyOtaFbBuiltAt);
    if (err == ESP_OK) err = otaCopyNvsString(handle, kNvsKeyOtaNewEnv, kNvsKeyOtaFbEnv);
    if (err == ESP_OK) err = otaCopyNvsString(handle, kNvsKeyOtaNewGitSha, kNvsKeyOtaFbGitSha);
    if (err == ESP_OK) err = otaCopyNvsString(handle, kNvsKeyOtaNewSource, kNvsKeyOtaFbSource);
    return err;
}

static void otaCopyText(char *dst, size_t dstLen, const char *src)
{
    if (!dst || dstLen == 0) return;
    snprintf(dst, dstLen, "%s", src ? src : "");
}

static void otaFillMetaFromPartitionLabel(const char *label,
                                          char *version, size_t versionLen,
                                          char *buildId, size_t buildIdLen,
                                          char *builtAt, size_t builtAtLen)
{
    if (!label || !label[0]) return;
    const esp_partition_t *part = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
    if (!part) return;
    esp_app_desc_t desc = {};
    if (esp_ota_get_partition_description(part, &desc) != ESP_OK) return;
    if (version && versionLen > 0 && !version[0]) otaCopyText(version, versionLen, desc.version);
    if (buildId && buildIdLen > 0 && !buildId[0]) otaCopyText(buildId, buildIdLen, desc.project_name);
    if (builtAt && builtAtLen > 0 && !builtAt[0]) {
        snprintf(builtAt, builtAtLen, "%s %s", desc.date, desc.time);
    }
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

static esp_err_t nvsReadU8OrDefault(nvs_handle_t handle, const char *key,
                                    uint8_t fallback, uint8_t &value)
{
    esp_err_t err = nvs_get_u8(handle, key, &value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        value = fallback;
        return ESP_OK;
    }
    return err;
}

static esp_err_t nvsReadU32OrDefault(nvs_handle_t handle, const char *key,
                                     uint32_t fallback, uint32_t &value)
{
    esp_err_t err = nvs_get_u32(handle, key, &value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        value = fallback;
        return ESP_OK;
    }
    return err;
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

static bool purgeRetiredExperimentNvs()
{
    static constexpr const char *keys[] = {
        "ulc_stalk", "alc_offhwy", "ulc_offhwy", "ulc_speed", "ulc_blind",
        "auto_lc", "bk_ui_ulc", "bk_alc_off", "enh_autopilot", "bk_eap",
        "nag_prof", "nag_id", "nag_tc", "nag_tb2", "nag_tb3", "nag_ho",
    };
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK) return false;
    uint8_t erased = 0;
    bool ok = true;
    for (const char *key : keys) {
        esp_err_t err = nvs_erase_key(handle, key);
        if (err == ESP_OK) erased++;
        else if (err != ESP_ERR_NVS_NOT_FOUND) {
            ok = false;
            Serial.printf("NVS: retired experiment key purge failed for %s (%ld)\n",
                          key, static_cast<long>(err));
        }
    }
    if (erased > 0) {
        esp_err_t err = nvs_commit(handle);
        if (err != ESP_OK) {
            ok = false;
            Serial.printf("NVS: retired experiment purge commit failed (%ld)\n", static_cast<long>(err));
        }
    }
    nvs_close(handle);
    return ok;
}

static void applyOtaSafeFeatureRuntimeDefaults()
{
    isaSpeedChimeSuppressRuntime = false;
    emergencyVehicleDetectionRuntime = false;
    summonUnlockRuntime = false;
    summonConditionLimitRuntime = kSummonConditionLimitDefaultEnabled;
    nagKillerRuntime = false;
    nagApOnlyRuntime = kNagApOnlyDefaultEnabled;
    tsllcRuntime = false;
    aChannelTxRuntime = false;
    uint8_t defaultMhz = (kAMcpDefaultSpiFreqHz >= 10000000UL) ? 10 : 8;
    aMcpSpiFreqHz = (uint32_t)defaultMhz * 1000000UL;
    aMcpRequestedSpiFreqHz = (uint32_t)defaultMhz * 1000000UL;
    aMcpOneShotRuntime = kAMcpOneShotDefaultEnabled;
    aTxGuardRuntime = kATxGuardDefaultEnabled;
}

static void enterCanBootFailClosed(const char *reason, esp_err_t err = ESP_OK)
{
    applyOtaSafeFeatureRuntimeDefaults();
    gOtaRecoveryModeActive = true;
    if (err == ESP_OK) snprintf(gCanBootBlockReason, sizeof(gCanBootBlockReason), "%s", reason);
    else snprintf(gCanBootBlockReason, sizeof(gCanBootBlockReason), "%s (%ld)",
                  reason, static_cast<long>(err));
    Serial.printf("[BOOT-SAFE] CAN 시작 차단: %s\n", gCanBootBlockReason);
    logRing.push(gCanBootBlockReason, millis());
}

static esp_err_t writeOtaSafeFeatureSettings(nvs_handle_t handle)
{
    applyOtaSafeFeatureRuntimeDefaults();

    uint8_t defaultMhz = (kAMcpDefaultSpiFreqHz >= 10000000UL) ? 10 : 8;
    esp_err_t err = nvs_set_u8(handle, kNvsKeyIsaSpeedChime, 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyEmergencyVehicleDetection, 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeySummonUnlock, 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeySummonConditionLimit,
                                         kSummonConditionLimitDefaultEnabled ? 1 : 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyNagKiller, 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyTsllc, 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyAChTx, 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyASpiMhz, defaultMhz);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyAOneShot, kAMcpOneShotDefaultEnabled ? 1 : 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyATxGuard, kATxGuardDefaultEnabled ? 1 : 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyNagMode, kNagModeDefault);
    if (err == ESP_OK) err = nvs_set_u8(handle, kNvsKeyNagApOnly, kNagApOnlyDefaultEnabled ? 1 : 0);
    return err;
}

// 로그 다운로드 중 NAG 킬러 TX 임시 중단 (NVS 미저장, 핸들러 완료 시 복원)
// A채널 포워딩은 유지하고 B채널 nag 주입만 끄고 TX 큐를 비운다.
// A채널 TX까지 끄면 로그 저장 수십 초간 포워딩이 끊겨 차량 CAN 에러가 발생한다.
static void logDownloadCanQuietOn(bool &savedATx, bool &savedNag)
{
    savedATx = false;  // A채널 TX는 끄지 않음 — 포워딩 유지
    savedNag = (bool)nagKillerRuntime;
    if (savedNag) {
        nagKillerRuntime = false;
        esp_err_t ce = twai_clear_transmit_queue();
        (void)ce;
        logRing.push("[Web] 로그 저장: NAG TX 임시 중단 (A채널 포워딩 유지)", millis());
    }
}

static void logDownloadCanQuietOff(bool savedATx, bool savedNag)
{
    (void)savedATx;  // A채널 TX는 건드리지 않음
    if (savedNag) {
        nagKillerRuntime = savedNag;
        logRing.push("[Web] 로그 저장 완료: NAG TX 복원", millis());
    }
}

static bool prepareOtaUploadCanQuiet()
{
    // OTA 플래시 쓰기 전 새 수정 송신을 원자적으로 차단하고, 이미 허가된
    // A/B sendCheck()가 끝나는 것을 기다린다. 단순 토글 OFF와 달리 코어 간
    // 경합으로 인한 마지막 1건의 송신을 OTA 구간 밖으로 밀어낸다.
    canTxQuiesceBegin();
    const uint32_t drainStartedMs = millis();

    nvs_handle_t handle;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        applyOtaSafeFeatureRuntimeDefaults();
        if (appDriver) (void)appDriver->quiesceTransmit();
        if (gWebDriverB) (void)gWebDriverB->quiesceTransmit();
        Serial.printf("[OTA] 업로드 전 CAN TX OFF: NVS open 실패 (%ld)\n", static_cast<long>(err));
        return false;
    }

    err = writeOtaSafeFeatureSettings(handle);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err != ESP_OK) {
        if (appDriver) (void)appDriver->quiesceTransmit();
        if (gWebDriverB) (void)gWebDriverB->quiesceTransmit();
        Serial.printf("[OTA] 업로드 전 CAN TX OFF 저장 실패 (%ld)\n", static_cast<long>(err));
        return false;
    }

    while (!canTxQuiesceIdle()) {
        if (millis() - drainStartedMs >= 250) {
            logRing.push("[OTA] CAN 송신 종료 확인 시간 초과 — 업로드 취소", millis());
            Serial.println("[OTA] CAN 송신 종료 확인 시간 초과 — 업로드 취소");
            if (appDriver) (void)appDriver->quiesceTransmit();
            if (gWebDriverB) (void)gWebDriverB->quiesceTransmit();
            return false;
        }
        delay(1);
    }

    const bool aQuiet = !appDriver || appDriver->quiesceTransmit();
    const bool bQuiet = !gWebDriverB || gWebDriverB->quiesceTransmit();
    if (!aQuiet || !bQuiet) {
        Serial.printf("[OTA] CAN 물리 TX 정지 실패 (A=%u B=%u)\n",
                      aQuiet ? 1U : 0U, bQuiet ? 1U : 0U);
        return false;
    }

    logRing.push("[OTA] 업로드 시작: A=Listen-Only, B=Stopped, 기능 NVS OFF", millis());
    Serial.println("[OTA] 업로드 시작: A=Listen-Only, B=Stopped, 기능 NVS OFF");
    return true;
}

// NagConfig NVS 저장/로드 헬퍼
static void nagCfgSave(const NagConfig &c) {
    nvs_handle_t h;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h, kNvsKeyNagMode, nagModeClamp(c.mode));
    nvs_commit(h);
    nvs_close(h);
}

static esp_err_t loadVehicleRuntimeSettingsBeforeCan()
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    uint8_t isa = kIsaSpeedChimeSuppressDefaultEnabled ? 1 : 0;
    uint8_t emergency = kEmergencyVehicleDetectionDefaultEnabled ? 1 : 0;
    uint8_t summon = kSummonUnlockDefaultEnabled ? 1 : 0;
    uint8_t summonConditionLimit = kSummonConditionLimitDefaultEnabled ? 1 : 0;
    uint8_t nag = kNagKillerDefaultEnabled ? 1 : 0;
    uint8_t tsllc = kTsllcDefaultEnabled ? 1 : 0;
    uint8_t aTx = 1;
    uint8_t defaultMhz = (kAMcpDefaultSpiFreqHz >= 10000000UL) ? 10 : 8;
    uint8_t spiMhz = defaultMhz;
    uint8_t oneShot = kAMcpOneShotDefaultEnabled ? 1 : 0;
    uint8_t txGuard = kATxGuardDefaultEnabled ? 1 : 0;
    uint8_t nagMode = kNagModeDefault;
    uint8_t nagApOnly = kNagApOnlyDefaultEnabled ? 1 : 0;
    uint32_t busoffCooldown = 1000;

    err = nvsReadU8OrDefault(h, kNvsKeyIsaSpeedChime, isa, isa);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyEmergencyVehicleDetection, emergency, emergency);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeySummonUnlock, summon, summon);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeySummonConditionLimit,
                                                summonConditionLimit, summonConditionLimit);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyNagKiller, nag, nag);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyTsllc, tsllc, tsllc);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyAChTx, aTx, aTx);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyASpiMhz, spiMhz, spiMhz);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyAOneShot, oneShot, oneShot);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyATxGuard, txGuard, txGuard);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyNagMode, nagMode, nagMode);
    if (err == ESP_OK) err = nvsReadU8OrDefault(h, kNvsKeyNagApOnly, nagApOnly, nagApOnly);
    if (err == ESP_OK) err = nvsReadU32OrDefault(h, kNvsKeyBoCool, busoffCooldown, busoffCooldown);
    nvs_close(h);
    if (err != ESP_OK) return err;

    spiMhz = sanitizeASpiMhz(spiMhz);
    if (busoffCooldown < 300) busoffCooldown = 300;
    if (busoffCooldown > 10000) busoffCooldown = 10000;

    isaSpeedChimeSuppressRuntime = isa != 0;
    emergencyVehicleDetectionRuntime = emergency != 0;
    summonUnlockRuntime = summon != 0;
    summonConditionLimitRuntime = summonConditionLimit != 0;
    nagKillerRuntime = nag != 0;
    nagApOnlyRuntime = nagApOnly != 0;
    tsllcRuntime = tsllc != 0;
    aChannelTxRuntime = aTx != 0;
    aMcpSpiFreqHz = (uint32_t)spiMhz * 1000000UL;
    aMcpRequestedSpiFreqHz = (uint32_t)spiMhz * 1000000UL;
    aMcpOneShotRuntime = oneShot != 0;
    aTxGuardRuntime = txGuard != 0;
    bChannelDiag.busoffCooldownMs = busoffCooldown;

    NagConfig c;
    nagCfgDefaults(c);
    const bool retiredNagModeStored = nagMode != kNagMode1 && nagMode != kNagMode2;
    c.mode = nagModeClamp(nagMode);
    portENTER_CRITICAL(&nagCfgMux);
    nagConfig = c;
    portEXIT_CRITICAL(&nagCfgMux);
    bChannelDiag.nagMode = c.mode;
    if (retiredNagModeStored) {
        // 과거 Mode 3 NVS 값은 최신 검증 원본의 기본 Mode 2로 즉시 치환한다.
        (void)nvsWriteU8(kNvsKeyNagMode, kNagModeDefault);
        logRing.push("[NVS] 제거된 Nag Mode 3 값을 기본 Mode 2로 정리", millis());
    }
    return ESP_OK;
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

    if (enabled && ((&target == &summonUnlockRuntime) || (&target == &tsllcRuntime)) &&
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
    char buf[80];
    snprintf(buf, sizeof(buf), "[Web] %s: %s", logName, enabled ? "ON" : "OFF");
    logRing.push(buf, millis());
    if ((&target == &summonUnlockRuntime) || (&target == &tsllcRuntime) ||
        (&target == &nagKillerRuntime) || (&target == &nagApOnlyRuntime) ||
        (&target == &aChannelTxRuntime)) {
        eventLogPush(EV_FEATURE_STATE,
                     (uint16_t)bChannelDiag.twaiTxErrNow,
                     (uint16_t)bChannelDiag.twaiRxErrNow,
                     eventFeatureStateDetail());
    }

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

    logRing.push("[Web] 보드 재부팅 요청", millis());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

static esp_err_t nvsResetHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    if (!nvsInit())
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NVS init failed");
        return ESP_FAIL;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &handle);
    if (err == ESP_OK)
    {
        err = nvs_erase_all(handle);
        if (err == ESP_OK) err = writeOtaSafeFeatureSettings(handle);
        if (err == ESP_OK) err = nvs_set_u8(handle, "nvs_init_ok", 1);
        if (err == ESP_OK) err = nvs_commit(handle);
        nvs_close(handle);
    }

    if (err != ESP_OK)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "NVS reset failed (%ld)", static_cast<long>(err));
        Serial.println(msg);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
        return ESP_FAIL;
    }

    logRing.push("[Web] NVS 전체 초기화 요청 — 재부팅", millis());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"erased\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "nvs_reset", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// CAN Sniffer  (실시간 CAN 프레임 모니터링)
//

// --- HTTP handlers ---

static esp_err_t rootHandler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
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
    if (webDownloadBusy(handlerStartMs)) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_set_hdr(req, "Retry-After", "3");
        httpd_resp_send(req, "{\"busy\":\"logs_bundle\"}", HTTPD_RESP_USE_STRLEN);
        webHealthRecordDuration(gWebStatusLastDurMs, gWebStatusMaxDurMs, handlerStartMs);
        return ESP_OK;
    }

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
    bool isaSuppress = kWebSupportsIsaSpeedChimeSuppress ? (bool)isaSpeedChimeSuppressRuntime : false;
    bool emergencyVehicleDetection =
        kWebSupportsEmergencyVehicleDetection ? (bool)emergencyVehicleDetectionRuntime : false;
    bool aChannelTx = (bool)aChannelTxRuntime;
    bool summonUnlockEnabled =
        kWebSupportsSummonUnlock ? (bool)summonUnlockRuntime : false;
    const bool summonConditionLimit = (bool)summonConditionLimitRuntime;
    bool nagKiller = kWebSupportsNagKiller ? (bool)nagKillerRuntime : false;
    bool tsllcEnabled = kWebSupportsTsllc ? (bool)tsllcRuntime : false;
    bool nagApOnly = (bool)nagApOnlyRuntime;
    bool aGuardActiveNow = aTxGuardActive(handlerStartMs);
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        webHealthRecordDuration(gWebStatusLastDurMs, gWebStatusMaxDurMs, handlerStartMs);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    cJSON_AddBoolToObject(root, "isa_speed_chime_suppress", isaSuppress);
    cJSON_AddBoolToObject(root, "emergency_vehicle_detection", emergencyVehicleDetection);
    cJSON_AddBoolToObject(root, "summon_unlock_enabled", summonUnlockEnabled);
    cJSON_AddBoolToObject(root, "summon_condition_limit", summonConditionLimit);
    cJSON_AddBoolToObject(root, "nag_killer", nagKiller);
    cJSON_AddBoolToObject(root, "nag_ap_only", nagApOnly);
    cJSON_AddBoolToObject(root, "a_channel_tx", aChannelTx);
    cJSON_AddBoolToObject(root, "tsllc_enabled", tsllcEnabled);
    cJSON_AddBoolToObject(root, "can_boot_allowed", !gOtaRecoveryModeActive);
    cJSON_AddStringToObject(root, "can_boot_block_reason", gCanBootBlockReason);

    cJSON *summon = cJSON_AddObjectToObject(root, "summon_unlock");
    if (summon) {
        const bool summonEnabled = summonUnlockEnabled;
        const bool gateOpen = summonGateOpen(handlerStartMs);
        const bool summonUnlockActive =
            summonEnabled && aChannelTx && gateOpen && !aGuardActiveNow;
        const uint32_t last280Ms = (uint32_t)summonGateDiag.last280Ms;
        const uint32_t last280AgeMs = last280Ms ? handlerStartMs - last280Ms : 0;
        cJSON_AddBoolToObject(summon, "enabled", summonEnabled);
        cJSON_AddBoolToObject(summon, "active", summonUnlockActive);
        cJSON_AddBoolToObject(summon, "tx_master", aChannelTx);
        cJSON_AddBoolToObject(summon, "gate", gateOpen);
        cJSON_AddBoolToObject(summon, "condition_limit", summonConditionLimit);
        cJSON_AddBoolToObject(summon, "guard", aGuardActiveNow);
        cJSON_AddBoolToObject(summon, "inject_ready",
                              summonEnabled && aChannelTx && gateOpen && !aGuardActiveNow);
        cJSON_AddBoolToObject(summon, "ap", (bool)summonGateDiag.apActive);
        cJSON_AddNumberToObject(summon, "ap_state", (uint8_t)summonGateDiag.apState);
        cJSON_AddNumberToObject(summon, "ap_stable_ms", summonApStableMs(handlerStartMs));
        cJSON_AddNumberToObject(summon, "ap_stable_required_ms", kSummonApStableRequiredMs);
        cJSON_AddStringToObject(summon, "gate_reason", summonGateReasonName(handlerStartMs));
        cJSON_AddBoolToObject(summon, "parked", (bool)summonGateDiag.parked);
        cJSON_AddBoolToObject(summon, "summon", (bool)summonGateDiag.summoning);
        cJSON_AddBoolToObject(summon, "aca", (bool)summonGateDiag.acaActive);
        cJSON_AddBoolToObject(summon, "spr", (bool)summonGateDiag.sprSeen);
        cJSON_AddStringToObject(summon, "block_reason",
                                !summonEnabled ? "DISABLED" :
                                !aChannelTx ? "A_TX_OFF" :
                                aGuardActiveNow ? "TX_GUARD" :
                                gateOpen ? "NONE" : summonGateReasonName(handlerStartMs));
        cJSON_AddNumberToObject(summon, "last_280_age_ms", last280AgeMs);
        cJSON_AddNumberToObject(summon, "parked_timeout_ms", kSummonParkedTimeoutMs);
        cJSON_AddNumberToObject(summon, "rx280", (uint32_t)summonGateDiag.frames280);
        cJSON_AddNumberToObject(summon, "rx390", (uint32_t)summonGateDiag.frames390);
        cJSON_AddNumberToObject(summon, "rx921", (uint32_t)summonGateDiag.frames921);
        cJSON_AddNumberToObject(summon, "rx1016", (uint32_t)summonGateDiag.frames1016);
        cJSON_AddNumberToObject(summon, "rxMux1", (uint32_t)summonGateDiag.mux1Received);
        cJSON_AddNumberToObject(summon, "txOk", (uint32_t)summonGateDiag.txOk);
        cJSON_AddNumberToObject(summon, "txBusy", (uint32_t)summonGateDiag.txBusy);
        cJSON_AddNumberToObject(summon, "txFail", (uint32_t)summonGateDiag.txFail);
        cJSON_AddNumberToObject(summon, "blocked", (uint32_t)summonGateDiag.blocked);
        cJSON_AddNumberToObject(summon, "last_tx_age_ms",
                                webSafeAgeMs(handlerStartMs, (uint32_t)summonGateDiag.lastTxMs));
        cJSON_AddNumberToObject(summon, "last_blocked_age_ms",
                                webSafeAgeMs(handlerStartMs, (uint32_t)summonGateDiag.lastBlockedMs));
        cJSON_AddNumberToObject(summon, "wake_count", (uint32_t)aChannelDiag.wakeCount);
        cJSON_AddBoolToObject(summon, "wake_waiting_tx", (bool)aChannelDiag.wakeAwaitingSummonTx);
        cJSON_AddNumberToObject(summon, "wake_to_tx_ms", (uint32_t)aChannelDiag.wakeToSummonTxMs);
        cJSON_AddNumberToObject(summon, "last_wake_rx_ms", (uint32_t)aChannelDiag.lastWakeRxMs);
        const uint8_t summonCanState = ((uint8_t)aChannelDiag.mcpEflg & 0x20U) ? 2U : 1U;
        cJSON_AddNumberToObject(summon, "canState", summonCanState);
        cJSON_AddNumberToObject(summon, "uptimeS", handlerStartMs / 1000U);
        cJSON_AddStringToObject(summon, "hardware", "HW3");
        cJSON_AddNumberToObject(summon, "enable_bit", 46);
    }

    cJSON *tsllc = cJSON_AddObjectToObject(root, "tsllc");
    if (tsllc) {
        cJSON_AddBoolToObject(tsllc, "enabled", tsllcEnabled);
        cJSON_AddBoolToObject(tsllc, "tx_master", aChannelTx);
        cJSON_AddBoolToObject(tsllc, "guard", aGuardActiveNow);
        cJSON_AddBoolToObject(tsllc, "inject_ready",
                              tsllcEnabled && aChannelTx && !aGuardActiveNow);
        cJSON_AddNumberToObject(tsllc, "txOk", (uint32_t)aChannelDiag.tsllcTxOk);
        cJSON_AddNumberToObject(tsllc, "txFail", (uint32_t)aChannelDiag.tsllcTxFail);
        cJSON_AddNumberToObject(tsllc, "last_tx_age_ms",
                                webSafeAgeMs(handlerStartMs,
                                             (uint32_t)aChannelDiag.lastTsllcTxMs));
    }

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
    cJSON_AddNumberToObject(root, "user_marker_count", (uint32_t)userMarkerCount);
    cJSON_AddNumberToObject(root, "user_marker_log_count", tsDelta((uint32_t)userMarkerCount, (uint32_t)tsBaseUserMark));
    cJSON_AddNumberToObject(root, "user_marker_last_ms", (uint32_t)userMarkerLastMs);
    cJSON_AddNumberToObject(root, "user_marker_last_detail", (uint32_t)userMarkerLastDetail);
    cJSON_AddStringToObject(root, "user_marker_last_detail_text", userMarkerDetailName((uint32_t)userMarkerLastDetail));
    cJSON_AddBoolToObject(root, "user_marker_active", (bool)userMarkerActive);
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
        uint8_t otaPending = gOtaBootPendingState;
        cJSON_AddNumberToObject(root, "ota_pending_state", otaPending);
        cJSON_AddBoolToObject(root, "ota_pending_verify",        otaPending == 2);
        cJSON_AddBoolToObject(root, "ota_rollback_confirm_pending", otaPending == 4);
        cJSON_AddBoolToObject(root, "ota_recovery_mode",         gOtaRecoveryModeActive);
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
        char fallback[32] = {};
        char expectPart[32] = {};
        char uploadAt[40] = {};
        char fbVersion[24] = {};
        char fbBuildId[96] = {};
        char fbBuiltAt[40] = {};
        char fbEnv[32] = {};
        char fbGitSha[48] = {};
        char fbSource[96] = {};
        nvs_handle_t fnh;
        if (nvs_open(kNvsNamespace, NVS_READONLY, &fnh) == ESP_OK) {
            nvsGetStrFromHandle(fnh, kNvsKeyOtaFallback, fallback, sizeof(fallback));
            nvsGetStrFromHandle(fnh, kNvsKeyOtaExpectPart, expectPart, sizeof(expectPart));
            nvsGetStrFromHandle(fnh, kNvsKeyOtaUploadAt, uploadAt, sizeof(uploadAt));
            nvsGetStrFromHandle(fnh, kNvsKeyOtaFbVersion, fbVersion, sizeof(fbVersion));
            nvsGetStrFromHandle(fnh, kNvsKeyOtaFbBuildId, fbBuildId, sizeof(fbBuildId));
            nvsGetStrFromHandle(fnh, kNvsKeyOtaFbBuiltAt, fbBuiltAt, sizeof(fbBuiltAt));
            nvsGetStrFromHandle(fnh, kNvsKeyOtaFbEnv, fbEnv, sizeof(fbEnv));
            nvsGetStrFromHandle(fnh, kNvsKeyOtaFbGitSha, fbGitSha, sizeof(fbGitSha));
            nvsGetStrFromHandle(fnh, kNvsKeyOtaFbSource, fbSource, sizeof(fbSource));
            nvs_close(fnh);
        }
        otaFillMetaFromPartitionLabel(fallback, fbVersion, sizeof(fbVersion),
                                      fbBuildId, sizeof(fbBuildId), fbBuiltAt, sizeof(fbBuiltAt));
        cJSON_AddStringToObject(root, "ota_fallback_label", fallback);
        cJSON_AddStringToObject(root, "ota_expected_label", expectPart);
        cJSON_AddStringToObject(root, "ota_upload_at", uploadAt);
        cJSON_AddStringToObject(root, "ota_current_version", FIRMWARE_VERSION);
        cJSON_AddStringToObject(root, "ota_current_build_id", FIRMWARE_BUILD_ID);
        cJSON_AddStringToObject(root, "ota_current_build_at", FIRMWARE_BUILD_AT);
        cJSON_AddStringToObject(root, "ota_current_build_env", FIRMWARE_BUILD_ENV);
        cJSON_AddStringToObject(root, "ota_current_git_sha", FIRMWARE_GIT_SHA);
        cJSON_AddStringToObject(root, "ota_current_source_hash", FIRMWARE_SOURCE_HASH);
        cJSON_AddStringToObject(root, "ota_fallback_version", fbVersion);
        cJSON_AddStringToObject(root, "ota_fallback_build_id", fbBuildId);
        cJSON_AddStringToObject(root, "ota_fallback_build_at", fbBuiltAt);
        cJSON_AddStringToObject(root, "ota_fallback_build_env", fbEnv);
        cJSON_AddStringToObject(root, "ota_fallback_git_sha", fbGitSha);
        cJSON_AddStringToObject(root, "ota_fallback_source_hash", fbSource);
    }

    cJSON *features = cJSON_AddObjectToObject(root, "features");

    addFeatureState(features, "isa_speed_chime_suppress",
                    kWebSupportsIsaSpeedChimeSuppress, isaSuppress, kIsaSpeedChimeSuppressBuildEnabled);
    addFeatureState(features, "emergency_vehicle_detection",
                    kWebSupportsEmergencyVehicleDetection, emergencyVehicleDetection,
                    kEmergencyVehicleDetectionBuildEnabled);
    addFeatureState(features, "summon_unlock",
                    kWebSupportsSummonUnlock, summonUnlockEnabled,
                    kSummonUnlockBuildEnabled);
    addFeatureState(features, "summon_condition_limit",
                    kWebSupportsSummonUnlock, summonConditionLimit,
                    kSummonUnlockBuildEnabled);
    addFeatureState(features, "nag_killer", kWebSupportsNagKiller, nagKiller, kNagKillerBuildEnabled);
    addFeatureState(features, "nag_ap_only", kWebSupportsNagKiller, nagApOnly, kNagKillerBuildEnabled);
    addFeatureState(features, "a_channel_tx", true, aChannelTx, true);
    // TSLLC: 스톱사인/신호등 자동 정지 + 앞차 있을 때 초록불 자동 출발
    addFeatureState(features, "tsllc_enabled", kWebSupportsTsllc, tsllcEnabled, kTsllcBuildEnabled);
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
    static uint32_t lastFrames280 = 0;
    static uint32_t lastFrames1016 = 0;
    static uint32_t lastFrames1021 = 0;
    static uint32_t lastFrames880 = 0;
    static uint32_t lastFrames921 = 0;
    static uint32_t lastFrames923 = 0;
    static uint32_t period280Ms = 0;
    static uint32_t period1016Ms = 0;
    static uint32_t period1021Ms = 0;
    static uint32_t period880Ms = 0;
    static uint32_t period921Ms = 0;
    static uint32_t period923Ms = 0;
    uint16_t nagTargetId = kNagFixedTargetId;
    {
        uint32_t now = millis();
        uint32_t elapsed = now - idRateLastMs;
        if (elapsed >= 1000) {
            const uint32_t cur280 = (uint32_t)aChannelDiag.frames280;
            const uint32_t cur1016 = (uint32_t)aChannelDiag.frames1016;
            const uint32_t cur1021 = (uint32_t)aChannelDiag.frames1021;
            const uint32_t cur880 = (uint32_t)bChannelDiag.frames880;
            const uint32_t cur921 = (uint32_t)bChannelDiag.frames921;
            const uint32_t cur923 = (uint32_t)bChannelDiag.frames923;

            const uint32_t d280 = cur280 - lastFrames280;
            const uint32_t d1016 = cur1016 - lastFrames1016;
            const uint32_t d1021 = cur1021 - lastFrames1021;
            const uint32_t d880  = cur880  - lastFrames880;
            const uint32_t d921  = cur921  - lastFrames921;
            const uint32_t d923  = cur923  - lastFrames923;

            period280Ms = (d280 > 0) ? (elapsed / d280) : 0;
            period1016Ms = (d1016 > 0) ? (elapsed / d1016) : 0;
            period1021Ms = (d1021 > 0) ? (elapsed / d1021) : 0;
            period880Ms  = (d880  > 0) ? (elapsed / d880)  : 0;
            period921Ms  = (d921  > 0) ? (elapsed / d921)  : 0;
            period923Ms  = (d923  > 0) ? (elapsed / d923)  : 0;

            lastFrames280 = cur280;
            lastFrames1016 = cur1016;
            lastFrames1021 = cur1021;
            lastFrames880 = cur880;
            lastFrames921 = cur921;
            lastFrames923 = cur923;
            idRateLastMs = now;
        }
    }

    const uint32_t statusNowMs = millis();

    cJSON *ach = cJSON_AddObjectToObject(channels, "a_channel");
    cJSON_AddNumberToObject(ach, "frames_received", (uint32_t)aChannelDiag.framesReceivedTotal);
    cJSON_AddNumberToObject(ach, "frame_hz", (double)(float)aChannelDiag.frameHz);
    cJSON_AddNumberToObject(ach, "frames_280", (uint32_t)aChannelDiag.frames280);
    cJSON_AddNumberToObject(ach, "id_280_period_ms", (uint32_t)period280Ms);
    cJSON_AddNumberToObject(ach, "frames_390", (uint32_t)aChannelDiag.frames390);
    cJSON_AddNumberToObject(ach, "frames_921", (uint32_t)aChannelDiag.frames921);
    cJSON_AddNumberToObject(ach, "frames_1016", (uint32_t)aChannelDiag.frames1016);
    cJSON_AddNumberToObject(ach, "id_1016_period_ms", (uint32_t)period1016Ms);
    cJSON_AddNumberToObject(ach, "frames_1021", (uint32_t)aChannelDiag.frames1021);
    cJSON_AddNumberToObject(ach, "id_1021_period_ms", (uint32_t)period1021Ms);
    cJSON_AddNumberToObject(ach, "summon_unlock_modified",
                            (uint32_t)aChannelDiag.summonUnlockModifiedCount);
    cJSON_AddNumberToObject(ach, "summon_tx_ok", (uint32_t)summonGateDiag.txOk);
    cJSON_AddNumberToObject(ach, "summon_tx_busy", (uint32_t)summonGateDiag.txBusy);
    cJSON_AddNumberToObject(ach, "summon_tx_fail", (uint32_t)summonGateDiag.txFail);
    cJSON_AddNumberToObject(ach, "summon_blocked", (uint32_t)summonGateDiag.blocked);
    cJSON_AddNumberToObject(ach, "tsllc_modified",
                            (uint32_t)aChannelDiag.tsllcModifiedCount);
    cJSON_AddNumberToObject(ach, "tsllc_tx_ok", (uint32_t)aChannelDiag.tsllcTxOk);
    cJSON_AddNumberToObject(ach, "tsllc_tx_busy", (uint32_t)aChannelDiag.tsllcTxBusy);
    cJSON_AddNumberToObject(ach, "tsllc_tx_fail", (uint32_t)aChannelDiag.tsllcTxFail);
    cJSON_AddNumberToObject(ach, "last_frame_id", (uint32_t)aChannelDiag.lastFrameIdReceived);
    cJSON_AddNumberToObject(ach, "last_loop_ms", (uint32_t)aChannelDiag.lastLoopMs);
    cJSON_AddNumberToObject(ach, "core_id", (int32_t)aChannelDiag.loopCoreId);
    cJSON_AddNumberToObject(ach, "spi_freq_hz", (uint32_t)aMcpSpiFreqHz);
    cJSON_AddNumberToObject(ach, "spi_requested_hz", (uint32_t)aMcpRequestedSpiFreqHz);
    cJSON_AddBoolToObject(ach, "spi_reboot_required", (uint32_t)aMcpSpiFreqHz != (uint32_t)aMcpRequestedSpiFreqHz);
    cJSON_AddBoolToObject(ach, "mcp_one_shot", (bool)aMcpOneShotRuntime);
    cJSON_AddBoolToObject(ach, "channel_tx_enabled", (bool)aChannelTxRuntime);
    cJSON_AddBoolToObject(ach, "tx_guard_enabled", (bool)aTxGuardRuntime);
    // MCP2515 에러 플래그 (EFLG 레지스터, 1초 폴링)
    cJSON_AddNumberToObject(ach, "mcp_eflg", (uint32_t)aChannelDiag.mcpEflg);
    cJSON_AddNumberToObject(ach, "mcp_eflg_peak", (uint32_t)aChannelDiag.mcpEflgPeak);
    cJSON_AddNumberToObject(ach, "mcp_txbo_count", (uint32_t)aChannelDiag.mcpTxBoCount);
    cJSON_AddNumberToObject(ach, "mcp_recovery_attempt", (uint32_t)aChannelDiag.mcpRecoveryAttemptCount);
    cJSON_AddNumberToObject(ach, "mcp_recovery_success", (uint32_t)aChannelDiag.mcpRecoverySuccessCount);
    cJSON_AddNumberToObject(ach, "mcp_recovery_fail", (uint32_t)aChannelDiag.mcpRecoveryFailCount);
    cJSON_AddNumberToObject(ach, "mcp_busoff_since_ms", (uint32_t)aChannelDiag.mcpBusOffSinceMs);
    cJSON_AddNumberToObject(ach, "mcp_last_recovery_ms", (uint32_t)aChannelDiag.mcpLastRecoveryMs);
    // A채널 송수신 진단 카운터 (1초 EFLG 폴링 + handler 송신 결과)
    cJSON_AddNumberToObject(ach, "tx_ok",   (uint32_t)aChannelDiag.aTxOk);
    cJSON_AddNumberToObject(ach, "tx_queued", (uint32_t)aChannelDiag.aTxOk);
    cJSON_AddNumberToObject(ach, "tx_busy", (uint32_t)aChannelDiag.aTxBusy);
    cJSON_AddNumberToObject(ach, "tx_fail", (uint32_t)aChannelDiag.aTxFail);
    cJSON_AddNumberToObject(ach, "tx_hard_error", (uint32_t)aChannelDiag.aTxFail);
    cJSON_AddNumberToObject(ach, "tx_completed", (uint32_t)aChannelDiag.aTxCompleted);
    cJSON_AddNumberToObject(ach, "tx_arbitration_lost", (uint32_t)aChannelDiag.aTxArbitrationLost);
    cJSON_AddNumberToObject(ach, "tx_aborted", (uint32_t)aChannelDiag.aTxAborted);
    cJSON_AddNumberToObject(ach, "tx_fail_window",
                            (uint32_t)(uint8_t)aChannelDiag.aTxFailWindowDelta);
    cJSON_AddNumberToObject(ach, "tx_fail_window_peak",
                            (uint32_t)(uint8_t)aChannelDiag.aTxFailWindowPeak);
    cJSON_AddNumberToObject(ach, "tx_fail_guard_threshold",
                            (uint32_t)kATxGuardTxFailBurstThreshold);
    cJSON_AddNumberToObject(ach, "tec",      (uint32_t)(uint8_t)aChannelDiag.aTec);
    cJSON_AddNumberToObject(ach, "rec",      (uint32_t)(uint8_t)aChannelDiag.aRec);
    cJSON_AddNumberToObject(ach, "tec_peak", (uint32_t)(uint8_t)aChannelDiag.aTecPeak);
    cJSON_AddNumberToObject(ach, "merrf",    (uint32_t)aChannelDiag.aMerrfCount);
    cJSON_AddNumberToObject(ach, "rx_ovr",   (uint32_t)aChannelDiag.aRxOvrCount);
    cJSON_AddNumberToObject(ach, "rx0_ovr",  (uint32_t)aChannelDiag.aRx0OvrCount);
    cJSON_AddNumberToObject(ach, "rx1_ovr",  (uint32_t)aChannelDiag.aRx1OvrCount);
    cJSON_AddNumberToObject(ach, "rx_buffer0_frames", (uint32_t)aChannelDiag.aRxBuffer0Frames);
    cJSON_AddNumberToObject(ach, "rx_buffer1_frames", (uint32_t)aChannelDiag.aRxBuffer1Frames);
    cJSON_AddNumberToObject(ach, "rx_drain_frames", (uint32_t)aChannelDiag.aRxDrainFrames);
    cJSON_AddNumberToObject(ach, "rx_drain_calls", (uint32_t)aChannelDiag.aRxDrainCalls);
    cJSON_AddNumberToObject(ach, "rx_queue_high_water", (uint32_t)aChannelDiag.aRxQueueHighWater);
    cJSON_AddNumberToObject(ach, "rx_queue_drops", (uint32_t)aChannelDiag.aRxQueueDropCount);
    cJSON_AddNumberToObject(ach, "rec_peak",         (uint32_t)(uint8_t)aChannelDiag.aRecPeak);
    cJSON_AddNumberToObject(ach, "last_frame_rx_ms", (uint32_t)aChannelDiag.lastFrameRxMs);
    cJSON_AddNumberToObject(ach, "last_tx_ms",       (uint32_t)aChannelDiag.lastTxMs);
    cJSON_AddNumberToObject(ach, "loop_gap_last_us", (uint32_t)aChannelDiag.loopGapLastUs);
    cJSON_AddNumberToObject(ach, "loop_gap_peak_us", (uint32_t)aChannelDiag.loopGapPeakUs);
    cJSON_AddNumberToObject(ach, "loop_gap_over_250us", (uint32_t)aChannelDiag.loopGapOver250usCount);
    cJSON_AddNumberToObject(ach, "loop_gap_over_500us", (uint32_t)aChannelDiag.loopGapOver500usCount);
    cJSON_AddNumberToObject(ach, "loop_gap_over_1ms", (uint32_t)aChannelDiag.loopGapOver1msCount);
    cJSON_AddNumberToObject(ach, "loop_gap_over_2ms", (uint32_t)aChannelDiag.loopGapOver2msCount);
    cJSON_AddStringToObject(ach, "last_overrun_phase", aCanPhaseName((uint8_t)aChannelDiag.lastOverrunPhase));
    cJSON_AddNumberToObject(ach, "eflg_event_count", (uint32_t)aChannelDiag.mcpEflgEventCount);
    cJSON_AddNumberToObject(ach, "wake_count", (uint32_t)aChannelDiag.wakeCount);
    cJSON_AddNumberToObject(ach, "last_wake_rx_ms", (uint32_t)aChannelDiag.lastWakeRxMs);
    cJSON_AddBoolToObject(ach, "wake_waiting_summon_tx", (bool)aChannelDiag.wakeAwaitingSummonTx);
    cJSON_AddNumberToObject(ach, "wake_to_summon_tx_ms", (uint32_t)aChannelDiag.wakeToSummonTxMs);
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
    const bool aDriverOk = (bool)aChannelDiag.driverInitialized;
    const uint8_t aEflg = (uint8_t)aChannelDiag.mcpEflg;
    const char *aHealthState = "OK";
    const char *aHealthReason = "정상 수신 중";
    uint8_t aHealthLevel = 0;
    if (!aDriverOk) {
        aHealthState = "INIT_FAILED";
        aHealthReason = "MCP2515 초기화 실패";
        aHealthLevel = 2;
    } else if (!aTaskAlive) {
        aHealthState = "LOOP_STALLED";
        aHealthReason = "A채널 폴링 루프 지연";
        aHealthLevel = 2;
    } else if (aEflg & 0x20U) {
        aHealthState = "BUS_OFF";
        aHealthReason = "MCP2515 BUS-OFF";
        aHealthLevel = 2;
    } else if (!aFresh) {
        aHealthState = "NO_FRAMES";
        aHealthReason = "최근 2초간 수신 프레임 없음";
        aHealthLevel = 1;
    } else if (aEflg != 0) {
        aHealthState = aMcpEflgStateName(aEflg);
        if (strcmp(aHealthState, "RX_OVERRUN") == 0)
            aHealthReason = "MCP2515 수신 버퍼 오버런";
        else if (strcmp(aHealthState, "ERROR_PASSIVE") == 0)
            aHealthReason = "MCP2515 에러 패시브";
        else
            aHealthReason = "MCP2515 오류 카운터 경고";
        aHealthLevel = aMcpEflgSeverity(aEflg);
    }
    const bool aConnected = aDriverOk && aTaskAlive && aFresh && ((aEflg & 0x20U) == 0);
    cJSON_AddBoolToObject(ach, "driver_initialized", (bool)aChannelDiag.driverInitialized);
    cJSON_AddBoolToObject(ach, "driver_ok", aDriverOk);
    cJSON_AddBoolToObject(ach, "connected", aConnected);
    cJSON_AddBoolToObject(ach, "fresh", aFresh);
    cJSON_AddBoolToObject(ach, "task_alive", aTaskAlive);
    cJSON_AddNumberToObject(ach, "frame_age_ms", aFrameAgeMs);
    cJSON_AddNumberToObject(ach, "loop_age_ms", aLoopAgeMs);
    cJSON_AddStringToObject(ach, "health_state", aHealthState);
    cJSON_AddStringToObject(ach, "health_reason", aHealthReason);
    cJSON_AddNumberToObject(ach, "health_level", aHealthLevel);
    cJSON_AddBoolToObject(ach, "eflg_rx1_overrun", (aEflg & 0x80U) != 0);
    cJSON_AddBoolToObject(ach, "eflg_rx0_overrun", (aEflg & 0x40U) != 0);
    cJSON_AddBoolToObject(ach, "eflg_tx_bus_off",  (aEflg & 0x20U) != 0);
    cJSON_AddBoolToObject(ach, "eflg_tx_passive",  (aEflg & 0x10U) != 0);
    cJSON_AddBoolToObject(ach, "eflg_rx_passive",  (aEflg & 0x08U) != 0);
    cJSON_AddBoolToObject(ach, "eflg_tx_warning",  (aEflg & 0x04U) != 0);
    cJSON_AddBoolToObject(ach, "eflg_rx_warning",  (aEflg & 0x02U) != 0);
    cJSON_AddBoolToObject(ach, "eflg_warning",     (aEflg & 0x01U) != 0);
    
    cJSON *bch = cJSON_AddObjectToObject(channels, "b_channel");
    cJSON_AddNumberToObject(bch, "frames_received", (uint32_t)bChannelDiag.framesReceivedTotal);
    cJSON_AddNumberToObject(bch, "frame_hz", (double)(float)bChannelDiag.frameHz);
    cJSON_AddNumberToObject(bch, "filtered_hz", (double)(float)bChannelDiag.filteredHz);
    cJSON_AddNumberToObject(bch, "frames_880", (uint32_t)bChannelDiag.frames880);
    cJSON_AddNumberToObject(bch, "frames_target", (uint32_t)bChannelDiag.frames880);
    cJSON_AddNumberToObject(bch, "frames_921", (uint32_t)bChannelDiag.frames921);
    cJSON_AddNumberToObject(bch, "frames_923", (uint32_t)bChannelDiag.frames923);
    cJSON_AddNumberToObject(bch, "target_id", (uint32_t)nagTargetId);
    cJSON_AddNumberToObject(bch, "id_880_period_ms", (uint32_t)period880Ms);
    cJSON_AddNumberToObject(bch, "id_target_period_ms", (uint32_t)period880Ms);
    cJSON_AddNumberToObject(bch, "id_921_period_ms", (uint32_t)period921Ms);
    cJSON_AddNumberToObject(bch, "id_923_period_ms", (uint32_t)period923Ms);
    cJSON_AddNumberToObject(bch, "das_hands_state", (uint32_t)bChannelDiag.dasHandsOnStateRx);
    cJSON_AddStringToObject(bch, "das_hands_state_name", dasHandsOnStateName((uint8_t)bChannelDiag.dasHandsOnStateRx));
    cJSON_AddStringToObject(bch, "das_hands_state_group", dasHandsOnStateGroup((uint8_t)bChannelDiag.dasHandsOnStateRx));
    cJSON_AddNumberToObject(bch, "das_hands_warn_level", (uint32_t)dasHandsOnWarningLevel((uint8_t)bChannelDiag.dasHandsOnStateRx));
    cJSON_AddBoolToObject(bch, "das_hands_warning", dasHandsOnStateIsWarning((uint8_t)bChannelDiag.dasHandsOnStateRx));
    cJSON_AddNumberToObject(bch, "das_source_id", (uint32_t)bChannelDiag.dasStatusSourceId);
    cJSON_AddNumberToObject(bch, "last_das_status_rx_ms", (uint32_t)bChannelDiag.lastDasStatusRxMs);
    cJSON_AddNumberToObject(bch, "nag_mode", (uint32_t)nagModeClamp((uint8_t)bChannelDiag.nagMode));
    cJSON_AddStringToObject(bch, "nag_mode_name", nagModeName((uint8_t)bChannelDiag.nagMode));
    cJSON_AddBoolToObject(bch, "nag_ap_only", (bool)nagApOnlyRuntime);
    cJSON_AddBoolToObject(bch, "nag_ap_active",
                         nagApStateAllowsInjection((uint8_t)bChannelDiag.dasAutopilotStateRx));
    cJSON_AddNumberToObject(bch, "echo_count", (uint32_t)bChannelDiag.echoCount);
    cJSON_AddNumberToObject(bch, "tx_attempt_count", (uint32_t)bChannelDiag.txAttemptCount);
    cJSON_AddNumberToObject(bch, "tx_success_count", (uint32_t)bChannelDiag.txSuccessCount);
    cJSON_AddNumberToObject(bch, "echo_confirm_count", (uint32_t)bChannelDiag.echoConfirmCount);
    cJSON_AddNumberToObject(bch, "tx_latency_us", (uint32_t)bChannelDiag.txLatencyUs);
    cJSON_AddNumberToObject(bch, "echo_latency_us", (uint32_t)bChannelDiag.echoLatUs);
    cJSON_AddNumberToObject(bch, "echo_drop_late", (uint32_t)bChannelDiag.echoDroppedLate);
    cJSON_AddNumberToObject(bch, "skip_runtime_or_inactive", (uint32_t)bChannelDiag.skipRuntimeOrInactive);
    cJSON_AddNumberToObject(bch, "skip_warmup", (uint32_t)bChannelDiag.skipWarmup);
    cJSON_AddNumberToObject(bch, "skip_ap_state", (uint32_t)bChannelDiag.skipApState);
    cJSON_AddNumberToObject(bch, "skip_hands_on", (uint32_t)bChannelDiag.skipHandsOn);
    cJSON_AddNumberToObject(bch, "skip_das_state", (uint32_t)bChannelDiag.skipDasState);
    cJSON_AddBoolToObject(bch, "nag_ready", (bool)bChannelDiag.nagReady);
    cJSON_AddStringToObject(bch, "nag_readiness", nagReadinessName((uint8_t)bChannelDiag.nagReadiness));
    cJSON_AddNumberToObject(bch, "nag_warmup_frames", (uint32_t)bChannelDiag.nagWarmupFramesSeen);
    cJSON_AddNumberToObject(bch, "twai_state_code", (uint32_t)bChannelDiag.twaiStateCode);
    cJSON_AddNumberToObject(bch, "last_frame_id", (uint32_t)bChannelDiag.frameIdReceived);
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
    cJSON_AddNumberToObject(bch, "twai_rx_err_now", (uint32_t)bChannelDiag.twaiRxErrNow);
    cJSON_AddNumberToObject(bch, "twai_tx_err_now", (uint32_t)bChannelDiag.twaiTxErrNow);
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
                            (twaiStateCode == 1 || twaiStateCode == 3) &&
                            twaiStateCode != 2 &&
                            bFresh &&
                            bTaskAlive;
    const char *bHealthState = "OK";
    const char *bHealthReason = "정상 수신 중";
    uint8_t bHealthLevel = 0;
    if (!(bool)bChannelDiag.nagTaskCreated) {
        bHealthState = "TASK_OFF";
        bHealthReason = "통합 CAN 태스크 미생성";
        bHealthLevel = 2;
    } else if (!bDriverOk) {
        bHealthState = "DRIVER_FAILED";
        bHealthReason = "TWAI 드라이버 초기화 실패";
        bHealthLevel = 2;
    } else if (!bTaskAlive) {
        bHealthState = "LOOP_STALLED";
        bHealthReason = "B채널 폴링 루프 지연";
        bHealthLevel = 2;
    } else if (twaiStateCode == 2) {
        bHealthState = "BUS_OFF";
        bHealthReason = "TWAI BUS-OFF";
        bHealthLevel = 2;
    } else if (twaiStateCode == 3) {
        bHealthState = "RECOVERING";
        bHealthReason = "TWAI 복구 진행 중";
        bHealthLevel = 1;
    } else if (!bFresh) {
        bHealthState = "NO_FRAMES";
        bHealthReason = "최근 2초간 수신 프레임 없음";
        bHealthLevel = 1;
    } else if ((uint32_t)bChannelDiag.twaiTxErrNow >= 96U ||
               (uint32_t)bChannelDiag.twaiRxErrNow >= 96U) {
        bHealthState = "ERROR_WARNING";
        bHealthReason = "TWAI 오류 카운터 경고";
        bHealthLevel = 1;
    }
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
    cJSON_AddStringToObject(bch, "health_state", bHealthState);
    cJSON_AddStringToObject(bch, "health_reason", bHealthReason);
    cJSON_AddNumberToObject(bch, "health_level", bHealthLevel);


    const uint32_t aFramesReceived = (uint32_t)aChannelDiag.framesReceivedTotal;
    const uint32_t bFramesReceived = (uint32_t)bChannelDiag.framesReceivedTotal;
    const uint32_t totalFramesReceived = aFramesReceived + bFramesReceived;

    // CAN bus diagnostics
    twai_status_info_t twaiStatus;
    cJSON *can = cJSON_AddObjectToObject(root, "can");
    cJSON_AddStringToObject(can, "state_channel", "B");
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
    const uint32_t aFramesSent = appHandler ? (uint32_t)appHandler->framesSent : 0;
    const uint32_t bFramesSent = (uint32_t)bChannelDiag.echoCount;
    cJSON_AddNumberToObject(can, "frames_sent", aFramesSent + bFramesSent);
    cJSON_AddNumberToObject(can, "frames_sent_a", aFramesSent);
    cJSON_AddNumberToObject(can, "frames_sent_b", bFramesSent);

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

static esp_err_t summonUnlockHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, summonUnlockRuntime,
                                kWebSupportsSummonUnlock,
                                kNvsKeySummonUnlock, "SUMMON_UNLOCK_HW3");
}

static esp_err_t summonConditionLimitHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, summonConditionLimitRuntime,
                                kWebSupportsSummonUnlock,
                                kNvsKeySummonConditionLimit,
                                "SUMMON_CONDITION_LIMIT");
}

static esp_err_t nagKillerHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, nagKillerRuntime, kWebSupportsNagKiller, kNvsKeyNagKiller, "NAG_KILLER");
}

static esp_err_t nagApOnlyHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, nagApOnlyRuntime, kWebSupportsNagKiller,
                                kNvsKeyNagApOnly, "NAG_AP_ONLY");
}


static esp_err_t tsllcHandler(httpd_req_t *req)
{
    // TSLLC 토글: 스톱사인/신호등 자동 정지 + 앞차 있을 때 초록불 자동 출발 (ID 1021 Mux0 주입)
    return featureToggleHandler(req, tsllcRuntime, kWebSupportsTsllc, kNvsKeyTsllc, "TSLLC");
}

static esp_err_t aChannelTxHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, aChannelTxRuntime, true, kNvsKeyAChTx, "A_CHANNEL_TX");
}

static void applyAChannelRuntimeSettings()
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
    eventLogPush(EV_A_SPI_TARGET,
                 (uint16_t)(uint8_t)aChannelDiag.aTec,
                 (uint16_t)(uint8_t)aChannelDiag.aRec,
                 spiMhz);
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
    applyAChannelRuntimeSettings();
    char buf[80];
    snprintf(buf, sizeof(buf), "[Web] A MCP2515 mode: %s", enabled ? "One-Shot" : "Normal");
    logRing.push(buf, millis());
    eventLogPush(EV_FEATURE_STATE,
                 (uint16_t)bChannelDiag.twaiTxErrNow,
                 (uint16_t)bChannelDiag.twaiRxErrNow,
                 eventFeatureStateDetail());
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
    eventLogPush(EV_FEATURE_STATE,
                 (uint16_t)bChannelDiag.twaiTxErrNow,
                 (uint16_t)bChannelDiag.twaiRxErrNow,
                 eventFeatureStateDetail());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ─── GET /api/nag-config  ────────────────────────────────────────────────────
// 현재 NagConfig를 JSON으로 반환 (v2 /api/config 에 해당)
static void addNagModeJson(cJSON *root, uint8_t mode)
{
    mode = nagModeClamp(mode);
    cJSON_AddNumberToObject(root, "mode", mode);
    cJSON_AddStringToObject(root, "modeStr", nagModeName(mode));
    cJSON_AddStringToObject(root, "modeSummary", nagModeSummary(mode));
    cJSON_AddBoolToObject(root, "defaultMode", mode == kNagModeDefault);
    cJSON_AddBoolToObject(root, "requiresContext", (bool)nagApOnlyRuntime);
}

static esp_err_t nagConfigGetHandler(httpd_req_t *req)
{
    NagConfig c;
    portENTER_CRITICAL(&nagCfgMux);
    c = nagConfig;
    portEXIT_CRITICAL(&nagCfgMux);
    c.mode = nagModeClamp(c.mode);

    cJSON *root = cJSON_CreateObject();
    addNagModeJson(root, c.mode);
    cJSON_AddNumberToObject(root, "targetId", kNagFixedTargetId);
    cJSON_AddNumberToObject(root, "torqueMinNm", -1.8);
    cJSON_AddNumberToObject(root, "torqueMaxNm", 1.8);
    cJSON_AddNumberToObject(root, "apStateFreshMs", 1000);
    cJSON_AddBoolToObject(root, "apOnly", (bool)nagApOnlyRuntime);
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
    if (webDownloadBusy(handlerStartMs)) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_set_hdr(req, "Retry-After", "3");
        httpd_resp_send(req, "{\"busy\":\"logs_bundle\"}", HTTPD_RESP_USE_STRLEN);
        webHealthRecordDuration(gWebNagStatsLastDurMs, gWebNagStatsMaxDurMs, handlerStartMs);
        return ESP_OK;
    }
    uint32_t nowMs = handlerStartMs;
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        webHealthRecordDuration(gWebNagStatsLastDurMs, gWebNagStatsMaxDurMs, handlerStartMs);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    cJSON_AddNumberToObject(root, "rx",       (uint32_t)bChannelDiag.frames880);
    cJSON_AddNumberToObject(root, "echo",     (uint32_t)bChannelDiag.echoCount);
    cJSON_AddNumberToObject(root, "txAttempts", (uint32_t)bChannelDiag.txAttemptCount);
    cJSON_AddNumberToObject(root, "txSuccess", (uint32_t)bChannelDiag.txSuccessCount);
    cJSON_AddNumberToObject(root, "echoConfirmed", (uint32_t)bChannelDiag.echoConfirmCount);
    cJSON_AddNumberToObject(root, "txFail",   (uint32_t)bChannelDiag.txFail);
    cJSON_AddNumberToObject(root, "txLatUs",  (uint32_t)bChannelDiag.txLatencyUs);
    cJSON_AddNumberToObject(root, "latUs",    (uint32_t)bChannelDiag.echoLatUs);
    cJSON_AddNumberToObject(root, "ho",       (uint8_t)bChannelDiag.realHo);
    cJSON_AddNumberToObject(root, "torqueNm", (double)(float)bChannelDiag.realTorqueNm);
    cJSON_AddNumberToObject(root, "busoffCount", (uint32_t)bChannelDiag.busoffCount);
    cJSON_AddNumberToObject(root, "dasHandsState", (uint32_t)bChannelDiag.dasHandsOnStateRx);
    cJSON_AddStringToObject(root, "dasHandsStateName", dasHandsOnStateName((uint8_t)bChannelDiag.dasHandsOnStateRx));
    cJSON_AddStringToObject(root, "dasHandsStateGroup", dasHandsOnStateGroup((uint8_t)bChannelDiag.dasHandsOnStateRx));
    cJSON_AddNumberToObject(root, "dasHandsWarnLevel", (uint32_t)dasHandsOnWarningLevel((uint8_t)bChannelDiag.dasHandsOnStateRx));
    cJSON_AddBoolToObject(root, "dasHandsWarning", dasHandsOnStateIsWarning((uint8_t)bChannelDiag.dasHandsOnStateRx));
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
    cJSON_AddBoolToObject(root, "nagReady", (bool)bChannelDiag.nagReady);
    cJSON_AddNumberToObject(root, "nagReadinessCode", (uint8_t)bChannelDiag.nagReadiness);
    cJSON_AddStringToObject(root, "nagReadiness", nagReadinessName((uint8_t)bChannelDiag.nagReadiness));
    cJSON_AddNumberToObject(root, "warmupElapsedMs",
        webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.nagWarmupStartMs));
    cJSON_AddNumberToObject(root, "warmupRequiredMs", kNagWarmupMs);
    cJSON_AddNumberToObject(root, "warmupFrames", (uint32_t)bChannelDiag.nagWarmupFramesSeen);
    cJSON_AddNumberToObject(root, "warmupRequiredFrames", kNagWarmupTargetFrames);
    cJSON_AddNumberToObject(root, "skipWarmup", (uint32_t)bChannelDiag.skipWarmup);
    cJSON_AddBoolToObject(root, "apOnly", (bool)nagApOnlyRuntime);
    cJSON_AddBoolToObject(root, "apActive",
                         nagApStateAllowsInjection((uint8_t)bChannelDiag.dasAutopilotStateRx));

    uint8_t mode = nagModeClamp((uint8_t)bChannelDiag.nagMode);
    addNagModeJson(root, mode);
    cJSON_AddNumberToObject(root, "targetId", kNagFixedTargetId);
    // Nag Mode 1/2 진단
    cJSON_AddNumberToObject(root, "dasApState",  (uint8_t)bChannelDiag.dasAutopilotStateRx);
    cJSON_AddNumberToObject(root, "modeBPhase",  (uint8_t)bChannelDiag.modeBPhase);
    cJSON_AddNumberToObject(root, "modeBInjects",(uint32_t)bChannelDiag.modeBInjectCount);
    cJSON_AddNumberToObject(root, "modeBLastNm", (double)(float)bChannelDiag.modeBLastTorqueNm);
    cJSON_AddNumberToObject(root, "lastTxNm", (double)(float)bChannelDiag.lastTxTorqueNm);
    cJSON_AddNumberToObject(root, "lastTxHo", (uint8_t)bChannelDiag.lastTxHandsOn);
    // BUS-OFF 복구 모드 상태
    bool isSoftMode = gWebDriverB ? gWebDriverB->getSoftRecovery() : false;
    cJSON_AddBoolToObject(root, "boSoftMode", isSoftMode);
    uint32_t softFallback = gWebDriverB ? gWebDriverB->getSoftRecoveryFallbackCount() : 0;
    cJSON_AddNumberToObject(root, "boSoftFallback", softFallback);
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
    cJSON_AddNumberToObject(root, "lastEchoAgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.lastEchoTxMs));
    cJSON_AddNumberToObject(root, "lastEchoConfirmAgeMs", webSafeAgeMs(nowMs, (uint32_t)bChannelDiag.lastEchoRxMs));
    char rawPayload[32];
    formatCanPayloadStable(bChannelDiag.raw880Seq, bChannelDiag.raw880Low,
                           bChannelDiag.raw880High, rawPayload, sizeof(rawPayload));
    cJSON_AddStringToObject(root, "raw880", rawPayload);
    formatCanPayloadStable(bChannelDiag.rawDasSeq, bChannelDiag.rawDasLow,
                           bChannelDiag.rawDasHigh, rawPayload, sizeof(rawPayload));
    cJSON_AddStringToObject(root, "rawDas", rawPayload);

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

// ─── POST /api/nag-mode?m=1|2 ─────────────────────────────────────────
// MODE 1/2 중 하나를 선택하고 NVS에 저장한다.
static esp_err_t nagModeHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char queryBuf[32] = {};
    char modeBuf[4] = {};
    if (httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf)) != ESP_OK ||
        httpd_query_key_value(queryBuf, "m", modeBuf, sizeof(modeBuf)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "m=1|2 required");
        return ESP_FAIL;
    }
    int requested = atoi(modeBuf);
    if (requested != kNagMode1 && requested != kNagMode2) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid nag mode");
        return ESP_FAIL;
    }
    NagConfig nc;
    portENTER_CRITICAL(&nagCfgMux);
    nc = nagConfig;
    portEXIT_CRITICAL(&nagCfgMux);
    nc.mode = static_cast<uint8_t>(requested);
    portENTER_CRITICAL(&nagCfgMux);
    nagConfig = nc;
    portEXIT_CRITICAL(&nagCfgMux);
    nagCfgSave(nc);
    bChannelDiag.nagMode = nc.mode;
    eventLogPush(EV_NAG_MODE, (uint16_t)bChannelDiag.twaiTxErrNow,
                 (uint16_t)bChannelDiag.twaiRxErrNow, (uint32_t)nc.mode);

    char logBuf[64];
    snprintf(logBuf, sizeof(logBuf), "[NAG] 모드 변경: %s", nagModeName(nc.mode));
    logRing.push(logBuf, millis());

    return nagConfigGetHandler(req);  // 변경된 설정 반환
}

// ─── POST /api/nag-update  ───────────────────────────────────────────────────
// targetId/토크 테이블 변경 요청은 무시하고 880 고정.
static esp_err_t nagUpdateHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    logRing.push("[NAG] /api/nag-update — ID 880/토크 상한 고정 유지", millis());
    return nagConfigGetHandler(req);
}

// ─── POST /api/nag-reset  ────────────────────────────────────────────────────
// 실차 확인 및 최신 원본 기준 기본 MODE 2 + AP 전용으로 리셋
static esp_err_t nagResetHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    NagConfig nc;
    nagCfgDefaults(nc);
    portENTER_CRITICAL(&nagCfgMux);
    nagConfig = nc;
    portEXIT_CRITICAL(&nagCfgMux);
    nagCfgSave(nc);
    nagApOnlyRuntime = kNagApOnlyDefaultEnabled;
    nvsWriteBool(kNvsKeyNagApOnly, nagApOnlyRuntime);
    bChannelDiag.nagMode = nc.mode;
    logRing.push("[NAG] 설정 리셋 → MODE 2 + AP 전용(기본)", millis());
    return nagConfigGetHandler(req);
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

static void csvEscapeCell(const char *in, char *out, size_t out_n) {
    if (!out || out_n == 0) return;
    if (out_n < 3) {
        out[0] = '\0';
        return;
    }
    size_t j = 0;
    out[j++] = '"';
    if (in) {
        for (size_t i = 0; in[i] && j + 2 < out_n; ++i) {
            if (in[i] == '"') {
                if (j + 2 >= out_n) break;
                out[j++] = '"';
                out[j++] = '"';
            } else {
                out[j++] = in[i];
            }
        }
    }
    if (j < out_n - 1) out[j++] = '"';
    out[j] = '\0';
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

// POST /api/user-marker
// 사용자가 임의 이벤트 구간의 시작/종료 지점을 사후 분석 로그에 찍는 일반 마커.
static esp_err_t userMarkerHandler(httpd_req_t *req) {
    uint32_t now = millis();
    bool activeBefore = (bool)userMarkerActive;
    uint32_t detail = activeBefore ? kUserMarkerEnd : kUserMarkerStart;
    char queryBuf[64] = {};
    if (httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf)) == ESP_OK) {
        char typeBuf[24] = {};
        if (httpd_query_key_value(queryBuf, "type", typeBuf, sizeof(typeBuf)) == ESP_OK) {
            if (strcmp(typeBuf, "start") == 0) detail = kUserMarkerStart;
            else if (strcmp(typeBuf, "end") == 0) detail = kUserMarkerEnd;
        }
    }
    bool activeAfter = detail == kUserMarkerStart;
    // 이벤트 링이 alert 폭주로 밀려도 timeseries의 dUserMark가 20분 동안 완료 구간 기준점을 보존한다.
    if (detail == kUserMarkerEnd && activeBefore) {
        userMarkerCount = (uint32_t)userMarkerCount + 1;
    }
    userMarkerLastMs = now;
    userMarkerLastDetail = detail;
    userMarkerActive = activeAfter;

    uint16_t tec = (uint16_t)(uint32_t)bChannelDiag.twaiTxErrNow;
    uint16_t rec = (uint16_t)(uint32_t)bChannelDiag.twaiRxErrNow;
    eventLogPush(EV_USER_MARK, tec, rec, detail);
    uint8_t nagMode = nagModeClamp((uint8_t)bChannelDiag.nagMode);

    const char *detailName = userMarkerDetailName(detail);
    char raw880[32];
    char rawDas[32];
    formatCanPayloadStable(bChannelDiag.raw880Seq, bChannelDiag.raw880Low,
                           bChannelDiag.raw880High, raw880, sizeof(raw880));
    formatCanPayloadStable(bChannelDiag.rawDasSeq, bChannelDiag.rawDasLow,
                           bChannelDiag.rawDasHigh, rawDas, sizeof(rawDas));
    NagMarkerSnapshot markerSnapshot = {};
    markerSnapshot.tMs = now;
    markerSnapshot.detail = detail;
    markerSnapshot.txAttempts = (uint32_t)bChannelDiag.txAttemptCount;
    markerSnapshot.txSuccess = (uint32_t)bChannelDiag.txSuccessCount;
    markerSnapshot.echoConfirmed = (uint32_t)bChannelDiag.echoConfirmCount;
    markerSnapshot.txLatencyUs = (uint32_t)bChannelDiag.txLatencyUs;
    markerSnapshot.echoLatencyUs = (uint32_t)bChannelDiag.echoLatUs;
    markerSnapshot.echoDrop = (uint32_t)bChannelDiag.echoDroppedLate;
    markerSnapshot.dasSourceId = (uint16_t)(uint32_t)bChannelDiag.dasStatusSourceId;
    markerSnapshot.mode = nagMode;
    markerSnapshot.readiness = (uint8_t)bChannelDiag.nagReadiness;
    markerSnapshot.decision = (uint8_t)bChannelDiag.nagLastDecision;
    markerSnapshot.apState = (uint8_t)bChannelDiag.dasAutopilotStateRx;
    markerSnapshot.phase = (uint8_t)bChannelDiag.modeBPhase;
    markerSnapshot.realHo = (uint8_t)bChannelDiag.realHo;
    markerSnapshot.dasState = (uint8_t)bChannelDiag.dasHandsOnStateRx;
    markerSnapshot.txHo = (uint8_t)bChannelDiag.lastTxHandsOn;
    markerSnapshot.realTorqueNm = (float)bChannelDiag.realTorqueNm;
    markerSnapshot.txTorqueNm = (float)bChannelDiag.lastTxTorqueNm;
    strncpy(markerSnapshot.raw880, raw880, sizeof(markerSnapshot.raw880) - 1);
    strncpy(markerSnapshot.rawDas, rawDas, sizeof(markerSnapshot.rawDas) - 1);
    nagMarkerSnapshotPush(markerSnapshot);
    char msg[192];
    snprintf(msg, sizeof(msg),
        "[USER-MARK] %s Mode=%s Ready=%s AP=%u Phase=%u HO=%u DAS=%s(0x%02X L%u warn=%u) Last=%s TEC=%u/REC=%u",
        detailName,
        nagModeName(nagMode),
        nagReadinessName((uint8_t)bChannelDiag.nagReadiness),
        (unsigned)(uint8_t)bChannelDiag.dasAutopilotStateRx,
        (unsigned)(uint8_t)bChannelDiag.modeBPhase,
        (unsigned)(uint8_t)bChannelDiag.realHo,
        dasHandsOnStateName((uint8_t)bChannelDiag.dasHandsOnStateRx),
        (unsigned)(uint8_t)bChannelDiag.dasHandsOnStateRx,
        (unsigned)dasHandsOnWarningLevel((uint8_t)bChannelDiag.dasHandsOnStateRx),
        dasHandsOnStateIsWarning((uint8_t)bChannelDiag.dasHandsOnStateRx) ? 1U : 0U,
        nagDecisionName((uint8_t)bChannelDiag.nagLastDecision),
        (unsigned)tec,
        (unsigned)rec);
    logRing.push(msg, now);
    char txMsg[192];
    snprintf(txMsg, sizeof(txMsg),
        "[USER-MARK-TX] Try/OK/Ack=%u/%u/%u Lat=%u/%uus TX=%.2fNm/HO%u Real=%.2fNm Drop=%u",
        (unsigned)bChannelDiag.txAttemptCount,
        (unsigned)bChannelDiag.txSuccessCount,
        (unsigned)bChannelDiag.echoConfirmCount,
        (unsigned)bChannelDiag.txLatencyUs,
        (unsigned)bChannelDiag.echoLatUs,
        (double)(float)bChannelDiag.lastTxTorqueNm,
        (unsigned)(uint8_t)bChannelDiag.lastTxHandsOn,
        (double)(float)bChannelDiag.realTorqueNm,
        (unsigned)bChannelDiag.echoDroppedLate);
    logRing.push(txMsg, now);
    char rawMsg[192];
    snprintf(rawMsg, sizeof(rawMsg),
        "[USER-MARK-RAW] C880/921/923=%u/%u/%u DAS_ID=%u 880=[%s] DAS=[%s]",
        (unsigned)bChannelDiag.frames880,
        (unsigned)bChannelDiag.frames921,
        (unsigned)bChannelDiag.frames923,
        (unsigned)(uint32_t)bChannelDiag.dasStatusSourceId,
        raw880, rawDas);
    logRing.push(rawMsg, now);

    httpd_resp_set_type(req, "application/json");
    char body[192];
    snprintf(body, sizeof(body), "{\"ok\":true,\"timestamp_ms\":%u,\"count\":%u,\"log_count\":%u,\"active\":%s,\"detail\":%u,\"detail_text\":\"%s\"}",
        (unsigned)now, (unsigned)(uint32_t)userMarkerCount,
        (unsigned)tsDelta((uint32_t)userMarkerCount, (uint32_t)tsBaseUserMark),
        activeAfter ? "true" : "false", (unsigned)detail, detailName);
    httpd_resp_sendstr(req, body);
    return ESP_OK;
}

// GET /api/logs-bundle — 통합 로그 번들 (런타임 + BUS-OFF + 스냅샷 + 시계열 + 이벤트)
static esp_err_t logsBundleHandler(httpd_req_t *req) {
    const uint32_t handlerStartMs = millis();
    webHealthMark(gWebLogsBundleReqCount, gWebLogsBundleLastMs, handlerStartMs);
    // UI polling만 짧게 보호한다. 전 행 스트리밍이라 3분 보호 창은 필요하지 않다.
    gWebDownloadBusyUntilMs = handlerStartMs + 30000UL;
    const esp_err_t wdtDeleteErr = esp_task_wdt_delete(NULL);  // 다운로드 중 TCP send 블로킹 허용
    const bool wdtDetached = (wdtDeleteErr == ESP_OK);
    logsBundleSerialTrace(wdtDetached ? "start wdt=off" : "start wdt=keep", handlerStartMs);
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
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Connection", "close");
    char line[1152];
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
    logsBundleSerialTrace("meta sent", handlerStartMs);

    // 섹션 1: 런타임 로그 (logRing 전체) — 스트리밍 (스택 절약, kCapacity=256 대응)
    logsBundleSerialTrace("section1 runtime", handlerStartMs);
    httpd_resp_sendstr_chunk(req, "=== [1] 런타임 로그 ===\r\n");
    {
        uint32_t h = logRing.currentHead();
        uint32_t oldest = (h > LogRingBuffer::kCapacity) ? (h - LogRingBuffer::kCapacity) : 0;
        if (h == 0) {
            httpd_resp_sendstr_chunk(req, "(로그 없음)\r\n");
        } else {
            for (uint32_t i = oldest; i < h; i++) {
                esp_task_wdt_reset();
                const LogRingBuffer::Entry &e = logRing.at(i);
                formatLogTimestamp(e.timestamp_ms, tsBuf, sizeof(tsBuf));
                snprintf(line, sizeof(line), "[%s] %s\r\n", tsBuf, e.msg);
                if (httpd_resp_sendstr_chunk(req, line) != ESP_OK) {
                    logsBundleSerialTrace("fail section1", handlerStartMs);
                    if (wdtDetached) esp_task_wdt_add(NULL);
                    gWebDownloadBusyUntilMs = 0;
                    return ESP_FAIL;
                }
            }
        }
    }

    // 섹션 2: BUS-OFF 이벤트 로그
    logsBundleSerialTrace("section2 busoff", handlerStartMs);
    httpd_resp_sendstr_chunk(req, "\r\n=== [2] BUS-OFF 이벤트 로그 ===\r\n");
    httpd_resp_sendstr_chunk(req, "seq,wall_time,timestamp_ms,tec,rec,recovery_dur_ms,since_last_ms,recovered\r\n");
    uint32_t h = busOffLog.count();
    if (h == 0) {
        httpd_resp_sendstr_chunk(req, "(BUS-OFF 없음)\r\n");
    } else {
        uint32_t oldest = (h > (uint32_t)BusOffEventLog::kCapacity) ? (h - BusOffEventLog::kCapacity) : 0;
        for (uint32_t i = oldest; i < h; i++) {
            esp_task_wdt_reset();
            const BusOffEvent& ev = busOffLog.at(i);
            formatLogTimestamp(ev.timestampMs, tsBuf, sizeof(tsBuf));
            snprintf(line, sizeof(line), "%u,%s,%u,%u,%u,%u,%u,%u\r\n",
                (unsigned)ev.seqNum, tsBuf, (unsigned)ev.timestampMs,
                (unsigned)ev.tec, (unsigned)ev.rec,
                (unsigned)ev.recoveryDurMs, (unsigned)ev.sinceLastMs, (unsigned)ev.recovered);
            if (httpd_resp_sendstr_chunk(req, line) != ESP_OK) {
                logsBundleSerialTrace("fail section2", handlerStartMs);
                if (wdtDetached) esp_task_wdt_add(NULL);
                gWebDownloadBusyUntilMs = 0;
                return ESP_FAIL;
            }
        }
    }

    // 섹션 3: 채널 상태 스냅샷
    esp_task_wdt_reset();
    logsBundleSerialTrace("section3 snapshot", handlerStartMs);
    httpd_resp_sendstr_chunk(req, "\r\n=== [3] 채널 상태 스냅샷 ===\r\n");
    snprintf(line, sizeof(line), "A채널: RX=%u 280=%u 390=%u 921=%u 1016=%u 1021=%u Summon=%u Blocked=%u\r\n",
        (unsigned)aChannelDiag.framesReceivedTotal,
        (unsigned)aChannelDiag.frames280,
        (unsigned)aChannelDiag.frames390,
        (unsigned)aChannelDiag.frames921,
        (unsigned)aChannelDiag.frames1016,
        (unsigned)aChannelDiag.frames1021,
        (unsigned)aChannelDiag.summonUnlockModifiedCount,
        (unsigned)summonGateDiag.blocked);
    httpd_resp_sendstr_chunk(req, line);
    // A채널 진단 카운터. 큐 등록과 실제 완료를 구분하며 Busy는 하드 오류가 아니다.
    uint32_t aGuardUntil = (uint32_t)aChannelDiag.aTxGuardUntilMs;
    uint32_t aNow = millis();
    bool aGuardActive = aTxGuardActive(aNow);
    snprintf(line, sizeof(line),
        "A진단: TX Q=%u Busy=%u Hard=%u Hard1s=%u/peak=%u/th=%u Done=%u Arb=%u Abort=%u | TEC=%u/peak=%u REC=%u | MERRF=%u | RX-OVR=%u | EFLG=0x%02X/peak=0x%02X | BUS-OFF=%u | Cfg=SPI%lu->%lu/OS%s/Guard%s | Guard=%s/%ums/%s skip=%u count=%u\r\n",
        (unsigned)aChannelDiag.aTxOk,
        (unsigned)aChannelDiag.aTxBusy,
        (unsigned)aChannelDiag.aTxFail,
        (unsigned)(uint8_t)aChannelDiag.aTxFailWindowDelta,
        (unsigned)(uint8_t)aChannelDiag.aTxFailWindowPeak,
        (unsigned)kATxGuardTxFailBurstThreshold,
        (unsigned)aChannelDiag.aTxCompleted,
        (unsigned)aChannelDiag.aTxArbitrationLost,
        (unsigned)aChannelDiag.aTxAborted,
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
            "A수신: RX-OVR=%u (RX0=%u RX1=%u) | RXB0/1=%u/%u | Drain frames/calls=%u/%u | Queue peak/drop=%u/%u | Gap >250/500/1000/2000us=%u/%u/%u/%u | LastOverrun=%s\r\n",
            (unsigned)aChannelDiag.aRxOvrCount,
            (unsigned)aChannelDiag.aRx0OvrCount,
            (unsigned)aChannelDiag.aRx1OvrCount,
            (unsigned)aChannelDiag.aRxBuffer0Frames,
            (unsigned)aChannelDiag.aRxBuffer1Frames,
            (unsigned)aChannelDiag.aRxDrainFrames,
            (unsigned)aChannelDiag.aRxDrainCalls,
            (unsigned)aChannelDiag.aRxQueueHighWater,
            (unsigned)aChannelDiag.aRxQueueDropCount,
            (unsigned)aChannelDiag.loopGapOver250usCount,
            (unsigned)aChannelDiag.loopGapOver500usCount,
            (unsigned)aChannelDiag.loopGapOver1msCount,
            (unsigned)aChannelDiag.loopGapOver2msCount,
            aCanPhaseName((uint8_t)aChannelDiag.lastOverrunPhase));
        httpd_resp_sendstr_chunk(req, line);
    snprintf(line, sizeof(line),
        "기능설정: Summon=%s ECE조건=%s Gate=%s(%s) AP=%u/%ums Ready=%s | TSLLC=%s Ready=%s | Nag=%s %s Scope=%s AP=%u Ready=%s\r\n",
        (bool)summonUnlockRuntime ? "ON" : "OFF",
        (bool)summonConditionLimitRuntime ? "ON" : "OFF",
        summonGateOpen() ? "OPEN" : "BLOCKED",
        summonGateReasonName(aNow),
        (unsigned)(uint8_t)summonGateDiag.apState,
        (unsigned)summonApStableMs(aNow),
        ((bool)summonUnlockRuntime && (bool)aChannelTxRuntime &&
         summonGateOpen() && !aGuardActive) ? "YES" : "NO",
        (bool)tsllcRuntime ? "ON" : "OFF",
        ((bool)tsllcRuntime && (bool)aChannelTxRuntime && !aGuardActive)
            ? "YES" : "NO",
        (bool)nagKillerRuntime ? "ON" : "OFF",
        nagModeName((uint8_t)bChannelDiag.nagMode),
        (bool)nagApOnlyRuntime ? "AP_ONLY" : "ORIGINAL",
        (unsigned)(uint8_t)bChannelDiag.dasAutopilotStateRx,
        (bool)bChannelDiag.nagReady ? "YES" : "NO");
    httpd_resp_sendstr_chunk(req, line);
    snprintf(line, sizeof(line),
        "기능활동: Summon Q/B/H=%u/%u/%u Blocked=%u Last=%ums | TSLLC Q/B/H=%u/%u/%u Modified=%u Last=%ums | A Done/Arb/Abort=%u/%u/%u | Nag TX=%u Last=%ums\r\n",
        (unsigned)summonGateDiag.txOk,
        (unsigned)summonGateDiag.txBusy,
        (unsigned)summonGateDiag.txFail,
        (unsigned)summonGateDiag.blocked,
        (unsigned)((uint32_t)summonGateDiag.lastTxMs
            ? aNow - (uint32_t)summonGateDiag.lastTxMs : 0U),
        (unsigned)aChannelDiag.tsllcTxOk,
        (unsigned)aChannelDiag.tsllcTxBusy,
        (unsigned)aChannelDiag.tsllcTxFail,
        (unsigned)aChannelDiag.tsllcModifiedCount,
        (unsigned)((uint32_t)aChannelDiag.lastTsllcTxMs
            ? aNow - (uint32_t)aChannelDiag.lastTsllcTxMs : 0U),
        (unsigned)aChannelDiag.aTxCompleted,
        (unsigned)aChannelDiag.aTxArbitrationLost,
        (unsigned)aChannelDiag.aTxAborted,
        (unsigned)bChannelDiag.echoCount,
        (unsigned)((uint32_t)bChannelDiag.lastEchoTxMs
            ? aNow - (uint32_t)bChannelDiag.lastEchoTxMs : 0U));
    httpd_resp_sendstr_chunk(req, line);
    snprintf(line, sizeof(line),
        "B채널: RX=%u Filt=%u Try/OK/Ack=%u/%u/%u TxFail=%u TEC=%u REC=%u TECpeak=%u 880=%u 921=%u 923=%u DAS=%u(%s/L%u/warn=%u)@%u Mode=%s Ready=%s(%u/%u) TWAI=%s InitErr=%d/%d\r\n",
        (unsigned)bChannelDiag.framesReceivedTotal,
        (unsigned)bChannelDiag.framesFilteredInTotal,
        (unsigned)bChannelDiag.txAttemptCount,
        (unsigned)bChannelDiag.txSuccessCount,
        (unsigned)bChannelDiag.echoConfirmCount,
        (unsigned)bChannelDiag.txFail,
        (unsigned)bChannelDiag.twaiTxErrNow,
        (unsigned)bChannelDiag.twaiRxErrNow,
        (unsigned)bChannelDiag.twaiTxErrPeak,
        (unsigned)bChannelDiag.frames880,
        (unsigned)bChannelDiag.frames921,
        (unsigned)bChannelDiag.frames923,
        (unsigned)bChannelDiag.dasHandsOnStateRx,
        dasHandsOnStateName((uint8_t)bChannelDiag.dasHandsOnStateRx),
        (unsigned)dasHandsOnWarningLevel((uint8_t)bChannelDiag.dasHandsOnStateRx),
        dasHandsOnStateIsWarning((uint8_t)bChannelDiag.dasHandsOnStateRx) ? 1U : 0U,
        (unsigned)bChannelDiag.dasStatusSourceId,
        nagModeName((uint8_t)bChannelDiag.nagMode),
        nagReadinessName((uint8_t)bChannelDiag.nagReadiness),
        (unsigned)bChannelDiag.nagWarmupFramesSeen,
        (unsigned)kNagWarmupTargetFrames,
        twS,
        gWebDriverB ? gWebDriverB->getLastInstallErr() : -1,
        gWebDriverB ? gWebDriverB->getLastStartErr() : -1);
    httpd_resp_sendstr_chunk(req, line);
    // B채널 심층 진단: TWAI 누적 카운터 + 에코 품질 + 스킵 사유
    snprintf(line, sizeof(line),
        "B심층: ArbLost=%u BusErr=%u TxFailed=%u RxMissed=%u | TxLat/EchoLat=%u/%uus EchoDrop=%u | Skip RT/WARM/AP/HO/DAS=%u/%u/%u/%u/%u\r\n",
        (unsigned)bChannelDiag.bArbLost,
        (unsigned)bChannelDiag.bBusError,
        (unsigned)bChannelDiag.bTxFailed,
        (unsigned)bChannelDiag.bRxMissed,
        (unsigned)bChannelDiag.txLatencyUs,
        (unsigned)bChannelDiag.echoLatUs,
        (unsigned)bChannelDiag.echoDroppedLate,
        (unsigned)bChannelDiag.skipRuntimeOrInactive,
        (unsigned)bChannelDiag.skipWarmup,
        (unsigned)bChannelDiag.skipApState,
        (unsigned)bChannelDiag.skipHandsOn,
        (unsigned)bChannelDiag.skipDasState);
    httpd_resp_sendstr_chunk(req, line);
    {
        uint32_t now = millis();
        uint32_t age880 = (uint32_t)bChannelDiag.last880RxMs ? (now - (uint32_t)bChannelDiag.last880RxMs) : 0;
        uint32_t age921 = (uint32_t)bChannelDiag.last921RxMs ? (now - (uint32_t)bChannelDiag.last921RxMs) : 0;
        uint32_t age923 = (uint32_t)bChannelDiag.last923RxMs ? (now - (uint32_t)bChannelDiag.last923RxMs) : 0;
        uint32_t ageEcho = (uint32_t)bChannelDiag.lastEchoTxMs ? (now - (uint32_t)bChannelDiag.lastEchoTxMs) : 0;
        snprintf(line, sizeof(line),
            "B나그판정: 880=%u(age=%ums) 921=%u(age=%ums) 923=%u(age=%ums) Echo=%u(age=%ums) | Mode=%s Ready=%s AP=%u Phase=%u HO=%u Torque=%.2fNm DAS=%s(0x%02X/L%u/warn=%u)@%u Last=%s\r\n",
            (unsigned)bChannelDiag.frames880, (unsigned)age880,
            (unsigned)bChannelDiag.frames921, (unsigned)age921,
            (unsigned)bChannelDiag.frames923, (unsigned)age923,
            (unsigned)bChannelDiag.echoCount, (unsigned)ageEcho,
            nagModeName((uint8_t)bChannelDiag.nagMode),
            nagReadinessName((uint8_t)bChannelDiag.nagReadiness),
            (unsigned)(uint8_t)bChannelDiag.dasAutopilotStateRx,
            (unsigned)(uint8_t)bChannelDiag.modeBPhase,
            (unsigned)(uint8_t)bChannelDiag.realHo,
            (double)(float)bChannelDiag.realTorqueNm,
            dasHandsOnStateName((uint8_t)bChannelDiag.dasHandsOnStateRx),
            (unsigned)(uint8_t)bChannelDiag.dasHandsOnStateRx,
            (unsigned)dasHandsOnWarningLevel((uint8_t)bChannelDiag.dasHandsOnStateRx),
            dasHandsOnStateIsWarning((uint8_t)bChannelDiag.dasHandsOnStateRx) ? 1U : 0U,
            (unsigned)bChannelDiag.dasStatusSourceId,
            nagDecisionName((uint8_t)bChannelDiag.nagLastDecision));
        httpd_resp_sendstr_chunk(req, line);
        snprintf(line, sizeof(line),
            "B차단사유: OFF=%u WARMUP=%u AP_BLOCK=%u HandsOn=%u DAS_IDLE=%u LateDrop=%u NoDAS_Echo=%u\r\n",
            (unsigned)bChannelDiag.skipRuntimeOrInactive,
            (unsigned)bChannelDiag.skipWarmup,
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
            "사용자마커: count=%u log_count=%u active=%u last_age=%ums detail=%u(%s)\r\n",
            (unsigned)markerCount,
            (unsigned)tsDelta(markerCount, (uint32_t)tsBaseUserMark),
            (bool)userMarkerActive ? 1U : 0U,
            (unsigned)markerAge,
            (unsigned)(uint32_t)userMarkerLastDetail,
            userMarkerDetailName((uint32_t)userMarkerLastDetail));
        httpd_resp_sendstr_chunk(req, line);
    }
    {
        NagMarkerSnapshot snapshots[kNagMarkerSnapshotCapacity] = {};
        uint32_t markerHead = 0;
        portENTER_CRITICAL(&gNagMarkerSnapshotMux);
        markerHead = gNagMarkerSnapshotHead;
        const uint32_t markerStored = std::min<uint32_t>(
            markerHead, static_cast<uint32_t>(kNagMarkerSnapshotCapacity));
        const uint32_t markerStart = markerHead > kNagMarkerSnapshotCapacity
            ? markerHead - kNagMarkerSnapshotCapacity : 0;
        for (uint32_t i = 0; i < markerStored; ++i)
            snapshots[i] = gNagMarkerSnapshots[(markerStart + i) %
                                               kNagMarkerSnapshotCapacity];
        portEXIT_CRITICAL(&gNagMarkerSnapshotMux);
        httpd_resp_sendstr_chunk(req,
            "사용자마커원문: wall_time,timestamp_ms,marker,mode,ready,decision,ap,phase,ho,das,das_source,real_nm,tx_nm,tx_ho,tx_try,tx_ok,echo_confirm,tx_lat_us,echo_lat_us,drop,raw880,raw_das\r\n");
        for (uint32_t i = 0; i < markerStored; ++i) {
            const NagMarkerSnapshot &s = snapshots[i];
            formatLogTimestamp(s.tMs, tsBuf, sizeof(tsBuf));
            snprintf(line, sizeof(line),
                "사용자마커원문: %s,%u,%s,%s,%s,%s,%u,%u,%u,%u,%u,%.2f,%.2f,%u,%u,%u,%u,%u,%u,%u,%s,%s\r\n",
                tsBuf, (unsigned)s.tMs, userMarkerDetailName(s.detail),
                nagModeName(s.mode), nagReadinessName(s.readiness),
                nagDecisionName(s.decision), (unsigned)s.apState,
                (unsigned)s.phase, (unsigned)s.realHo, (unsigned)s.dasState,
                (unsigned)s.dasSourceId,
                (double)s.realTorqueNm, (double)s.txTorqueNm,
                (unsigned)s.txHo, (unsigned)s.txAttempts,
                (unsigned)s.txSuccess, (unsigned)s.echoConfirmed,
                (unsigned)s.txLatencyUs, (unsigned)s.echoLatencyUs,
                (unsigned)s.echoDrop, s.raw880, s.rawDas);
            httpd_resp_sendstr_chunk(req, line);
        }
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

    // 섹션 4: 20분 시계열 로그 (5초 × 240 샘플)
    logsBundleSerialTrace("section4 timeseries", handlerStartMs);
    httpd_resp_sendstr_chunk(req, "\r\n=== [4] 20분 시계열 로그 ===\r\n");
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
        "wall_time,timestamp_ms,busoff,tec,rec,arbLost,busErr,txFail,echo,f880,f921,f923,ho,dasState,dasStateName,dasStateGroup,dasWarnLevel,dasWarning,nagMode,nagModeDefault,dasSource,echoDrop,skipOff,skipAP,skipHO,skipDAS,noDAS,userMark,d880,d921,d923,dEcho,dDrop,dSkipOff,dSkipAP,dSkipHO,dSkipDAS,dNoDAS,dUserMark,lastDecision,intervalDecision,apState,nagPhase,realTorqueNm,nagInject,nagLastNm,age880Ms,ageDasMs,ageEchoMs,dNagInject,aFrames,aFrameHz,aEflg,aEflgState,aEflgPeak,aTec,aRec,aTecPeak,aRecPeak,aTxQueued,aTxBusy,aTxHardError,aTxCompleted,aTxArbitrationLost,aTxAborted,aMerrf,aRxOvr,aRx0Ovr,aRx1Ovr,aRxB0Frames,aRxB1Frames,aRxDrainFrames,aRxDrainCalls,aRxQueueHighWater,aRxQueueDrops,aEflgEvents,aFrameAgeMs,aLoopAgeMs,aGuardActive,aGuardReason,aGuardRemainingMs,aWakeCount,aWakeToSummonTxMs,aWakeAwaitingTx,dAFrames,dATxOk,dATxFail,dAMerrf,dARxOvr,dAEflgEvents,aDriverOk,aTxEnabled,summonEnabled,tsllcEnabled,aOneShotEnabled,aTxGuardEnabled,aSpiMhz,aSpiTargetMhz,bDriverState,nagEnabled,aLoopGapLastUs,aLoopGapPeakUs,aLoopGapOver250us,aLoopGapOver500us,aLoopGapOver1ms,aLoopGapOver2ms,dALoopGapOver2ms,aLastOverrunPhase,summonGateOpen,summonConditionLimit,summonApState,summonApActive,summonParked,summoning,summonApStableMs,summonGateReason,summonInjectReady,summonTxOk,summonTxFail,summonBlocked,dSummonTxOk,dSummonTxFail,dSummonBlocked,tsllcInjectReady,tsllcTxOk,tsllcTxFail,dTsllcTxOk,dTsllcTxFail,nagApOnly,nagApActive,nagInjecting,nagTxOk,dNagTxOk\r\n");
    {
        if (tsN == 0) {
            httpd_resp_sendstr_chunk(req, "(시계열 없음)\r\n");
        } else {
            // 최대 79.7KB 전체 스냅샷 malloc은 TCP 송신용 heap까지 잠식해
            // iPhone 다운로드가 멈출 수 있었다. 한 행씩 짧게 복사해 고정
            // 메모리로 스트리밍한다. 샘플 추가 주기는 5초라 잠금 경합도 작다.
            const size_t streamStart = (tsN < TS_CAP) ? 0 : tsSnapHead;
            for (size_t i = 0; i < tsN; ++i) {
                esp_task_wdt_reset();
                TsSample s;
                timeseriesCopyAt(streamStart + i, s);
                formatLogTimestamp(s.t_ms, tsBuf, sizeof(tsBuf));
                int used = snprintf(line, sizeof(line),
                    "%s,%u,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
                    "%u,%u,%s,%s,%u,%u,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
                    "%u,%u,%u,%u,"
                    "%.2f,%u,%.2f,"
                    "%u,%u,%u,%u",
                    tsBuf, (unsigned)s.t_ms,
                    (unsigned)s.busoff, (unsigned)s.tec, (unsigned)s.rec,
                    (unsigned)s.arbLost, (unsigned)s.busErr, (unsigned)s.txFail,
                    (unsigned)s.echoCnt, (unsigned)s.f880, (unsigned)s.f921, (unsigned)s.f923,
                    (unsigned)s.handsOn, (unsigned)s.dasState,
                    dasHandsOnStateName(s.dasState), dasHandsOnStateGroup(s.dasState),
                    (unsigned)dasHandsOnWarningLevel(s.dasState), dasHandsOnStateIsWarning(s.dasState) ? 1U : 0U,
                    (unsigned)s.nagMode, (unsigned)s.nagModeDefault, (unsigned)s.dasSourceId,
                    (unsigned)s.echoDrop, (unsigned)s.skipRuntime,
                    (unsigned)s.skipAp, (unsigned)s.skipHandsOn, (unsigned)s.skipDas,
                    (unsigned)s.noDasEcho, (unsigned)s.userMark,
                    (unsigned)s.d880, (unsigned)s.d921, (unsigned)s.d923,
                    (unsigned)s.dEcho, (unsigned)s.dDrop, (unsigned)s.dSkipRuntime,
                    (unsigned)s.dSkipAp, (unsigned)s.dSkipHandsOn, (unsigned)s.dSkipDas,
                    (unsigned)s.dNoDasEcho, (unsigned)s.dUserMark,
                    (unsigned)s.lastDecision,
                    (unsigned)s.intervalDecision,
                    (unsigned)s.apState, (unsigned)s.modeBPhase,
                    (double)s.realTorqueNm,
                    (unsigned)s.modeBInject, (double)s.modeBLastNm,
                    (unsigned)s.age880Ms, (unsigned)s.ageDasMs,
                    (unsigned)s.ageEchoMs, (unsigned)s.dModeBInject);
                if (used < 0 || (size_t)used >= sizeof(line)) continue;
                snprintf(line + used, sizeof(line) - (size_t)used,
                    ",%u,%.1f,%u,%s,%u,%u,%u,%u,%u,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%s,%u,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%s,"
                    "%u,%u,%u,%u,%u,%u,%u,%s,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
                    (unsigned)s.aFrames, (double)s.aFrameHz,
                    (unsigned)s.aEflg, aMcpEflgStateName(s.aEflg), (unsigned)s.aEflgPeak,
                    (unsigned)s.aTec, (unsigned)s.aRec, (unsigned)s.aTecPeak, (unsigned)s.aRecPeak,
                    (unsigned)s.aTxOk, (unsigned)s.aTxBusy, (unsigned)s.aTxFail,
                    (unsigned)s.aTxCompleted, (unsigned)s.aTxArbitrationLost,
                    (unsigned)s.aTxAborted, (unsigned)s.aMerrf,
                    (unsigned)s.aRxOvr, (unsigned)s.aRx0Ovr, (unsigned)s.aRx1Ovr,
                    (unsigned)s.aRxBuffer0Frames, (unsigned)s.aRxBuffer1Frames,
                    (unsigned)s.aRxDrainFrames, (unsigned)s.aRxDrainCalls,
                    (unsigned)s.aRxQueueHighWater, (unsigned)s.aRxQueueDrops,
                    (unsigned)s.aEflgEvents,
                    (unsigned)s.aFrameAgeMs, (unsigned)s.aLoopAgeMs,
                    (unsigned)s.aGuardActive, aTxGuardReasonName(s.aGuardReason),
                    (unsigned)s.aGuardRemainingMs,
                    (unsigned)s.aWakeCount, (unsigned)s.aWakeToSummonTxMs, (unsigned)s.aWakeAwaitingTx,
                    (unsigned)s.dAFrames, (unsigned)s.dATxOk, (unsigned)s.dATxFail,
                    (unsigned)s.dAMerrf, (unsigned)s.dARxOvr, (unsigned)s.dAEflgEvents,
                    (unsigned)s.aDriverOk, (unsigned)s.aTxEnabled,
                    (unsigned)s.summonEnabled, (unsigned)s.tsllcEnabled,
                    (unsigned)s.aOneShotEnabled, (unsigned)s.aTxGuardEnabled,
                    (unsigned)s.aSpiMhz, (unsigned)s.aSpiTargetMhz,
                    (unsigned)s.bDriverState, (unsigned)s.nagEnabled,
                    (unsigned)s.aLoopGapLastUs, (unsigned)s.aLoopGapPeakUs,
                    (unsigned)s.aLoopGapOver250us, (unsigned)s.aLoopGapOver500us,
                    (unsigned)s.aLoopGapOver1ms,
                    (unsigned)s.aLoopGapOver2ms, (unsigned)s.dALoopGapOver2ms,
                    aCanPhaseName(s.aLastOverrunPhase),
                    (unsigned)s.summonGateOpen, (unsigned)s.summonConditionLimit,
                    (unsigned)s.summonApState, (unsigned)s.summonApActive,
                    (unsigned)s.summonParked, (unsigned)s.summoning,
                    (unsigned)s.summonApStableMs,
                    summonGateReasonNameFromCode(s.summonGateReason),
                    (unsigned)s.summonInjectReady,
                    (unsigned)s.summonTxOk, (unsigned)s.summonTxFail,
                    (unsigned)s.summonBlocked, (unsigned)s.dSummonTxOk,
                    (unsigned)s.dSummonTxFail, (unsigned)s.dSummonBlocked,
                    (unsigned)s.tsllcInjectReady, (unsigned)s.tsllcTxOk,
                    (unsigned)s.tsllcTxFail, (unsigned)s.dTsllcTxOk,
                    (unsigned)s.dTsllcTxFail, (unsigned)s.nagApOnly,
                    (unsigned)s.nagApActive, (unsigned)s.nagInjecting,
                    (unsigned)s.modeBInject, (unsigned)s.dModeBInject);
                if (httpd_resp_sendstr_chunk(req, line) != ESP_OK) {
                    logsBundleSerialTrace("fail section4", handlerStartMs);
                    if (wdtDetached) esp_task_wdt_add(NULL);
                    gWebDownloadBusyUntilMs = 0;
                    return ESP_FAIL;
                }
            }
        }
    }
    logsBundleSerialTrace("section4 done", handlerStartMs);

    // 섹션 5: 밀리초 이벤트 로그는 별도 CSV로 분리한다.
    logsBundleSerialTrace("section5 summary", handlerStartMs);
    httpd_resp_sendstr_chunk(req, "\r\n=== [5] 밀리초 이벤트 로그 ===\r\n");
    {
        size_t evtN = 0;
        size_t evtSnapHead = 0;
        eventLogSnapshot(evtN, evtSnapHead);
        snprintf(line, sizeof(line),
            "채널별 이벤트: records=%u occurrences=%u coalesced=%u overwritten=%u cap=%u csv=/api/events.csv\r\n",
            (unsigned)evtN, (unsigned)evtOccurrenceTotal,
            (unsigned)evtCoalescedTotal, (unsigned)evtOverwrittenTotal,
            (unsigned)EVT_CAP);
        httpd_resp_sendstr_chunk(req, line);
        (void)evtSnapHead;
    }

    logsBundleSerialTrace("final chunk", handlerStartMs);
    esp_err_t finalErr = httpd_resp_sendstr_chunk(req, nullptr);
    gWebDownloadBusyUntilMs = 0;
    webHealthRecordDuration(gWebLogsBundleLastDurMs, gWebLogsBundleMaxDurMs, handlerStartMs);
    logsBundleSerialTrace(finalErr == ESP_OK ? "done ok" : "done fail", handlerStartMs);
    if (wdtDetached) esp_task_wdt_add(NULL);
    return finalErr == ESP_OK ? ESP_OK : ESP_FAIL;
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
    const uint32_t currentHead = diagLog.currentHead();
    const uint32_t oldest = currentHead > LogRingBuffer::kCapacity
                                ? currentHead - LogRingBuffer::kCapacity : 0;
    const uint32_t start = since > oldest ? since : oldest;
    const uint32_t nextHead = start + (uint32_t)n;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "head",  (int)nextHead);
    cJSON_AddNumberToObject(root, "complete_head", (int)currentHead);
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

// GET /api/can-diag/log-dl — 자가 진단 결과 CSV 다운로드
static esp_err_t canDiagLogDlHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/csv; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"can_diag.csv\"");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_sendstr_chunk(req, "\xEF\xBB\xBF");
    httpd_resp_sendstr_chunk(req,
        "schema_version,wall_time,uptime_ms,diag_state,sequence,scope,message\r\n");

    const uint32_t head = diagLog.currentHead();
    const uint32_t start = head > LogRingBuffer::kCapacity ? head - LogRingBuffer::kCapacity : 0;
    const char *stateName =
        diagState == DiagState::RUNNING ? "RUNNING" :
        diagState == DiagState::DONE ? "DONE" : "NOT_RUN";
    char escaped[LogRingBuffer::kMaxMsgLen * 2 + 3];
    char line[LogRingBuffer::kMaxMsgLen * 2 + 160];
    char wallTime[40];
    if (head == 0) {
        const uint32_t now = millis();
        formatLogTimestamp(now, wallTime, sizeof(wallTime));
        csvEscapeCell("자가 진단을 실행한 기록이 없습니다. 먼저 '자가 진단 실행'을 완료한 뒤 저장하세요.",
                      escaped, sizeof(escaped));
        snprintf(line, sizeof(line), "2,%s,%u,NOT_RUN,0,A/B,%s\r\n",
                 wallTime, (unsigned)now, escaped);
        httpd_resp_sendstr_chunk(req, line);
    }
    for (uint32_t i = start; i < head; ++i) {
        const LogRingBuffer::Entry &entry = diagLog.at(i);
        formatLogTimestamp(entry.timestamp_ms, wallTime, sizeof(wallTime));
        csvEscapeCell(entry.msg, escaped, sizeof(escaped));
        snprintf(line, sizeof(line), "2,%s,%u,%s,%u,A/B,%s\r\n",
                 wallTime, (unsigned)entry.timestamp_ms, stateName,
                 (unsigned)(i - start + 1U), escaped);
        if (httpd_resp_sendstr_chunk(req, line) != ESP_OK) return ESP_FAIL;
    }
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}


static void applyEmergencyDisableAllFeatures()
{
    // 현재 상태를 NVS에 백업 (나중에 원복용)
    nvsWriteBool("bk_isa", isaSpeedChimeSuppressRuntime);
    nvsWriteBool("bk_emerg", emergencyVehicleDetectionRuntime);
    nvsWriteBool("bk_summon", summonUnlockRuntime);
    nvsWriteBool("bk_nag", nagKillerRuntime);
    nvsWriteBool("bk_tsllc", tsllcRuntime);
    nvsWriteBool("bk_a_ch_tx", aChannelTxRuntime);

    isaSpeedChimeSuppressRuntime = false;
    emergencyVehicleDetectionRuntime = false;
    summonUnlockRuntime = false;
    nagKillerRuntime = false;
    tsllcRuntime = false;
    aChannelTxRuntime = false;

    nvsWriteBool(kNvsKeyIsaSpeedChime, false);
    nvsWriteBool(kNvsKeyEmergencyVehicleDetection, false);
    nvsWriteBool(kNvsKeySummonUnlock, false);
    nvsWriteBool(kNvsKeyNagKiller, false);
    nvsWriteBool(kNvsKeyTsllc, false);
    nvsWriteBool(kNvsKeyAChTx, false);

}

static void applyEmergencyRestoreAllFeatures()
{
    isaSpeedChimeSuppressRuntime = nvsReadBool("bk_isa", false);
    emergencyVehicleDetectionRuntime = nvsReadBool("bk_emerg", false);
    summonUnlockRuntime = nvsReadBool("bk_summon", false);
    nagKillerRuntime = nvsReadBool("bk_nag", false);
    tsllcRuntime = nvsReadBool("bk_tsllc", false);
    aChannelTxRuntime = nvsReadBool("bk_a_ch_tx", false);

    nvsWriteBool(kNvsKeyIsaSpeedChime, isaSpeedChimeSuppressRuntime);
    nvsWriteBool(kNvsKeyEmergencyVehicleDetection, emergencyVehicleDetectionRuntime);
    nvsWriteBool(kNvsKeySummonUnlock, summonUnlockRuntime);
    nvsWriteBool(kNvsKeyNagKiller, nagKillerRuntime);
    nvsWriteBool(kNvsKeyTsllc, tsllcRuntime);
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
    eventLogPush(EV_FEATURE_STATE,
                 (uint16_t)bChannelDiag.twaiTxErrNow,
                 (uint16_t)bChannelDiag.twaiRxErrNow,
                 eventFeatureStateDetail());
    logRing.push("🚨 [EMERGENCY] 긴급 기능해제 실행: 모든 런타임 기능 OFF", millis());

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
    eventLogPush(EV_FEATURE_STATE,
                 (uint16_t)bChannelDiag.twaiTxErrNow,
                 (uint16_t)bChannelDiag.twaiRxErrNow,
                 eventFeatureStateDetail());
    logRing.push("✅ [RESTORE] 기능 원복 실행: 이전 설정 복원", millis());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"message\":\"emergency_restored\"}", HTTPD_RESP_USE_STRLEN);
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

static void formatEpochMsKst(int64_t epochMs, char *out, size_t outLen)
{
    if (!out || outLen == 0) return;
    time_t sec = (time_t)(epochMs / 1000);
    int ms = (int)(epochMs % 1000);
    struct tm tm_v;
    localtime_r(&sec, &tm_v);
    snprintf(out, outLen, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm_v.tm_year + 1900, tm_v.tm_mon + 1, tm_v.tm_mday,
             tm_v.tm_hour, tm_v.tm_min, tm_v.tm_sec, ms);
}

static void formatOtaUploadTimestamp(const char *epochMsText, char *out, size_t outLen)
{
    if (!out || outLen == 0) return;
    out[0] = '\0';
    if (epochMsText && epochMsText[0]) {
        char *end = nullptr;
        int64_t epochMs = strtoll(epochMsText, &end, 10);
        if (end && *end == '\0' && epochMs > 0) {
            formatEpochMsKst(epochMs, out, outLen);
            return;
        }
    }
    formatLogTimestamp(millis(), out, outLen);
}



static esp_err_t otaHandler(httpd_req_t *req)
{
    if (req->content_len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing firmware payload");
        return ESP_FAIL;
    }

    const esp_partition_t *updatePart = esp_ota_get_next_update_partition(NULL);
    char uploadEpochMs[32] = {};
    httpd_req_get_hdr_value_str(req, "X-Upload-Epoch-Ms", uploadEpochMs, sizeof(uploadEpochMs));
    char uploadAt[40] = {};
    formatOtaUploadTimestamp(uploadEpochMs, uploadAt, sizeof(uploadAt));

    char contentType[64] = {};
    if (httpd_req_get_hdr_value_str(req, "Content-Type", contentType, sizeof(contentType)) == ESP_OK &&
        strstr(contentType, "multipart/form-data") != nullptr)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Upload raw firmware .bin, not multipart/form-data");
        return ESP_FAIL;
    }

    if (!prepareOtaUploadCanQuiet())
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to disable CAN TX before OTA");
        return ESP_FAIL;
    }

    if (!Update.begin(req->content_len))
    {
        logRing.push("[OTA] 시작 실패: CAN TX는 재부팅 전까지 차단 유지", millis());
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "OTA start failed; CAN TX remains disabled until reboot");
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
                logRing.push("[OTA] 수신 시간 초과: CAN TX는 재부팅 전까지 차단 유지", millis());
                httpd_resp_set_status(req, "408 Request Timeout");
                httpd_resp_send(req, "Upload timed out; reboot required", HTTPD_RESP_USE_STRLEN);
                return ESP_FAIL;
            }
            continue;
        }
        if (received <= 0)
        {
            Update.abort();
            logRing.push("[OTA] 수신 실패: CAN TX는 재부팅 전까지 차단 유지", millis());
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "Upload failed; CAN TX remains disabled until reboot");
            return ESP_FAIL;
        }
        timeoutCount = 0;
        if (Update.write(buffer, received) != (size_t)received)
        {
            Update.abort();
            logRing.push("[OTA] 기록 실패: CAN TX는 재부팅 전까지 차단 유지", millis());
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "Flash write failed; CAN TX remains disabled until reboot");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    if (!Update.end(true))
    {
        logRing.push("[OTA] 검증 실패: CAN TX는 재부팅 전까지 차단 유지", millis());
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "OTA validation failed; CAN TX remains disabled until reboot");
        return ESP_FAIL;
    }

    // ── OTA 롤백 정보 NVS 저장 ──────────────────────────────────────────────
    // 재부팅 전: 현재(구) 파티션 레이블 저장 + ota_pending=1 기록
    // 새 펌웨어 첫 부팅 시 setup() 초입에서 2로 변경, 정상 부팅 완료 시 0으로 클리어
    // 만약 새 펌웨어가 크래시로 부팅 완료를 못하면 다음 부팅 시 ota_pending==2를 발견 → 롤백
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_err_t metadataErr = (!running || !updatePart) ? ESP_ERR_NOT_FOUND : ESP_OK;
    if (metadataErr == ESP_OK) {
        nvs_handle_t wh;
        metadataErr = nvs_open(kNvsNamespace, NVS_READWRITE, &wh);
        if (metadataErr == ESP_OK) {
            metadataErr = nvs_set_u8(wh, kNvsKeyOtaPending, 1);
            if (metadataErr == ESP_OK) metadataErr = nvs_set_str(wh, kNvsKeyOtaFallback, running->label);
            if (metadataErr == ESP_OK) metadataErr = nvs_set_str(wh, kNvsKeyOtaExpectPart, updatePart->label);
            if (metadataErr == ESP_OK) metadataErr = nvs_set_str(wh, kNvsKeyOtaUploadAt, uploadAt);
            if (metadataErr == ESP_OK) metadataErr = otaWriteFallbackFirmwareMeta(wh);
            if (metadataErr == ESP_OK) metadataErr = nvs_commit(wh);
            nvs_close(wh);
        }
    }

    if (metadataErr != ESP_OK) {
        esp_err_t restoreErr = running ? esp_ota_set_boot_partition(running) : ESP_ERR_NOT_FOUND;
        if (restoreErr != ESP_OK) {
            Serial.printf("[OTA] metadata 실패 후 현재 boot 파티션 복원 실패 (%ld)\n",
                          static_cast<long>(restoreErr));
        }
        enterCanBootFailClosed("OTA_METADATA_SAVE_FAILED", metadataErr);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "OTA metadata save failed; current firmware kept");
        return ESP_FAIL;
    }

    char pendingBuf[80];
    snprintf(pendingBuf, sizeof(pendingBuf), "[OTA] 롤백 대기 설정: fallback=%s expect=%s",
             running->label, updatePart->label);
    Serial.println(pendingBuf);
    logRing.push(pendingBuf, millis());
    gOtaBootPendingState = 1;

    Serial.println("Web: OTA upload complete, restarting");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

// ─── POST /api/ota-confirm ───────────────────────────────────────────────────
// 신 펌웨어 정상 동작 확인 → pending=0. 표시용 이전 펌웨어 메타는 보존.
static esp_err_t otaStorePendingState(uint8_t pending, bool clearExpectedPart)
{
    nvs_handle_t nh;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &nh);
    if (err != ESP_OK) return err;
    err = nvs_set_u8(nh, kNvsKeyOtaPending, pending);
    if (err == ESP_OK && clearExpectedPart) err = nvs_set_str(nh, kNvsKeyOtaExpectPart, "");
    if (err == ESP_OK) err = nvs_commit(nh);
    nvs_close(nh);
    if (err == ESP_OK) gOtaBootPendingState = pending;
    return err;
}

static esp_err_t otaConfirmHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    esp_err_t err = otaStorePendingState(0, true);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to confirm OTA state");
        return ESP_FAIL;
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
    nvs_handle_t nh = 0;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &nh);
    if (err == ESP_OK) {
        size_t sz = sizeof(fallback);
        err = nvs_get_str(nh, kNvsKeyOtaFallback, fallback, &sz);
    }
    const esp_partition_t *prev = fallback[0] ? esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, fallback) : nullptr;
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (err == ESP_OK && (!prev || !running)) err = ESP_ERR_NOT_FOUND;
    if (err == ESP_OK) err = writeOtaSafeFeatureSettings(nh);
    if (err == ESP_OK) {
        esp_err_t clearErr = twai_clear_transmit_queue();
        if (clearErr != ESP_OK && clearErr != ESP_ERR_INVALID_STATE) err = clearErr;
    }
    if (err == ESP_OK) err = esp_ota_set_boot_partition(prev);
    if (err == ESP_OK) err = nvs_set_u8(nh, kNvsKeyOtaPending, 3);
    if (err == ESP_OK) err = nvs_commit(nh);
    if (nh) nvs_close(nh);
    if (err != ESP_OK) {
        if (running) esp_ota_set_boot_partition(running);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to prepare OTA rollback");
        return ESP_FAIL;
    }
    gOtaBootPendingState = 3;
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
// 복구 파티션 정상 확인 → pending=0, 복구모드 해제. 표시용 이전 펌웨어 메타는 보존.
static esp_err_t otaRecoveryConfirmHandler(httpd_req_t *req)
{
    if (!rateLimitOk()) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    esp_err_t err = otaStorePendingState(0, true);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to confirm recovery state");
        return ESP_FAIL;
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
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &nh);
    if (err == ESP_OK) {
        err = writeOtaSafeFeatureSettings(nh);
        if (err == ESP_OK) {
            esp_err_t clearErr = twai_clear_transmit_queue();
            if (clearErr != ESP_OK && clearErr != ESP_ERR_INVALID_STATE) err = clearErr;
        }
        if (err == ESP_OK) err = nvs_set_u8(nh, kNvsKeyOtaPending, 5);
        if (err == ESP_OK) err = nvs_commit(nh);
        nvs_close(nh);
    }
    if (err != ESP_OK) {
        enterCanBootFailClosed("OTA_ENTER_RECOVERY_SAVE_FAILED", err);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to enter recovery mode");
        return ESP_FAIL;
    }
    gOtaBootPendingState = 5;
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
            esp_err_t err = ESP_OK;
            nvs_handle_t nh = 0;
            err = nvs_open(kNvsNamespace, NVS_READWRITE, &nh);
            if (err == ESP_OK) {
                size_t sz = sizeof(fallback);
                err = nvs_get_str(nh, kNvsKeyOtaFallback, fallback, &sz);
            }
            const esp_partition_t *prev = fallback[0] ? esp_partition_find_first(
                ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, fallback) : nullptr;
            const esp_partition_t *running = esp_ota_get_running_partition();
            if (err == ESP_OK && (!prev || !running)) err = ESP_ERR_NOT_FOUND;
            if (err == ESP_OK) err = writeOtaSafeFeatureSettings(nh);
            if (err == ESP_OK) {
                esp_err_t clearErr = twai_clear_transmit_queue();
                if (clearErr != ESP_OK && clearErr != ESP_ERR_INVALID_STATE) err = clearErr;
            }
            if (err == ESP_OK) err = esp_ota_set_boot_partition(prev);
            if (err == ESP_OK) err = nvs_set_u8(nh, kNvsKeyOtaPending, 3);
            if (err == ESP_OK) err = nvs_commit(nh);
            if (nh) nvs_close(nh);
            if (err != ESP_OK) {
                if (running) esp_ota_set_boot_partition(running);
                enterCanBootFailClosed("OTA_WATCHDOG_ROLLBACK_FAILED", err);
                esp_err_t clearErr = twai_clear_transmit_queue();
                (void)clearErr;
                vTaskDelete(NULL);
                return;
            }
            gOtaBootPendingState = 3;
            logRing.push("[OTA] 확인 창 만료 → 자동 롤백 재부팅", millis());
            Serial.println("[OTA] 확인 창 만료 → 자동 롤백 재부팅");
            vTaskDelay(pdMS_TO_TICKS(200));
            esp_restart();
        }

        if (gOtaRollbackDeadlineMs > 0 && now >= gOtaRollbackDeadlineMs) {
            // 복구 확인 창 만료 → 복구모드 진입
            gOtaRollbackDeadlineMs = 0;
            nvs_handle_t nh;
            esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &nh);
            if (err == ESP_OK) {
                err = writeOtaSafeFeatureSettings(nh);
                if (err == ESP_OK) {
                    esp_err_t clearErr = twai_clear_transmit_queue();
                    if (clearErr != ESP_OK && clearErr != ESP_ERR_INVALID_STATE) err = clearErr;
                }
                if (err == ESP_OK) err = nvs_set_u8(nh, kNvsKeyOtaPending, 5);
                if (err == ESP_OK) err = nvs_commit(nh);
                nvs_close(nh);
            }
            enterCanBootFailClosed(err == ESP_OK ? "OTA_RECOVERY_TIMEOUT" : "OTA_RECOVERY_TIMEOUT_SAVE_FAILED", err);
            if (err == ESP_OK) gOtaBootPendingState = 5;
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
    tsTimeFormatter = formatLogTimestamp;
    eventTimeFormatter = formatLogTimestamp;
    // 차량 영향 설정은 CAN 시작 전에 선로드된다. 여기서는 UI 전용 설정만 읽는다.
    if (nvsInit())
    {
        nvsReadStr(kNvsKeyTheme, themeRuntime, sizeof(themeRuntime), "dark");
    }
    else
    {
        Serial.println("NVS: init failed, using defaults");
    }
    eventLogPush(EV_FEATURE_STATE, 0, 0, eventFeatureStateDetail());

    // OTA 상태 머신: pending 값을 읽어 타이머 데드라인 설정 + 워치독 시작
    {
        uint8_t otaPending = gOtaBootPendingState;
        if (!gOtaRecoveryModeActive && otaPending == 2) {
            gOtaConfirmDeadlineMs = millis() + kOtaConfirmWindowMs;
            Serial.printf("[OTA] 신 FW 확인 창 시작: %lu ms 남음\n", (unsigned long)kOtaConfirmWindowMs);
            xTaskCreatePinnedToCore(otaWatchdogTask, "ota_wd", 3072, NULL, 2, NULL, 0);
        } else if (!gOtaRecoveryModeActive && otaPending == 4) {
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

    // DNS captive portal on Core 0
    xTaskCreatePinnedToCore(dnsTask, "dns", 4096, NULL, 2, NULL, 0);

    // HTTP server on Core 0
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0;
    config.max_uri_handlers = 65;
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
    httpd_uri_t uriSummonUnlock = {
        .uri = "/api/summon-unlock", .method = HTTP_POST, .handler = summonUnlockHandler, .user_ctx = NULL};
    httpd_uri_t uriSummonConditionLimit = {
        .uri = "/api/summon-condition-limit", .method = HTTP_POST, .handler = summonConditionLimitHandler, .user_ctx = NULL};
    httpd_uri_t uriNagKiller = {
        .uri = "/api/nag-killer", .method = HTTP_POST, .handler = nagKillerHandler, .user_ctx = NULL};
    httpd_uri_t uriNagApOnly = {
        .uri = "/api/nag-ap-only", .method = HTTP_POST, .handler = nagApOnlyHandler, .user_ctx = NULL};
    httpd_uri_t uriTsllc = {
        .uri = "/api/tsllc", .method = HTTP_POST, .handler = tsllcHandler, .user_ctx = NULL};
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
    httpd_uri_t uriSetTheme = {
        .uri = "/api/set-theme", .method = HTTP_POST, .handler = setThemeHandler, .user_ctx = NULL};
    httpd_uri_t uriOta = {
        .uri = "/api/ota", .method = HTTP_POST, .handler = otaHandler, .user_ctx = NULL};
    httpd_uri_t uriReboot = {
        .uri = "/api/reboot", .method = HTTP_POST, .handler = rebootHandler, .user_ctx = NULL};
    httpd_uri_t uriNvsReset = {
        .uri = "/api/nvs-reset", .method = HTTP_POST, .handler = nvsResetHandler, .user_ctx = NULL};
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
    httpd_uri_t uriNagUpdate = {
        .uri = "/api/nag-update", .method = HTTP_POST, .handler = nagUpdateHandler, .user_ctx = NULL};
    httpd_uri_t uriNagReset = {
        .uri = "/api/nag-reset", .method = HTTP_POST, .handler = nagResetHandler, .user_ctx = NULL};
    // CAN 자가 진단 API
    httpd_uri_t uriCanDiagStart = {
        .uri = "/api/can-diag/start", .method = HTTP_POST, .handler = canDiagStartHandler, .user_ctx = NULL};
    httpd_uri_t uriCanDiagLog = {
        .uri = "/api/can-diag/log", .method = HTTP_GET, .handler = canDiagLogHandler, .user_ctx = NULL};
    httpd_uri_t uriCanDiagLogDl = {
        .uri = "/api/can-diag/log-dl", .method = HTTP_GET, .handler = canDiagLogDlHandler, .user_ctx = NULL};
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

    httpd_register_uri_handler(webServer, &uriRoot);
    httpd_register_uri_handler(webServer, &uriStatus);
    httpd_register_uri_handler(webServer, &uriIsaSpeedChime);
    httpd_register_uri_handler(webServer, &uriEmergencyVehicleDetection);
    httpd_register_uri_handler(webServer, &uriSummonUnlock);
    httpd_register_uri_handler(webServer, &uriSummonConditionLimit);
    httpd_register_uri_handler(webServer, &uriNagKiller);
    httpd_register_uri_handler(webServer, &uriNagApOnly);
    httpd_register_uri_handler(webServer, &uriTsllc);
    httpd_register_uri_handler(webServer, &uriAChannelTx);
    httpd_register_uri_handler(webServer, &uriASpi8Mhz);
    httpd_register_uri_handler(webServer, &uriAOneShot);
    httpd_register_uri_handler(webServer, &uriATxGuard);
    httpd_register_uri_handler(webServer, &uriEmergencyDisable);
    httpd_register_uri_handler(webServer, &uriEmergencyRestore);
    httpd_register_uri_handler(webServer, &uriSetTheme);
    httpd_register_uri_handler(webServer, &uriOta);
    httpd_register_uri_handler(webServer, &uriReboot);
    httpd_register_uri_handler(webServer, &uriNvsReset);
    httpd_register_uri_handler(webServer, &uriGenerate204);
    httpd_register_uri_handler(webServer, &uriHotspot);
    httpd_register_uri_handler(webServer, &uriNagConfigGet);
    httpd_register_uri_handler(webServer, &uriNagStatsGet);
    httpd_register_uri_handler(webServer, &uriNagMode);
    httpd_register_uri_handler(webServer, &uriNagUpdate);
    httpd_register_uri_handler(webServer, &uriNagReset);
    httpd_register_uri_handler(webServer, &uriCanDiagStart);
    httpd_register_uri_handler(webServer, &uriCanDiagLog);
    httpd_register_uri_handler(webServer, &uriCanDiagLogDl);
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
