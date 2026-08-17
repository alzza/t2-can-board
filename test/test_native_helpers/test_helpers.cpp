#include <unity.h>
#include "can_frame_types.h"
#include "can_helpers.h"
#include "event_log.h"
#include "web/monitor_protocol.h"

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
    tsllcRearmRequired = false;
    aChannelDiag.aSafetyHold = false;
    aChannelDiag.aSafetyLatched = false;
    aChannelDiag.aSafetyFirstOverrunMs = 0;
    aChannelDiag.aSafetyHoldCount = 0;
    aChannelDiag.aSafetyLatchCount = 0;
    aChannelDiag.aSafetyRecoveryCount = 0;
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

void test_monitor_protocol_is_compact_versioned_and_read_only()
{
    MonitorSnapshot s;
    s.uptimeS = 42;
    s.firmwareVersion = "1.3.7";
    s.firmwareBuild = "FW137-test";
    s.aHealthLevel = 0;
    s.aHealthState = "OK";
    s.bHealthLevel = 1;
    s.bHealthState = "RECOVERING";
    s.eceR79 = true;
    s.summon = true;
    s.gateReason = "AP_STABLE";
    s.userMarkCount = 3;
    char json[2048];
    const size_t len = formatMonitorJson(json, sizeof(json), s);
    TEST_ASSERT_GREATER_THAN_UINT32(0, len);
    TEST_ASSERT_LESS_THAN_UINT32(sizeof(json), len);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"schema\":1"));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"state\":\"RECOVERING\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"ece_r79\":true"));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"reason\":\"AP_STABLE\""));
}

void test_busoff_recorder_keeps_entry_pending_until_recovery_result()
{
    BusOffEventLog log;
    BusOffEventRecorder recorder;
    const BusOffEvent ev = recorder.begin(1000, 1, 256, 7);

    TEST_ASSERT_TRUE(recorder.pendingValid);
    TEST_ASSERT_EQUAL_UINT32(0, log.count());
    TEST_ASSERT_EQUAL_UINT32(1, ev.seqNum);
    TEST_ASSERT_EQUAL_UINT32(0, ev.sinceLastMs);

    TEST_ASSERT_TRUE(recorder.complete(log, true, 420));
    TEST_ASSERT_FALSE(recorder.pendingValid);
    TEST_ASSERT_EQUAL_UINT32(1, log.count());
    TEST_ASSERT_EQUAL_UINT32(420, log.at(0).recoveryDurMs);
    TEST_ASSERT_EQUAL_UINT8(1, log.at(0).recovered);
}

void test_busoff_recorder_writes_one_row_per_success_or_failure()
{
    BusOffEventLog log;
    BusOffEventRecorder recorder;

    recorder.begin(1000, 1, 256, 0);
    TEST_ASSERT_TRUE(recorder.complete(log, true, 300));
    TEST_ASSERT_FALSE(recorder.complete(log, true, 999));

    const BusOffEvent ev2 = recorder.begin(2500, 2, 256, 12);
    TEST_ASSERT_EQUAL_UINT32(1500, ev2.sinceLastMs);
    TEST_ASSERT_TRUE(recorder.complete(log, false, 800));

    TEST_ASSERT_EQUAL_UINT32(2, log.count());
    TEST_ASSERT_EQUAL_UINT32(1, log.at(0).seqNum);
    TEST_ASSERT_EQUAL_UINT8(1, log.at(0).recovered);
    TEST_ASSERT_EQUAL_UINT32(2, log.at(1).seqNum);
    TEST_ASSERT_EQUAL_UINT8(0, log.at(1).recovered);
    TEST_ASSERT_EQUAL_UINT32(800, log.at(1).recoveryDurMs);
}

void test_unexpected_reset_restores_previous_rtc_snapshot()
{
    rtcCanSnapshot = {};
    rtcCanSnapshot.magic = kRtcCanSnapshotMagic;
    rtcCanSnapshot.schema = kRtcCanSnapshotSchema;
    rtcCanSnapshot.bootCount = 7;
    rtcCanSnapshot.uptimeMs = 12345;
    rtcCanSnapshot.aEflg = 0x80;
    rtcCanSnapshot.aRxOverrunCount = 2;

    bootDiagnosticsBegin(6); // TASK_WDT

    TEST_ASSERT_TRUE((bool)bootDiagnostics.unexpectedReset);
    TEST_ASSERT_TRUE((bool)bootDiagnostics.previousSnapshotValid);
    TEST_ASSERT_EQUAL_UINT32(8, (uint32_t)bootDiagnostics.rtcBootCount);
    TEST_ASSERT_EQUAL_UINT32(12345, bootDiagnostics.previous.uptimeMs);
    TEST_ASSERT_EQUAL_HEX8(0x80, bootDiagnostics.previous.aEflg);
}

void test_a_safety_first_overrun_recovers_and_second_latches()
{
    aChannelDiag.framesReceivedTotal = 1000;
    TEST_ASSERT_FALSE(aSafetyRecordOverrun(100));
    TEST_ASSERT_TRUE((bool)aChannelDiag.aSafetyHold);
    TEST_ASSERT_FALSE((bool)aChannelDiag.aSafetyLatched);

    aChannelDiag.mcpEflg = 0;
    aChannelDiag.framesReceivedTotal = 1100;
    aChannelDiag.lastFrameRxMs = 2100;
    TEST_ASSERT_TRUE(aSafetyTryRecover(2100));
    TEST_ASSERT_FALSE((bool)aChannelDiag.aSafetyHold);
    TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)aChannelDiag.aSafetyRecoveryCount);

    TEST_ASSERT_TRUE(aSafetyRecordOverrun(5000));
    TEST_ASSERT_TRUE((bool)aChannelDiag.aSafetyLatched);
    TEST_ASSERT_FALSE((bool)aChannelDiag.aSafetyHold);
    TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)aChannelDiag.aSafetyLatchCount);
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
    RUN_TEST(test_runtime_bypass_tlssc_overrides_when_bit_clear);
    RUN_TEST(test_runtime_bypass_tlssc_off_reads_frame);
    RUN_TEST(test_runtime_bypass_tlssc_off_still_reads_real_bit);
    RUN_TEST(test_runtime_defaults_match_build_configuration);
    RUN_TEST(test_user_marker_names_are_generic);
    RUN_TEST(test_feature_state_detail_includes_nag_ap_only_and_summon_condition);
    RUN_TEST(test_feature_activity_detail_maps_each_feature);
    RUN_TEST(test_monitor_protocol_is_compact_versioned_and_read_only);
    RUN_TEST(test_busoff_recorder_keeps_entry_pending_until_recovery_result);
    RUN_TEST(test_busoff_recorder_writes_one_row_per_success_or_failure);
    RUN_TEST(test_unexpected_reset_restores_previous_rtc_snapshot);
    RUN_TEST(test_a_safety_first_overrun_recovers_and_second_latches);

    return UNITY_END();
}
