// Web UI 문자열 회귀를 검증하는 native 테스트
#include <cstring>

#include <unity.h>

#include "web/web_ui.h"

namespace {

bool contains(const char *haystack, const char *needle)
{
    return std::strstr(haystack, needle) != nullptr;
}

} // namespace

void setUp() {}
void tearDown() {}

void test_recovery_upload_sets_epoch_header_and_triggers_auto_reload()
{
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "xhr.setRequestHeader('X-Upload-Epoch-Ms',String(Date.now()));"));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "scheduleRecoveryOtaReload(st);"));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "fetch('/api/status?ota_reload='+Date.now(),{cache:'no-store'})"));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "location.replace('/?ota_reload='+Date.now());"));
}

void test_recovery_status_card_renders_upload_time_and_fallback_firmware()
{
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "id=\"recUploadAt\""));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "id=\"recFallbackFw\""));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "setRecStat('recUploadAt',d.ota_upload_at||'--'"));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML,
                              "setRecStat('recFallbackFw',recFwBrief(d.ota_fallback_version,d.ota_fallback_build_id,d.ota_fallback_build_at)"));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "setRecStat('recFallbackBuilt',recShortDateTime(d.ota_fallback_build_at)"));
}

void test_main_ui_logs_bundle_pauses_background_polling_during_download()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "onclick=\"return handleDownloadAction(event,this)\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "function handleDownloadAction(ev,el){"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "var LOGS_BUNDLE_PAUSE_MS=180000;"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "var EVENTS_BUNDLE_PAUSE_MS=90000;"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "pauseBackgroundNetwork();"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "scheduleLogsBundleNetworkResume(pauseMs);"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "if(isBackgroundNetworkPaused())return;"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "if(_otaUploadInProgress||isBackgroundNetworkPaused())return;"));
}

void test_main_ui_download_uses_direct_attachment_not_blob_buffer()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "function handleDownloadAction(ev,el){"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "el.setAttribute('href',appendQueryParam(base,'dl',Date.now()));"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "downloadFileViaBlob"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "new Blob([blob]"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "navigator.share"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "inlineUrl"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "data-download-url=\"/api/events-bundle\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "이벤트 묶음 저장"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "관찰 이벤트 저장"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "밀리초 이벤트 저장"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, ">관찰기 저장</a>"));
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_recovery_upload_sets_epoch_header_and_triggers_auto_reload);
    RUN_TEST(test_recovery_status_card_renders_upload_time_and_fallback_firmware);
    RUN_TEST(test_main_ui_logs_bundle_pauses_background_polling_during_download);
    RUN_TEST(test_main_ui_download_uses_direct_attachment_not_blob_buffer);

    return UNITY_END();
}
