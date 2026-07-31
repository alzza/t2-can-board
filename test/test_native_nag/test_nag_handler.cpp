// HW3 Nag Mode 1/2와 공통 송신 안전 경계를 검증한다.
#include <unity.h>

#include "can_frame_types.h"
#include "can_helpers.h"
#include "drivers/mock_driver.h"
#include "handlers.h"

static MockDriver mock;
static NagHandler handler;

static CanFrame makeEpasFrame(uint8_t handsOn, float torqueNm, uint8_t counter,
                              uint8_t eacStatus = 2)
{
    CanFrame frame = {.id = 880, .dlc = 8};
    frame.data[0] = 0x12;
    frame.data[1] = 0x00;
    uint16_t raw = static_cast<uint16_t>((torqueNm + 20.5f) / 0.01f);
    frame.data[2] = static_cast<uint8_t>(0x08 | ((raw >> 8) & 0x0F));
    frame.data[3] = static_cast<uint8_t>(raw & 0xFF);
    frame.data[4] = static_cast<uint8_t>(((handsOn & 0x03) << 6) | 0x1F);
    frame.data[5] = 0x89;
    frame.data[6] = static_cast<uint8_t>((eacStatus << 5) | (counter & 0x0F));
    uint16_t sum = 0;
    for (int i = 0; i < 7; i++) sum += frame.data[i];
    frame.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);
    return frame;
}

static CanFrame makeDasFrame(uint8_t handsOnState, uint32_t id = 921,
                             uint8_t apState = 6)
{
    CanFrame frame = {.id = id, .dlc = 8};
    frame.data[0] = apState & 0x0F;
    frame.data[5] = static_cast<uint8_t>((handsOnState & 0x0F) << 2);
    return frame;
}

static uint16_t torqueRaw(const CanFrame &frame)
{
    return static_cast<uint16_t>(((frame.data[2] & 0x0F) << 8) | frame.data[3]);
}

static uint8_t handsOn(const CanFrame &frame)
{
    return static_cast<uint8_t>((frame.data[4] >> 6) & 0x03);
}

static bool checksumValid(const CanFrame &frame)
{
    uint16_t sum = 0;
    for (int i = 0; i < 7; i++) sum += frame.data[i];
    return frame.data[7] == static_cast<uint8_t>((sum + 0x73) & 0xFF);
}

static void setMode(uint8_t mode)
{
    nagConfig.mode = mode;
}

void setUp()
{
    mock.reset();
    bChannelDiag = BChannelDiagnostics();
    nagCfgDefaults(nagConfig);
    nagKillerRuntime = true;
    nagApOnlyRuntime = false;
    handler = NagHandler();
}

void tearDown() {}

void test_nag_defaults_off_with_mode2_ap_only_selected()
{
    NagConfig defaults;
    nagCfgDefaults(defaults);
    TEST_ASSERT_FALSE(kNagKillerDefaultEnabled);
    TEST_ASSERT_TRUE(kNagApOnlyDefaultEnabled);
    TEST_ASSERT_EQUAL_UINT8(kNagMode2, defaults.mode);
    TEST_ASSERT_EQUAL_STRING("MODE 2", nagModeName(defaults.mode));
}

void test_a_tx_guard_uses_hard_error_only()
{
    TEST_ASSERT_EQUAL_UINT8(1, kATxGuardTxFailBurstThreshold);
}

void test_nag_mode_clamp_accepts_1_and_2_and_defaults_retired_3_to_2()
{
    TEST_ASSERT_EQUAL_UINT8(kNagMode1, nagModeClamp(kNagMode1));
    TEST_ASSERT_EQUAL_UINT8(kNagMode2, nagModeClamp(kNagMode2));
    TEST_ASSERT_EQUAL_UINT8(kNagMode2, nagModeClamp(3));
    TEST_ASSERT_EQUAL_UINT8(kNagMode2, nagModeClamp(0));
    TEST_ASSERT_EQUAL_UINT8(kNagMode2, nagModeClamp(99));
}

void test_nag_filter_contains_only_required_ids()
{
    const uint32_t *ids = handler.filterIds();
    TEST_ASSERT_EQUAL_UINT8(3, handler.filterIdCount());
    TEST_ASSERT_EQUAL_UINT32(880, ids[0]);
    TEST_ASSERT_EQUAL_UINT32(921, ids[1]);
    TEST_ASSERT_EQUAL_UINT32(923, ids[2]);
}

void test_mode1_fixed_echo_matches_verified_frame_rules()
{
    setMode(kNagMode1);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x0C);
    handler.handleMessageAt(frame, mock, 100);

    TEST_ASSERT_EQUAL(1, mock.sent.size());
    const CanFrame &echo = mock.sent[0];
    TEST_ASSERT_EQUAL_UINT32(880, echo.id);
    TEST_ASSERT_EQUAL_UINT8(8, echo.dlc);
    TEST_ASSERT_EQUAL_HEX16(kNagTorqueRawMax, torqueRaw(echo));
    TEST_ASSERT_EQUAL_UINT8(1, handsOn(echo));
    TEST_ASSERT_EQUAL_HEX8(0x0D, echo.data[6] & 0x0F);
    TEST_ASSERT_TRUE(checksumValid(echo));
}

void test_mode1_does_not_require_das_or_steering_context()
{
    setMode(kNagMode1);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 5000);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
}

void test_mode1_blocks_real_hands_on()
{
    setMode(kNagMode1);
    CanFrame frame = makeEpasFrame(1, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 100);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionHandsOn, (uint8_t)bChannelDiag.nagLastDecision);
}

void test_mode2_cycles_four_torques_then_pauses()
{
    setMode(kNagMode2);
    constexpr uint16_t expected[] = {0x08B6, 0x0898, 0x076C, 0x074E};
    constexpr uint32_t times[] = {1000, 1200, 1400, 1600};
    for (uint8_t i = 0; i < 4; i++) {
        CanFrame frame = makeEpasFrame(0, 0.33f, i);
        handler.handleMessageAt(frame, mock, times[i]);
        TEST_ASSERT_EQUAL(i + 1, mock.sent.size());
        TEST_ASSERT_EQUAL_HEX16(expected[i], torqueRaw(mock.sent[i]));
        TEST_ASSERT_EQUAL_UINT8(1, handsOn(mock.sent[i]));
    }

    CanFrame paused = makeEpasFrame(0, 0.33f, 0x05);
    handler.handleMessageAt(paused, mock, 2000);
    TEST_ASSERT_EQUAL(4, mock.sent.size());

    CanFrame resumed = makeEpasFrame(0, 0.33f, 0x06);
    handler.handleMessageAt(resumed, mock, 3500);
    TEST_ASSERT_EQUAL(5, mock.sent.size());
}

void test_mode2_does_not_require_das_or_steering_context()
{
    setMode(kNagMode2);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 100);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
}

void test_mode2_ap_only_blocks_without_fresh_ap_context()
{
    setMode(kNagMode2);
    nagApOnlyRuntime = true;
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 100);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionNo921,
                            (uint8_t)bChannelDiag.nagLastDecision);
}

void test_mode2_ap_only_blocks_general_drive_and_allows_active_ap()
{
    setMode(kNagMode2);
    nagApOnlyRuntime = true;

    CanFrame driveDas = makeDasFrame(1, 923, 2);
    handler.handleMessageAt(driveDas, mock, 100);
    CanFrame blocked = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(blocked, mock, 100);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionApBlocked,
                            (uint8_t)bChannelDiag.nagLastDecision);

    CanFrame apDas = makeDasFrame(1, 923, 3);
    handler.handleMessageAt(apDas, mock, 200);
    CanFrame allowed = makeEpasFrame(0, 0.33f, 0x02);
    handler.handleMessageAt(allowed, mock, 200);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
}

void test_mode1_original_scope_ignores_ap_when_ap_only_is_off()
{
    setMode(kNagMode1);
    nagApOnlyRuntime = false;
    CanFrame driveDas = makeDasFrame(1, 923, 1);
    handler.handleMessageAt(driveDas, mock, 100);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 100);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
}

void test_all_modes_block_when_driver_hands_are_detected()
{
    for (uint8_t mode = kNagMode1; mode <= kNagMode2; ++mode) {
        setUp();
        setMode(mode);
        CanFrame das = makeDasFrame(2, 923, 3);
        handler.handleMessageAt(das, mock, 100);
        CanFrame frame = makeEpasFrame(1, 0.33f, 0x01);
        handler.handleMessageAt(frame, mock, 100);
        TEST_ASSERT_EQUAL(0, mock.sent.size());
        TEST_ASSERT_EQUAL_UINT8(kNagDecisionHandsOn,
                                (uint8_t)bChannelDiag.nagLastDecision);
    }
}

void test_all_modes_stay_inside_common_torque_cap()
{
    for (uint8_t mode = kNagMode1; mode <= kNagMode2; mode++) {
        setUp();
        setMode(mode);
        CanFrame frame = makeEpasFrame(0, 0.33f, mode);
        handler.handleMessageAt(frame, mock, 100);
        TEST_ASSERT_EQUAL(1, mock.sent.size());
        TEST_ASSERT_TRUE(torqueRaw(mock.sent[0]) >= kNagTorqueRawMin);
        TEST_ASSERT_TRUE(torqueRaw(mock.sent[0]) <= kNagTorqueRawMax);
    }
}

void test_failed_send_does_not_increment_success_counters()
{
    setMode(kNagMode1);
    mock.sendSucceeds = false;
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 100);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0, handler.framesSent);
    TEST_ASSERT_EQUAL_UINT32(0, handler.nagEchoCount);
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)bChannelDiag.echoCount);
}

void test_successful_echo_is_not_reprocessed()
{
    setMode(kNagMode1);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 100);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    CanFrame ownEcho = mock.sent[0];

    mock.reset();
    handler.handleMessageAt(ownEcho, mock, 101);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(1, handler.framesSent);
    TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)bChannelDiag.echoConfirmCount);
}

void test_recent_echo_history_recognizes_older_delayed_echo()
{
    setMode(kNagMode1);
    CanFrame first = makeEpasFrame(0, 0.31f, 0x01);
    CanFrame second = makeEpasFrame(0, 0.32f, 0x02);
    handler.handleMessageAt(first, mock, 100);
    handler.handleMessageAt(second, mock, 101);
    TEST_ASSERT_EQUAL(2, mock.sent.size());
    CanFrame delayedFirstEcho = mock.sent[0];

    mock.reset();
    handler.handleMessageAt(delayedFirstEcho, mock, 102);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)bChannelDiag.echoConfirmCount);
}

void test_production_warmup_requires_time_and_one_thousand_880_frames()
{
    setMode(kNagMode1);
    handler.onCanStarted(100);

    CanFrame beforeTime = makeEpasFrame(0, 0.30f, 0x01);
    handler.handleMessageAt(beforeTime, mock, 15099);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(kNagReadinessWarmupTime,
                            (uint8_t)bChannelDiag.nagReadiness);

    for (uint16_t i = 1; i < 999; ++i) {
        CanFrame frame = makeEpasFrame(0, 0.30f, static_cast<uint8_t>(i & 0x0F));
        handler.handleMessageAt(frame, mock, 15100);
    }
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(999, (uint32_t)bChannelDiag.nagWarmupFramesSeen);
    TEST_ASSERT_EQUAL_UINT8(kNagReadinessWarmupFrames,
                            (uint8_t)bChannelDiag.nagReadiness);

    CanFrame ready = makeEpasFrame(0, 0.30f, 0x0F);
    handler.handleMessageAt(ready, mock, 15100);
    TEST_ASSERT_TRUE((bool)bChannelDiag.nagReady);
    TEST_ASSERT_EQUAL_UINT8(kNagReadinessReady,
                            (uint8_t)bChannelDiag.nagReadiness);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
}

void test_echo_later_than_six_milliseconds_is_dropped()
{
    setMode(kNagMode1);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    uint32_t oldRxUs = micros() - (kNagEchoDeadlineUs + 1000);
    handler.handleMessageAt(frame, mock, 100, true, oldRxUs);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)bChannelDiag.echoDroppedLate);
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionLateDrop,
                            (uint8_t)bChannelDiag.nagLastDecision);
}

void test_runtime_off_blocks_all_modes()
{
    setMode(kNagMode1);
    nagKillerRuntime = false;
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 100);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionRuntimeOff, (uint8_t)bChannelDiag.nagLastDecision);
}

void test_a_channel_eflg_health_classification_preserves_rx_overrun()
{
    TEST_ASSERT_EQUAL_STRING("OK", aMcpEflgStateName(0x00));
    TEST_ASSERT_EQUAL_STRING("RX_OVERRUN", aMcpEflgStateName(0x40));
    TEST_ASSERT_EQUAL_STRING("RX_OVERRUN", aMcpEflgStateName(0x41));
    TEST_ASSERT_EQUAL_STRING("ERROR_WARNING", aMcpEflgStateName(0x01));
    TEST_ASSERT_EQUAL_STRING("ERROR_PASSIVE", aMcpEflgStateName(0x08));
    TEST_ASSERT_EQUAL_STRING("BUS_OFF", aMcpEflgStateName(0x20));
    TEST_ASSERT_EQUAL_UINT8(0, aMcpEflgSeverity(0x00));
    TEST_ASSERT_EQUAL_UINT8(1, aMcpEflgSeverity(0x40));
    TEST_ASSERT_EQUAL_UINT8(2, aMcpEflgSeverity(0x20));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_nag_defaults_off_with_mode2_ap_only_selected);
    RUN_TEST(test_a_tx_guard_uses_hard_error_only);
    RUN_TEST(test_nag_mode_clamp_accepts_1_and_2_and_defaults_retired_3_to_2);
    RUN_TEST(test_nag_filter_contains_only_required_ids);
    RUN_TEST(test_mode1_fixed_echo_matches_verified_frame_rules);
    RUN_TEST(test_mode1_does_not_require_das_or_steering_context);
    RUN_TEST(test_mode1_blocks_real_hands_on);
    RUN_TEST(test_mode2_cycles_four_torques_then_pauses);
    RUN_TEST(test_mode2_does_not_require_das_or_steering_context);
    RUN_TEST(test_mode2_ap_only_blocks_without_fresh_ap_context);
    RUN_TEST(test_mode2_ap_only_blocks_general_drive_and_allows_active_ap);
    RUN_TEST(test_mode1_original_scope_ignores_ap_when_ap_only_is_off);
    RUN_TEST(test_all_modes_block_when_driver_hands_are_detected);
    RUN_TEST(test_all_modes_stay_inside_common_torque_cap);
    RUN_TEST(test_failed_send_does_not_increment_success_counters);
    RUN_TEST(test_successful_echo_is_not_reprocessed);
    RUN_TEST(test_recent_echo_history_recognizes_older_delayed_echo);
    RUN_TEST(test_production_warmup_requires_time_and_one_thousand_880_frames);
    RUN_TEST(test_echo_later_than_six_milliseconds_is_dropped);
    RUN_TEST(test_runtime_off_blocks_all_modes);
    RUN_TEST(test_a_channel_eflg_health_classification_preserves_rx_overrun);
    return UNITY_END();
}
