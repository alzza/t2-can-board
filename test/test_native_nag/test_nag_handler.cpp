#include <unity.h>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "handlers.h"
#include "drivers/mock_driver.h"

static MockDriver mock;
static NagHandler handler;

// Helper: build a realistic CAN 880 frame
static CanFrame makeEpasFrame(uint8_t handsOn, float torqueNm, uint8_t counter, uint8_t eacStatus = 2)
{
    CanFrame f = {.id = 880, .dlc = 8};
    // bytes 0-1: steeringRackForce (arbitrary realistic values)
    f.data[0] = 0x12;
    f.data[1] = 0x00;
    // bytes 2-3: torsionBarTorque = (torque + 20.5) / 0.01
    uint16_t tRaw = static_cast<uint16_t>((torqueNm + 20.5) / 0.01);
    f.data[2] = 0x08 | ((tRaw >> 8) & 0x0F); // upper nibble = flags (0x08)
    f.data[3] = tRaw & 0xFF;
    // byte 4: handsOnLevel in bits 7:6, internalSAS bits in lower
    f.data[4] = static_cast<uint8_t>((handsOn & 0x03) << 6) | 0x1F;
    // byte 5: internalSAS LSB
    f.data[5] = 0x89;
    // byte 6: upper nibble = eacStatus/tireID, lower nibble = counter
    f.data[6] = static_cast<uint8_t>((eacStatus << 5) | (counter & 0x0F));
    // byte 7: checksum = sum(b0..b6) + 0x73
    uint16_t sum = 0;
    for (int i = 0; i < 7; i++)
        sum += f.data[i];
    f.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);
    return f;
}

// Helper: verify checksum of a frame
static bool verifyChecksum(const CanFrame &f)
{
    uint16_t sum = 0;
    for (int i = 0; i < 7; i++)
        sum += f.data[i];
    return f.data[7] == static_cast<uint8_t>((sum + 0x73) & 0xFF);
}

static CanFrame makeDasFrame(uint8_t handsOnState, uint32_t id = 921, uint8_t apState = 3)
{
    CanFrame frame = {.id = id, .dlc = 8};
    frame.data[0] = apState & 0x0F;
    frame.data[5] = static_cast<uint8_t>((handsOnState & 0x0F) << 2);
    return frame;
}

static void primeDasRequest(uint8_t handsOnState = 2, uint32_t id = 921, uint8_t apState = 3)
{
    CanFrame dasFrame = makeDasFrame(handsOnState, id, apState);
    handler.handleMessage(dasFrame, mock);
    if (apState >= 3 && apState <= 6 && handsOnState == 2)
    {
        CanFrame warmupFrame = makeEpasFrame(0, 0.33f, 0x00);
        handler.handleMessage(warmupFrame, mock);
        handler._mbState2EnterMs = millis() - nagSmartProfileSettings(nagConfig.smartProfile).state2DelayMs - 1;
        mock.reset();
    }
}

void setUp()
{
    mock.reset();
    bChannelDiag = BChannelDiagnostics();
    nagCfgDefaultsSmart(nagConfig);
    handler = NagHandler();
    handler.enablePrint = false;
}

void tearDown() {}

// ============================================================
// Filter IDs
// ============================================================

void test_nag_filter_ids_count()
{
    // 880 + 921/923(DAS 후보) + 297(Mode B 조향각) = 4
    TEST_ASSERT_EQUAL_UINT8(4, handler.filterIdCount());
}

void test_nag_filter_ids_value()
{
    const uint32_t *ids = handler.filterIds();
    TEST_ASSERT_EQUAL_UINT32(880, ids[0]);
    TEST_ASSERT_EQUAL_UINT32(921, ids[1]);
    TEST_ASSERT_EQUAL_UINT32(923, ids[2]);
    TEST_ASSERT_EQUAL_UINT32(297, ids[3]);
}

// ============================================================
// Basic echo behavior
// ============================================================

void test_nag_echoes_when_handson_0()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
}

void test_nag_does_not_echo_when_das_state_0()
{
    primeDasRequest(0);
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionDasIdle, (uint8_t)bChannelDiag.nagLastDecision);
}

void test_nag_does_not_echo_when_handson_1()
{
    CanFrame f = makeEpasFrame(1, 1.5, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_nag_does_not_echo_when_handson_2()
{
    CanFrame f = makeEpasFrame(2, 2.5, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_nag_does_not_echo_when_handson_3()
{
    CanFrame f = makeEpasFrame(3, 3.0, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_nag_does_not_echo_when_disabled()
{
    handler.nagKillerActive = false;
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_nag_ignores_non_880_id()
{
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    f.id = 881; // wrong ID
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_nag_ignores_short_dlc()
{
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    f.dlc = 7; // too short
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

// ============================================================
// Counter+1 logic
// ============================================================

void test_nag_counter_increments_by_1()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    uint8_t outCounter = mock.sent[0].data[6] & 0x0F;
    TEST_ASSERT_EQUAL_HEX8(0x0D, outCounter); // 0x0C + 1
}

void test_nag_counter_wraps_from_f_to_0()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0F);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    uint8_t outCounter = mock.sent[0].data[6] & 0x0F;
    TEST_ASSERT_EQUAL_HEX8(0x00, outCounter); // 0x0F + 1 wraps to 0
}

void test_nag_counter_preserves_upper_nibble()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x05, 2); // eacStatus=2 -> upper nibble = 0x40
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    uint8_t upperNibble = mock.sent[0].data[6] & 0xF0;
    uint8_t expectedUpper = f.data[6] & 0xF0;
    TEST_ASSERT_EQUAL_HEX8(expectedUpper, upperNibble);
}

// ============================================================
// Modified field values
// ============================================================

void test_nag_sets_handson_to_1()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    uint8_t outHandsOn = (mock.sent[0].data[4] >> 6) & 0x03;
    TEST_ASSERT_TRUE(outHandsOn <= 1);
}

void test_nag_preserves_byte4_lower_bits()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    f.data[4] = 0x1F; // handsOn=0, lower bits = 0x1F
    handler.handleMessage(f, mock);
    uint8_t outLower = mock.sent[0].data[4] & 0x3F;
    TEST_ASSERT_EQUAL_HEX8(0x1F, outLower); // lower 6 bits preserved
}

void test_nag_torque_walk_not_fixed_0x08B6()
{
    primeDasRequest();
    bool seenNonFixed = false;
    for (int i = 0; i < 10 && !seenNonFixed; i++)
    {
        mock.reset();
        CanFrame f = makeEpasFrame(0, 0.33f, static_cast<uint8_t>(i & 0x0F));
        handler.handleMessage(f, mock);
        uint16_t tRaw = ((mock.sent[0].data[2] & 0x0F) << 8) | mock.sent[0].data[3];
        if (tRaw != 0x08B6) seenNonFixed = true;
    }
    TEST_ASSERT_TRUE(seenNonFixed);
}

void test_nag_copies_bytes_0_1_2_5_unchanged()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    f.data[0] = 0xAB;
    f.data[1] = 0xCD;
    f.data[2] = 0x8E; // upper nibble has flags
    f.data[5] = 0x42;
    // Recompute checksum after manual changes
    uint16_t sum = 0;
    for (int i = 0; i < 7; i++)
        sum += f.data[i];
    f.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);

    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL_HEX8(0xAB, mock.sent[0].data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xCD, mock.sent[0].data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x80, mock.sent[0].data[2] & 0xF0); // upper nibble preserved
    TEST_ASSERT_EQUAL_HEX8(0x42, mock.sent[0].data[5]);
}

// ============================================================
// Checksum verification
// ============================================================

void test_nag_checksum_correct()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_TRUE(verifyChecksum(mock.sent[0]));
}

void test_nag_checksum_correct_at_counter_boundary()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0F); // counter wraps
    handler.handleMessage(f, mock);
    TEST_ASSERT_TRUE(verifyChecksum(mock.sent[0]));
}

void test_nag_checksum_correct_with_various_inputs()
{
    // Test across multiple counter values and torques
    primeDasRequest();
    for (uint8_t cnt = 0; cnt < 16; cnt++)
    {
        mock.reset();
        CanFrame f = makeEpasFrame(0, -5.0 + cnt * 0.7, cnt);
        handler.handleMessage(f, mock);
        TEST_ASSERT_EQUAL(1, mock.sent.size());
        TEST_ASSERT_TRUE_MESSAGE(verifyChecksum(mock.sent[0]), "Checksum failed for counter sweep");
    }
}

// ============================================================
// Canary: output torque must stay in safe range
// ============================================================

void test_nag_output_torque_never_exceeds_safe_range()
{
    // Smart Torque state2 mild 출력 토크가 의도 범위를 벗어나지 않는지 확인
    primeDasRequest();
    for (uint8_t cnt = 0; cnt < 16; cnt++)
    {
        mock.reset();
        CanFrame f = makeEpasFrame(0, -20.0 + cnt * 2.5, cnt);
        handler.handleMessage(f, mock);
        TEST_ASSERT_EQUAL(1, mock.sent.size());

        uint16_t tRaw = ((mock.sent[0].data[2] & 0x0F) << 8) | mock.sent[0].data[3];
        TEST_ASSERT_TRUE(tRaw >= 2098);
        TEST_ASSERT_TRUE(tRaw <= 2198);
    }
}

void test_nag_output_raw_stays_in_state2_mild_range()
{
    primeDasRequest();
    for (uint8_t cnt = 0; cnt < 32; cnt++)
    {
        mock.reset();
        CanFrame f = makeEpasFrame(0, 0.33f, cnt & 0x0F);
        handler.handleMessage(f, mock);
        TEST_ASSERT_EQUAL(1, mock.sent.size());

        uint16_t tRaw = ((mock.sent[0].data[2] & 0x0F) << 8) | mock.sent[0].data[3];
        TEST_ASSERT_TRUE(tRaw >= 2098);
        TEST_ASSERT_TRUE(tRaw <= 2198);
    }
}

void test_nag_profile_c_settings_match_delay_torque_candidate()
{
    const NagSmartProfileSettings &profile = nagSmartProfileSettings(kNagSmartProfileC);
    TEST_ASSERT_EQUAL_UINT8(kNagSmartProfileC, profile.id);
    TEST_ASSERT_EQUAL_UINT16(600, profile.state2DelayMs);
    TEST_ASSERT_EQUAL_UINT16(400, profile.strongDelayMs);
    TEST_ASSERT_EQUAL_UINT16(50, profile.state2MildMinRawDelta);
    TEST_ASSERT_EQUAL_UINT16(170, profile.state2MildMaxRawDelta);
}

void test_nag_profile_clamp_accepts_c_and_rejects_out_of_range()
{
    TEST_ASSERT_EQUAL_UINT8(kNagSmartProfileC, nagSmartProfileClamp(kNagSmartProfileC));
    TEST_ASSERT_EQUAL_UINT8(kNagSmartProfileDefault, nagSmartProfileClamp(99));
}

void test_nag_profile_c_output_raw_stays_in_state2_mild_range()
{
    nagConfig.smartProfile = kNagSmartProfileC;
    primeDasRequest();
    for (uint8_t cnt = 0; cnt < 32; cnt++)
    {
        mock.reset();
        CanFrame f = makeEpasFrame(0, 0.33f, cnt & 0x0F);
        handler.handleMessage(f, mock);
        TEST_ASSERT_EQUAL(1, mock.sent.size());

        uint16_t tRaw = ((mock.sent[0].data[2] & 0x0F) << 8) | mock.sent[0].data[3];
        TEST_ASSERT_TRUE(tRaw >= 2098);
        TEST_ASSERT_TRUE(tRaw <= 2218);
    }
}

void test_nag_output_handson_never_exceeds_1()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    uint8_t ho = (mock.sent[0].data[4] >> 6) & 0x03;
    TEST_ASSERT_TRUE(ho <= 1);
}

// ============================================================
// Frame count tracking
// ============================================================

void test_nag_increments_frames_sent()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL_UINT32(1, handler.framesSent);
}

void test_nag_increments_echo_count()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL_UINT32(1, handler.nagEchoCount);
}

void test_nag_multiple_frames_count_correctly()
{
    primeDasRequest();
    for (int i = 0; i < 10; i++)
    {
        CanFrame f = makeEpasFrame(0, 0.33, i & 0x0F);
        handler.handleMessage(f, mock);
    }
    TEST_ASSERT_EQUAL_UINT32(10, handler.nagEchoCount);
    TEST_ASSERT_EQUAL(10, mock.sent.size());
}

void test_nag_blocks_no_das_fallback()
{
    for (int index = 0; index < 121; index++)
    {
        CanFrame frame = makeEpasFrame(0, 0.33, index & 0x0F);
        handler.handleMessage(frame, mock);
    }
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionApBlocked, (uint8_t)bChannelDiag.nagLastDecision);
}

void test_nag_updates_das_state_on_921()
{
    TEST_ASSERT_EQUAL_UINT8(0xFF, handler.dasHandsOnState);

    CanFrame dasFrame = makeDasFrame(2);
    handler.handleMessage(dasFrame, mock);

    TEST_ASSERT_EQUAL_UINT8(2, (uint8_t)handler.dasHandsOnState);
}

void test_nag_echoes_after_das_request_update()
{
    for (int index = 0; index < 121; index++)
    {
        CanFrame frame = makeEpasFrame(0, 0.33, index & 0x0F);
        handler.handleMessage(frame, mock);
    }
    TEST_ASSERT_EQUAL(0, mock.sent.size());

    primeDasRequest();

    CanFrame epasFrame = makeEpasFrame(0, 0.33, 0x01);
    handler.handleMessage(epasFrame, mock);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
}

// ============================================================
// Edge case: mixed handsOn sequence
// ============================================================

void test_nag_echoes_only_handson_0_in_mixed_sequence()
{
    primeDasRequest();
    // Simulate: ho=0, ho=1, ho=0, ho=2, ho=0
    CanFrame f0a = makeEpasFrame(0, 0.33, 0x00);
    CanFrame f1 = makeEpasFrame(1, 1.50, 0x01);
    CanFrame f0b = makeEpasFrame(0, 0.10, 0x02);
    CanFrame f2 = makeEpasFrame(2, 2.50, 0x03);
    CanFrame f0c = makeEpasFrame(0, 0.05, 0x04);

    handler.handleMessage(f0a, mock);
    handler.handleMessage(f1, mock);
    handler.handleMessage(f0b, mock);
    handler.handleMessage(f2, mock);
    handler.handleMessage(f0c, mock);

    TEST_ASSERT_EQUAL(3, mock.sent.size()); // only 3 echoes for ho=0
}

// ============================================================
// Output ID is always 880
// ============================================================

void test_nag_output_id_is_880()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL_UINT32(880, mock.sent[0].id);
}

void test_nag_output_dlc_is_8()
{
    primeDasRequest();
    CanFrame f = makeEpasFrame(0, 0.33, 0x0C);
    handler.handleMessage(f, mock);
    TEST_ASSERT_EQUAL_UINT8(8, mock.sent[0].dlc);
}

int main()
{
    UNITY_BEGIN();

    // Filter
    RUN_TEST(test_nag_filter_ids_count);
    RUN_TEST(test_nag_filter_ids_value);

    // Basic echo behavior
    RUN_TEST(test_nag_echoes_when_handson_0);
    RUN_TEST(test_nag_does_not_echo_when_das_state_0);
    RUN_TEST(test_nag_does_not_echo_when_handson_1);
    RUN_TEST(test_nag_does_not_echo_when_handson_2);
    RUN_TEST(test_nag_does_not_echo_when_handson_3);
    RUN_TEST(test_nag_does_not_echo_when_disabled);
    RUN_TEST(test_nag_ignores_non_880_id);
    RUN_TEST(test_nag_ignores_short_dlc);

    // Counter+1
    RUN_TEST(test_nag_counter_increments_by_1);
    RUN_TEST(test_nag_counter_wraps_from_f_to_0);
    RUN_TEST(test_nag_counter_preserves_upper_nibble);

    // Modified fields
    RUN_TEST(test_nag_sets_handson_to_1);
    RUN_TEST(test_nag_preserves_byte4_lower_bits);
    RUN_TEST(test_nag_torque_walk_not_fixed_0x08B6);
    RUN_TEST(test_nag_copies_bytes_0_1_2_5_unchanged);

    // Checksum
    RUN_TEST(test_nag_checksum_correct);
    RUN_TEST(test_nag_checksum_correct_at_counter_boundary);
    RUN_TEST(test_nag_checksum_correct_with_various_inputs);

    // Safety canary
    RUN_TEST(test_nag_output_torque_never_exceeds_safe_range);
    RUN_TEST(test_nag_output_raw_stays_in_state2_mild_range);
    RUN_TEST(test_nag_profile_c_settings_match_delay_torque_candidate);
    RUN_TEST(test_nag_profile_clamp_accepts_c_and_rejects_out_of_range);
    RUN_TEST(test_nag_profile_c_output_raw_stays_in_state2_mild_range);
    RUN_TEST(test_nag_output_handson_never_exceeds_1);

    // Counters
    RUN_TEST(test_nag_increments_frames_sent);
    RUN_TEST(test_nag_increments_echo_count);
    RUN_TEST(test_nag_multiple_frames_count_correctly);
    RUN_TEST(test_nag_blocks_no_das_fallback);
    RUN_TEST(test_nag_updates_das_state_on_921);
    RUN_TEST(test_nag_echoes_after_das_request_update);

    // Edge cases
    RUN_TEST(test_nag_echoes_only_handson_0_in_mixed_sequence);

    // Output frame
    RUN_TEST(test_nag_output_id_is_880);
    RUN_TEST(test_nag_output_dlc_is_8);

    return UNITY_END();
}
