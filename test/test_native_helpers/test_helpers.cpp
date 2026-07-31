#include <unity.h>
#include "can_frame_types.h"
#include "can_helpers.h"
#include "event_log.h"

void setUp()
{
    bypassTlsscRequirementRuntime = kBypassTlsscRequirementDefaultEnabled;
    isaSpeedChimeSuppressRuntime = kIsaSpeedChimeSuppressDefaultEnabled;
    emergencyVehicleDetectionRuntime = kEmergencyVehicleDetectionDefaultEnabled;
    summonUnlockRuntime = kSummonUnlockDefaultEnabled;
    tsllcRuntime = false;
    nagKillerRuntime = false;
    aChannelTxRuntime = false;
    aMcpOneShotRuntime = false;
    aTxGuardRuntime = false;
    nagApOnlyRuntime = false;
}
void tearDown() {}

// --- setBit ---

void test_setBit_sets_bit0_of_byte0()
{
    CanFrame f = {};
    setBit(f, 0, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, f.data[0]);
}

void test_setBit_sets_bit7_of_byte0()
{
    CanFrame f = {};
    setBit(f, 7, true);
    TEST_ASSERT_EQUAL_HEX8(0x80, f.data[0]);
}

void test_setBit_sets_bit_in_byte5()
{
    CanFrame f = {};
    setBit(f, 46, true); // byte 5, bit 6
    TEST_ASSERT_EQUAL_HEX8(0x40, f.data[5]);
}

void test_setBit_sets_bit_in_byte7()
{
    CanFrame f = {};
    setBit(f, 60, true); // byte 7, bit 4
    TEST_ASSERT_EQUAL_HEX8(0x10, f.data[7]);
}

void test_setBit_clears_bit()
{
    CanFrame f = {};
    f.data[2] = 0xFF;
    setBit(f, 19, false); // byte 2, bit 3
    TEST_ASSERT_EQUAL_HEX8(0xF7, f.data[2]);
}

void test_setBit_does_not_affect_other_bytes()
{
    CanFrame f = {};
    f.data[0] = 0xAA;
    f.data[1] = 0xBB;
    setBit(f, 8, true); // byte 1, bit 0
    TEST_ASSERT_EQUAL_HEX8(0xAA, f.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, f.data[1]);
}

// --- readMuxID ---

void test_readMuxID_extracts_lower_3_bits()
{
    CanFrame f = {};
    f.data[0] = 0x05;
    TEST_ASSERT_EQUAL_UINT8(5, readMuxID(f));
}

void test_readMuxID_masks_upper_bits()
{
    CanFrame f = {};
    f.data[0] = 0xFA; // binary: 11111010 -> lower 3 = 010 = 2
    TEST_ASSERT_EQUAL_UINT8(2, readMuxID(f));
}

void test_readMuxID_zero()
{
    CanFrame f = {};
    f.data[0] = 0x00;
    TEST_ASSERT_EQUAL_UINT8(0, readMuxID(f));
}

void test_readMuxID_max_value()
{
    CanFrame f = {};
    f.data[0] = 0x07;
    TEST_ASSERT_EQUAL_UINT8(7, readMuxID(f));
}

// --- isFSDSelectedInUI ---

void test_isFSDSelectedInUI_true_when_bit6_set()
{
    CanFrame f = {};
    f.data[4] = 0x40; // bit 6 set
    TEST_ASSERT_TRUE(isFSDSelectedInUI(f));
}

void test_isFSDSelectedInUI_false_when_bit6_clear()
{
    CanFrame f = {};
    f.data[4] = 0x00;
    TEST_ASSERT_FALSE(isFSDSelectedInUI(f));
}

void test_isFSDSelectedInUI_ignores_other_bits()
{
    CanFrame f = {};
    f.data[4] = 0xBF; // all bits set except bit 6
    TEST_ASSERT_FALSE(isFSDSelectedInUI(f));
}

void test_isFSDSelectedInUI_true_with_other_bits()
{
    CanFrame f = {};
    f.data[4] = 0xFF;
    TEST_ASSERT_TRUE(isFSDSelectedInUI(f));
}

// --- setSpeedProfileV12V13 ---

void test_setSpeedProfileV12V13_sets_profile_0()
{
    CanFrame f = {};
    f.data[6] = 0xFF;
    setSpeedProfileV12V13(f, 0);
    TEST_ASSERT_EQUAL_HEX8(0xF9, f.data[6]); // bits 1-2 cleared
}

void test_setSpeedProfileV12V13_sets_profile_1()
{
    CanFrame f = {};
    f.data[6] = 0x00;
    setSpeedProfileV12V13(f, 1);
    TEST_ASSERT_EQUAL_HEX8(0x02, f.data[6]);
}

void test_setSpeedProfileV12V13_sets_profile_2()
{
    CanFrame f = {};
    f.data[6] = 0x00;
    setSpeedProfileV12V13(f, 2);
    TEST_ASSERT_EQUAL_HEX8(0x04, f.data[6]);
}

void test_setSpeedProfileV12V13_preserves_other_bits()
{
    CanFrame f = {};
    f.data[6] = 0xF9; // bits 1-2 clear, rest set
    setSpeedProfileV12V13(f, 1);
    TEST_ASSERT_EQUAL_HEX8(0xFB, f.data[6]);
}

// --- Track mode helpers ---

void test_setTrackModeRequest_sets_on_and_preserves_upper_bits()
{
    CanFrame f = {};
    f.data[0] = 0xFE;
    setTrackModeRequest(f, kTrackModeRequestOn);
    TEST_ASSERT_EQUAL_HEX8(0xFD, f.data[0]);
}

void test_computeTeslaChecksum_sums_payload_and_frame_id()
{
    CanFrame f = {.id = 787, .dlc = 8};
    f.data[0] = 0xFD;
    f.data[1] = 0x10;
    f.data[2] = 0x20;
    f.data[3] = 0x04;
    f.data[4] = 0x00;
    f.data[5] = 0x00;
    f.data[6] = 0xA0;
    f.data[7] = 0x00;
    TEST_ASSERT_EQUAL_HEX8(0xE7, computeTeslaChecksum(f));
}

void test_finalizeTeslaCounter52Checksum56_advances_counter_and_checksum()
{
    CanFrame f = {.id = 659, .dlc = 8};
    f.data[0] = 0x10;
    f.data[1] = 0x20;
    f.data[2] = 0x30;
    f.data[3] = 0x01;
    f.data[4] = 0x40;
    f.data[5] = 0x50;
    f.data[6] = 0xA5;

    finalizeTeslaCounter52Checksum56(f);

    TEST_ASSERT_EQUAL_HEX8(0xB5, f.data[6]);
    TEST_ASSERT_EQUAL_HEX8(computeTeslaChecksum(f), f.data[7]);
}

void test_finalizeTeslaCounter52Checksum56_wraps_counter()
{
    CanFrame f = {.id = 659, .dlc = 8};
    f.data[6] = 0xF2;

    finalizeTeslaCounter52Checksum56(f);

    TEST_ASSERT_EQUAL_HEX8(0x02, f.data[6]);
    TEST_ASSERT_EQUAL_HEX8(computeTeslaChecksum(f), f.data[7]);
}

void test_finalizeTeslaCounter52Checksum56_ignores_short_frame()
{
    CanFrame f = {.id = 659, .dlc = 7};
    f.data[6] = 0xA5;
    f.data[7] = 0xEE;

    finalizeTeslaCounter52Checksum56(f);

    TEST_ASSERT_EQUAL_HEX8(0xA5, f.data[6]);
    TEST_ASSERT_EQUAL_HEX8(0xEE, f.data[7]);
}

void test_signalObserverExtractRaw_reads_little_endian_signal()
{
    CanFrame f = {.id = 585, .dlc = 8};
    f.data[2] = 0x05;
    SignalObserverDef def = {};
    def.enabled = true;
    def.channelMask = kSignalObserverChannelBoth;
    def.byteOrder = kSignalObserverByteOrderLittle;
    def.frameId = 585;
    def.startBit = 16;
    def.length = 4;

    uint32_t raw = 0;
    TEST_ASSERT_TRUE(signalObserverExtractRaw(f, def, raw));
    TEST_ASSERT_EQUAL_UINT32(5, raw);
}

void test_signalObserverExtractRaw_reads_big_endian_signal()
{
    CanFrame f = {.id = 880, .dlc = 8};
    f.data[2] = 0x08;
    f.data[3] = 0xB6;
    SignalObserverDef def = {};
    def.enabled = true;
    def.channelMask = kSignalObserverChannelB;
    def.byteOrder = kSignalObserverByteOrderBig;
    def.frameId = 880;
    def.startBit = 19;
    def.length = 12;

    uint32_t raw = 0;
    TEST_ASSERT_TRUE(signalObserverExtractRaw(f, def, raw));
    TEST_ASSERT_EQUAL_HEX32(0x8B6, raw);
}

void test_signalObserverExtractRaw_rejects_short_big_endian_frame()
{
    CanFrame f = {.id = 880, .dlc = 3};
    f.data[2] = 0x08;
    SignalObserverDef def = {};
    def.enabled = true;
    def.channelMask = kSignalObserverChannelB;
    def.byteOrder = kSignalObserverByteOrderBig;
    def.frameId = 880;
    def.startBit = 19;
    def.length = 12;

    uint32_t raw = 0;
    TEST_ASSERT_FALSE(signalObserverExtractRaw(f, def, raw));
}

void test_signalObserver_defaults_start_stopped_without_retired_experiment_signal()
{
    TEST_ASSERT_FALSE(signalObserverRuntime);
    TEST_ASSERT_EQUAL_UINT8(0, signalObserverCount);
    TEST_ASSERT_FALSE(signalObserverDefs[0].enabled);
}

void test_signalObserverExtractRaw_requires_matching_mux_value()
{
    CanFrame f = {.id = 585, .dlc = 8};
    f.data[0] = 0x00;
    f.data[2] = 0x05;

    SignalObserverDef def = {};
    def.enabled = true;
    def.channelMask = kSignalObserverChannelA;
    def.byteOrder = kSignalObserverByteOrderLittle;
    def.frameId = 585;
    def.startBit = 16;
    def.length = 4;
    def.muxStartBit = 0;
    def.muxLength = 3;
    def.muxValue = 1;

    uint32_t raw = 0;
    TEST_ASSERT_FALSE(signalObserverExtractRaw(f, def, raw));

    f.data[0] = 0x01;
    TEST_ASSERT_TRUE(signalObserverExtractRaw(f, def, raw));
    TEST_ASSERT_EQUAL_UINT32(5, raw);
}

// --- Runtime BYPASS_TLSSC_REQUIREMENT ---

void test_runtime_bypass_tlssc_overrides_when_bit_clear()
{
    bypassTlsscRequirementRuntime = true;
    CanFrame f = {};
    f.data[4] = 0x00;
    TEST_ASSERT_TRUE(isFSDSelectedInUI(f));
    bypassTlsscRequirementRuntime = false;
}

void test_runtime_bypass_tlssc_off_reads_frame()
{
    bypassTlsscRequirementRuntime = false;
    CanFrame f = {};
    f.data[4] = 0x00;
    TEST_ASSERT_FALSE(isFSDSelectedInUI(f));
}

void test_runtime_bypass_tlssc_off_still_reads_real_bit()
{
    bypassTlsscRequirementRuntime = false;
    CanFrame f = {};
    f.data[4] = 0x40;
    TEST_ASSERT_TRUE(isFSDSelectedInUI(f));
}

void test_runtime_defaults_match_build_configuration()
{
    TEST_ASSERT_FALSE(bypassTlsscRequirementRuntime);
    TEST_ASSERT_TRUE(isaSpeedChimeSuppressRuntime);
    TEST_ASSERT_TRUE(emergencyVehicleDetectionRuntime);
    TEST_ASSERT_TRUE(summonUnlockRuntime);
}

void test_user_marker_names_are_generic()
{
    TEST_ASSERT_EQUAL_STRING("USER_MARK_START", userMarkerDetailName(kUserMarkerStart));
    TEST_ASSERT_EQUAL_STRING("USER_MARK_END", userMarkerDetailName(kUserMarkerEnd));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", userMarkerDetailName(0));
}

void test_feature_state_detail_includes_nag_ap_only_and_summon_condition()
{
    aChannelTxRuntime = true;
    summonUnlockRuntime = true;
    tsllcRuntime = true;
    nagKillerRuntime = true;
    nagApOnlyRuntime = true;
    summonConditionLimitRuntime = true;
    TEST_ASSERT_EQUAL_HEX32(0xCF, eventFeatureStateDetail());
}

void test_feature_activity_detail_maps_each_feature()
{
    nagApOnlyRuntime = true;
    TEST_ASSERT_EQUAL_HEX32(0x6D,
        eventFeatureActivityDetail(true, false, true, true, false, true));
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_setBit_sets_bit0_of_byte0);
    RUN_TEST(test_setBit_sets_bit7_of_byte0);
    RUN_TEST(test_setBit_sets_bit_in_byte5);
    RUN_TEST(test_setBit_sets_bit_in_byte7);
    RUN_TEST(test_setBit_clears_bit);
    RUN_TEST(test_setBit_does_not_affect_other_bytes);

    RUN_TEST(test_readMuxID_extracts_lower_3_bits);
    RUN_TEST(test_readMuxID_masks_upper_bits);
    RUN_TEST(test_readMuxID_zero);
    RUN_TEST(test_readMuxID_max_value);

    RUN_TEST(test_isFSDSelectedInUI_true_when_bit6_set);
    RUN_TEST(test_isFSDSelectedInUI_false_when_bit6_clear);
    RUN_TEST(test_isFSDSelectedInUI_ignores_other_bits);
    RUN_TEST(test_isFSDSelectedInUI_true_with_other_bits);

    RUN_TEST(test_setSpeedProfileV12V13_sets_profile_0);
    RUN_TEST(test_setSpeedProfileV12V13_sets_profile_1);
    RUN_TEST(test_setSpeedProfileV12V13_sets_profile_2);
    RUN_TEST(test_setSpeedProfileV12V13_preserves_other_bits);
    RUN_TEST(test_setTrackModeRequest_sets_on_and_preserves_upper_bits);
    RUN_TEST(test_computeTeslaChecksum_sums_payload_and_frame_id);
    RUN_TEST(test_finalizeTeslaCounter52Checksum56_advances_counter_and_checksum);
    RUN_TEST(test_finalizeTeslaCounter52Checksum56_wraps_counter);
    RUN_TEST(test_finalizeTeslaCounter52Checksum56_ignores_short_frame);
    RUN_TEST(test_signalObserverExtractRaw_reads_little_endian_signal);
    RUN_TEST(test_signalObserverExtractRaw_reads_big_endian_signal);
    RUN_TEST(test_signalObserverExtractRaw_rejects_short_big_endian_frame);
    RUN_TEST(test_signalObserver_defaults_start_stopped_without_retired_experiment_signal);
    RUN_TEST(test_signalObserverExtractRaw_requires_matching_mux_value);

    RUN_TEST(test_runtime_bypass_tlssc_overrides_when_bit_clear);
    RUN_TEST(test_runtime_bypass_tlssc_off_reads_frame);
    RUN_TEST(test_runtime_bypass_tlssc_off_still_reads_real_bit);
    RUN_TEST(test_runtime_defaults_match_build_configuration);
    RUN_TEST(test_user_marker_names_are_generic);
    RUN_TEST(test_feature_state_detail_includes_nag_ap_only_and_summon_condition);
    RUN_TEST(test_feature_activity_detail_maps_each_feature);

    return UNITY_END();
}
