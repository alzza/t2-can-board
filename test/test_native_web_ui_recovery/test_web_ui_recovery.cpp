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
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "_recOtaReloadTimer=null;"));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "fetch('/api/status?ota_reload='+Date.now(),{cache:'no-store'})"));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "return r.json();"));
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
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "id=\"recBootBlock\""));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "d.can_boot_block_reason||'--'"));
    TEST_ASSERT_TRUE(contains(WEB_RECOVERY_UI_HTML, "d.can_boot_allowed?'v-ok':'v-err'"));
}

void test_main_ui_logs_bundle_pauses_background_polling_during_download()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "onclick=\"return handleDownloadAction(event,this)\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "function handleDownloadAction(ev,el){"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "var LOGS_BUNDLE_PAUSE_MS=15000;"));
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
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"downloadStatus\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/events-bundle"));
}

void test_main_ui_ota_reconnect_reloads_page_and_uses_only_firmware_banner()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "function scheduleOtaReload(st){"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "_otaReloadTimer=null;"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "return r.json();"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "_otaUploadInProgress=false;"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "location.replace('/?ota_reload='+Date.now());"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "if((state===2||state===4)&&typeof poll==='function'){poll();return;}"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "confirm('이 펌웨어를 계속 사용합니다."));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "confirm('이전 펌웨어로 되돌립니다."));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "confirm('복구가 완료되었음을 확인합니다."));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "confirm('OTA 복구모드로 전환합니다."));
}

void test_main_ui_keeps_can_nag_tsllc_and_summon_status_together()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"s-main-a\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"s-main-b\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"s-main-busoff\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"s-main-summon\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "Nag Killer"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "EAP / EU Unlock / Summon (HW3)"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "<span class=\"label\">TSLLC</span>"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "TSLLC / EAP"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "/api/summon-unlock"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "A 채널 (MCP2515 · Summon/TSLLC)"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "B 채널 (TWAI · Nag Killer)"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "fetch('/api/nag-stats')"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "var ch=d.channels||{},a=ch.a_channel||{},b=ch.b_channel||{};"));
}

void test_main_ui_uses_ino_summon_control_and_monitoring_fields()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "d.summon_unlock_enabled"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "var su=d.summon_unlock||{};"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"suActive\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "!su.tx_master?'TX OFF'"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"suCanState\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"aSummonGearRx\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"aSummonGateRx\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"aSummonMuxBlocked\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"aSummonTx\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"tAChannelTx\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/enhanced-autopilot"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "enhanced_autopilot"));
}

void test_main_ui_removes_retired_can_injection_experiments()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, ">상태</button>"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, ">기록</button>"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"view-records\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "var views={main:1,control:1,diag:1,records:1,ota:1};"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "experiment:1"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "신호 관찰기"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/signal-observer"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/ui-ulc-"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/ui-alc-"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/ui-auto-turn-signal-mode"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/das-ulc-confirmation-request"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "A채널 실험"));
}

void test_main_ui_uses_verified_nag_modes_and_removes_smart_profiles()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "name=\"nagMode\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "MODE 1"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "MODE 2"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "MODE 3"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "MODE 2 · AP 전용 · 권장"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"tNagApOnly\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "id=\"tSummonCondition\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "안전 송신 게이트"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"suEap\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"suBit46\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "/api/nag-ap-only"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "/api/nag-mode?m="));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/nag-profile"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "name=\"nagProfile\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, ">A안<"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, ">D안<"));
}

void test_status_view_removes_duplicate_switches_and_keeps_control_switches()
{
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "id=\"mSummon\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "id=\"mTsllc\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "id=\"mNag\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"tSummon\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"tTsllc\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"tNag\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "EAP / EU Unlock / Summon (HW3)"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "<span class=\"label\">TSLLC</span>"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "TSLLC / EAP"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "Policy '+(su.policy_mode||'ACA_SPR_1315')"));
}

void test_main_ui_exposes_authoritative_channel_health_and_csv_diagnostics()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "a.health_state"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "b.health_state"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"d-a-eflg-now\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"d-a-wake\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"suWakeDelay\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "/api/can-diag/log-dl"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "/api/timeseries.csv"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "/api/events.csv"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "id=\"tSsTx\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "id=\"tBoStop\""));
}

void test_main_ui_distinguishes_bus_error_from_bus_off()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, ">OFF / ERR<"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, ">BUS-OFF / BUS-ERR<"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "BUS-ERR는 CAN 프로토콜 오류 누적이며 BUS-OFF 진입 횟수가 아닙니다."));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "실제 TWAI BUS-OFF 진입과 복구 결과만 기록합니다."));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "bBusOffCount+' / '+bBusErrCount"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "'BUS-OFF '+bBusOffCount+' · ERR '+bBusErrCount"));
}

void test_main_ui_defaults_to_summary_and_important_live_logs()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "<details class=\"card diag-details\" id=\"diagDetails\">"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "채널별 상세 카운터 · BUS-OFF 이력"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "var liveLogMode='important',liveLogEntries=[];"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "function isImportantLiveLog(msg)"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"logModeImportant\" class=\"active\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"logModeAll\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "전체 저장 파일에는 모든 런타임 로그가 유지됩니다."));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "id=\"tLog\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "/api/enable-print"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "enable_print"));
}

void test_main_ui_uses_generic_paired_user_marker()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "class=\"header-info-bar\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"userMarkBtn\""));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "id=\"markBtn\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"hw-badge\">HW3</span>"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"ver-badge\">v--</span>"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "id=\"build-badge\">빌드 확인 중</span>"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "onclick=\"toggleUserMarker()\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "async function toggleUserMarker(){"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "fetch('/api/user-marker',{method:'POST'})"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "btn.textContent='USER_MARK';"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "USER_MARK_START"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "USER_MARK_END"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "d.user_marker_count!==undefined"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "AP_WARNING"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "markApWarning"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML,
                               "updateUserMarkerUi({active:false,log_count:0,count:0})"));
}

void test_main_ui_has_readable_iphone_safari_layout()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "<html lang=\"ko\""));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML,
                              "width=device-width,initial-scale=1,viewport-fit=cover"));
    TEST_ASSERT_FALSE(contains(WEB_UI_HTML, "user-scalable=no"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "-webkit-text-size-adjust:100%"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "--fs-xs:12px;--fs-sm:13px"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "min-height:44px"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "env(safe-area-inset-top)"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "@media(max-width:430px)"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, ".main-health-grid{grid-template-columns:1fr}"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML,
                              "/* ── iPhone/Safari: PC와 동일한 시각 체계, 모바일에 맞는 한 열 배치 ── */"));
}

void test_main_ui_exposes_tsllc_rearm_and_gate_states()
{
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "TSLLC 재승인"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "REARM_REQUIRED"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "STARTUP_FRAMES"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "A 안전 잠금"));
    TEST_ASSERT_TRUE(contains(WEB_UI_HTML, "fetch('/api/tsllc'"));
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_recovery_upload_sets_epoch_header_and_triggers_auto_reload);
    RUN_TEST(test_recovery_status_card_renders_upload_time_and_fallback_firmware);
    RUN_TEST(test_main_ui_logs_bundle_pauses_background_polling_during_download);
    RUN_TEST(test_main_ui_download_uses_direct_attachment_not_blob_buffer);
    RUN_TEST(test_main_ui_ota_reconnect_reloads_page_and_uses_only_firmware_banner);
    RUN_TEST(test_main_ui_keeps_can_nag_tsllc_and_summon_status_together);
    RUN_TEST(test_main_ui_uses_ino_summon_control_and_monitoring_fields);
    RUN_TEST(test_main_ui_removes_retired_can_injection_experiments);
    RUN_TEST(test_main_ui_uses_verified_nag_modes_and_removes_smart_profiles);
    RUN_TEST(test_status_view_removes_duplicate_switches_and_keeps_control_switches);
    RUN_TEST(test_main_ui_exposes_authoritative_channel_health_and_csv_diagnostics);
    RUN_TEST(test_main_ui_distinguishes_bus_error_from_bus_off);
    RUN_TEST(test_main_ui_defaults_to_summary_and_important_live_logs);
    RUN_TEST(test_main_ui_uses_generic_paired_user_marker);
    RUN_TEST(test_main_ui_has_readable_iphone_safari_layout);
    RUN_TEST(test_main_ui_exposes_tsllc_rearm_and_gate_states);

    return UNITY_END();
}
