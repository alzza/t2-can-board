// HW3 Nag Mode 1/2/3 상태기계와 공통 송신 안전 경계를 검증한다.
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

static CanFrame makeSteeringFrame(float angleDeg, uint8_t validity = 1)
{
    CanFrame frame = {.id = 297, .dlc = 8};
    int raw = static_cast<int>((angleDeg + 819.2f) * 10.0f + 0.5f);
    if (raw < 0) raw = 0;
    if (raw > 0x3FFF) raw = 0x3FFF;
    frame.data[2] = static_cast<uint8_t>(raw & 0xFF);
    frame.data[3] = static_cast<uint8_t>(((raw >> 8) & 0x3F) | ((validity & 0x03) << 6));
    return frame;
}

static uint16_t torqueRaw(const CanFrame &frame)
{
    return static_cast<uint16_t>(((frame.data[2] & 0x0F) << 8) | frame.data[3]);
}

static float torqueNm(const CanFrame &frame)
{
    return torqueRaw(frame) * 0.01f - 20.5f;
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

static void sendContextAt(uint32_t now, uint8_t handsOnState, float angleDeg,
                          uint32_t dasId = 921, uint8_t apState = 6,
                          uint8_t validity = 1)
{
    CanFrame das = makeDasFrame(handsOnState, dasId, apState);
    CanFrame steering = makeSteeringFrame(angleDeg, validity);
    handler.handleMessageAt(das, mock, now);
    handler.handleMessageAt(steering, mock, now);
}

void setUp()
{
    mock.reset();
    bChannelDiag = BChannelDiagnostics();
    nagCfgDefaults(nagConfig);
    nagKillerRuntime = true;
    handler = NagHandler();
}

void tearDown() {}

void test_nag_defaults_off_with_mode3_selected()
{
    NagConfig defaults;
    nagCfgDefaults(defaults);
    TEST_ASSERT_FALSE(kNagKillerDefaultEnabled);
    TEST_ASSERT_EQUAL_UINT8(kNagMode3, defaults.mode);
    TEST_ASSERT_EQUAL_STRING("MODE 3", nagModeName(defaults.mode));
}

void test_nag_mode_clamp_accepts_1_to_3_and_defaults_invalid_to_3()
{
    TEST_ASSERT_EQUAL_UINT8(kNagMode1, nagModeClamp(kNagMode1));
    TEST_ASSERT_EQUAL_UINT8(kNagMode2, nagModeClamp(kNagMode2));
    TEST_ASSERT_EQUAL_UINT8(kNagMode3, nagModeClamp(kNagMode3));
    TEST_ASSERT_EQUAL_UINT8(kNagMode3, nagModeClamp(0));
    TEST_ASSERT_EQUAL_UINT8(kNagMode3, nagModeClamp(99));
}

void test_nag_filter_contains_hw3_context_ids()
{
    const uint32_t *ids = handler.filterIds();
    TEST_ASSERT_EQUAL_UINT8(4, handler.filterIdCount());
    TEST_ASSERT_EQUAL_UINT32(880, ids[0]);
    TEST_ASSERT_EQUAL_UINT32(921, ids[1]);
    TEST_ASSERT_EQUAL_UINT32(923, ids[2]);
    TEST_ASSERT_EQUAL_UINT32(297, ids[3]);
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

void test_mode3_blocks_without_fresh_context()
{
    setMode(kNagMode3);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 1000);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_mode3_blocks_stale_das_or_steering_context()
{
    setMode(kNagMode3);
    sendContextAt(100, 2, 2.0f);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 1101);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_mode3_accepts_923_as_current_project_das_fallback()
{
    setMode(kNagMode3);
    sendContextAt(100, 2, 2.0f, 923);
    sendContextAt(2100, 2, 2.0f, 923);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 2200);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(923, (uint32_t)bChannelDiag.dasStatusSourceId);
}

void test_mode3_state2_waits_two_seconds_then_walks_opposite_steering()
{
    setMode(kNagMode3);
    sendContextAt(100, 2, 2.0f);
    CanFrame early = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(early, mock, 2099);
    TEST_ASSERT_EQUAL(0, mock.sent.size());

    sendContextAt(2100, 2, 2.0f);
    CanFrame ready = makeEpasFrame(0, 0.33f, 0x02);
    handler.handleMessageAt(ready, mock, 2200);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_TRUE(torqueNm(mock.sent[0]) < 0.0f);
    TEST_ASSERT_TRUE(torqueRaw(mock.sent[0]) >= kNagTorqueRawMin);
    TEST_ASSERT_TRUE(torqueRaw(mock.sent[0]) <= kNagTorqueRawMax);
}

void test_mode3_state3_waits_one_second_then_uses_triangle_wave()
{
    setMode(kNagMode3);
    sendContextAt(100, 3, 0.0f);
    CanFrame early = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(early, mock, 1099);
    TEST_ASSERT_EQUAL(0, mock.sent.size());

    sendContextAt(1100, 3, 0.0f);
    CanFrame ready = makeEpasFrame(0, 0.33f, 0x02);
    handler.handleMessageAt(ready, mock, 1100);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_EQUAL_HEX16(kNagTorqueRawMin, torqueRaw(mock.sent[0]));
}

void test_mode3_rejects_invalid_steering_validity()
{
    setMode(kNagMode3);
    sendContextAt(100, 2, 0.0f, 921, 6, 0);
    sendContextAt(2100, 2, 0.0f, 921, 6, 0);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 2200);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_FALSE((bool)bChannelDiag.steeringAngleValid);
}

void test_mode3_rejects_steering_outside_five_degrees()
{
    setMode(kNagMode3);
    sendContextAt(100, 2, 5.1f);
    sendContextAt(2100, 2, 5.1f);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 2200);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_mode3_rejects_ap_outside_active_range()
{
    setMode(kNagMode3);
    sendContextAt(100, 2, 0.0f, 921, 2);
    sendContextAt(2100, 2, 0.0f, 921, 2);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 2200);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionApBlocked, (uint8_t)bChannelDiag.nagLastDecision);
}

void test_mode3_only_supports_das_states_two_and_three()
{
    setMode(kNagMode3);
    sendContextAt(100, 4, 0.0f);
    sendContextAt(2100, 4, 0.0f);
    CanFrame frame = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(frame, mock, 2200);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_switching_to_mode3_resets_and_requires_new_context()
{
    setMode(kNagMode1);
    CanFrame mode1 = makeEpasFrame(0, 0.33f, 0x01);
    handler.handleMessageAt(mode1, mock, 100);
    TEST_ASSERT_EQUAL(1, mock.sent.size());

    mock.reset();
    setMode(kNagMode3);
    CanFrame mode3 = makeEpasFrame(0, 0.33f, 0x02);
    handler.handleMessageAt(mode3, mock, 200);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_all_modes_stay_inside_common_torque_cap()
{
    for (uint8_t mode = kNagMode1; mode <= kNagMode3; mode++) {
        setUp();
        setMode(mode);
        if (mode == kNagMode3) {
            sendContextAt(100, 3, 0.0f);
            sendContextAt(1100, 3, 0.0f);
        }
        CanFrame frame = makeEpasFrame(0, 0.33f, mode);
        handler.handleMessageAt(frame, mock, mode == kNagMode3 ? 1100 : 100);
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
    RUN_TEST(test_nag_defaults_off_with_mode3_selected);
    RUN_TEST(test_nag_mode_clamp_accepts_1_to_3_and_defaults_invalid_to_3);
    RUN_TEST(test_nag_filter_contains_hw3_context_ids);
    RUN_TEST(test_mode1_fixed_echo_matches_verified_frame_rules);
    RUN_TEST(test_mode1_does_not_require_das_or_steering_context);
    RUN_TEST(test_mode1_blocks_real_hands_on);
    RUN_TEST(test_mode2_cycles_four_torques_then_pauses);
    RUN_TEST(test_mode2_does_not_require_das_or_steering_context);
    RUN_TEST(test_mode3_blocks_without_fresh_context);
    RUN_TEST(test_mode3_blocks_stale_das_or_steering_context);
    RUN_TEST(test_mode3_accepts_923_as_current_project_das_fallback);
    RUN_TEST(test_mode3_state2_waits_two_seconds_then_walks_opposite_steering);
    RUN_TEST(test_mode3_state3_waits_one_second_then_uses_triangle_wave);
    RUN_TEST(test_mode3_rejects_invalid_steering_validity);
    RUN_TEST(test_mode3_rejects_steering_outside_five_degrees);
    RUN_TEST(test_mode3_rejects_ap_outside_active_range);
    RUN_TEST(test_mode3_only_supports_das_states_two_and_three);
    RUN_TEST(test_switching_to_mode3_resets_and_requires_new_context);
    RUN_TEST(test_all_modes_stay_inside_common_torque_cap);
    RUN_TEST(test_failed_send_does_not_increment_success_counters);
    RUN_TEST(test_successful_echo_is_not_reprocessed);
    RUN_TEST(test_runtime_off_blocks_all_modes);
    RUN_TEST(test_a_channel_eflg_health_classification_preserves_rx_overrun);
    return UNITY_END();
}
