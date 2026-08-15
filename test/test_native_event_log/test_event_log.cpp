#include <unity.h>

#include "event_log.h"

void setUp()
{
    eventLogReset();
}

void tearDown() {}

void test_event_channel_and_severity_are_explicit()
{
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_A_RX_OVERRUN));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_B, eventChannel(EV_ALERT_ARB_LOST));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_AB, eventChannel(EV_USER_MARK));
    TEST_ASSERT_EQUAL_STRING("A", eventChannelName(EV_CH_A));
    TEST_ASSERT_EQUAL_STRING("INFO", eventSeverityName(eventSeverity(EV_ALERT_ARB_LOST, 0)));
    TEST_ASSERT_EQUAL_STRING("WARN", eventSeverityName(eventSeverity(EV_A_RX_OVERRUN, 0x80)));
    TEST_ASSERT_EQUAL_STRING("ERROR", eventSeverityName(eventSeverity(EV_A_EFLG_SET, 0x20)));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_A_TX_FAILURE));
    TEST_ASSERT_EQUAL_STRING("WARN", eventSeverityName(eventSeverity(EV_A_TX_FAILURE, 0)));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_SUMMONING_STATE));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_B, eventChannel(EV_NAG_INJECTION_SESSION));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_SUMMON_UNLOCK_ACTIVITY));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_B, eventChannel(EV_NAG_GATE_STATE));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_A_TX_QUALITY));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_SUMMON_TX_SESSION));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_SUMMON_RETRY_SESSION));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_SUMMON_TX_TIMING));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_A, eventChannel(EV_SUMMON_POLICY_STATE));
    TEST_ASSERT_EQUAL_STRING("WARN", eventSeverityName(eventSeverity(EV_A_TX_QUALITY, 0)));
    TEST_ASSERT_EQUAL_UINT8(EV_CH_B, eventChannel(EV_B_BUS_ERR_SNAPSHOT));
}

void test_summon_policy_detail_preserves_reason_and_vehicle_context()
{
    const uint32_t detail = eventSummonPolicyStateDetail(
        SUMMON_SESSION_GEAR_CONFLICT, false, true, true, 4, 2, 734);
    TEST_ASSERT_EQUAL_UINT8(SUMMON_SESSION_GEAR_CONFLICT, detail & 0x0FU);
    TEST_ASSERT_FALSE((detail & (1U << 4)) != 0);
    TEST_ASSERT_TRUE((detail & (1U << 5)) != 0);
    TEST_ASSERT_TRUE((detail & (1U << 6)) != 0);
    TEST_ASSERT_EQUAL_UINT8(4, (detail >> 7) & 0x07U);
    TEST_ASSERT_EQUAL_UINT8(2, (detail >> 10) & 0x07U);
    TEST_ASSERT_EQUAL_UINT16(734, (detail >> 13) & 0x0FFFU);
}

void test_summon_retry_session_details_preserve_results_and_timing()
{
    const uint32_t tx = eventSummonTxSessionDetail(101, 22, 3, 4);
    TEST_ASSERT_EQUAL_UINT8(101, tx & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(22, (tx >> 8) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(3, (tx >> 16) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(4, (tx >> 24) & 0xFFU);

    const uint32_t retry = eventSummonRetrySessionDetail(22, 18, 2, 2);
    TEST_ASSERT_EQUAL_UINT8(22, retry & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(18, (retry >> 8) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(2, (retry >> 16) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(2, (retry >> 24) & 0xFFU);

    const uint32_t timing = eventSummonTxTimingDetail(7, 1234, 9);
    TEST_ASSERT_EQUAL_UINT8(7, timing & 0xFFU);
    TEST_ASSERT_EQUAL_UINT16(1234, (timing >> 8) & 0xFFFFU);
    TEST_ASSERT_EQUAL_UINT8(9, (timing >> 24) & 0xFFU);
}

void test_quality_and_bus_error_snapshot_details_preserve_context()
{
    const uint32_t quality = eventATxQualityDetail(2, 11, 14, 1);
    TEST_ASSERT_EQUAL_UINT8(2, quality & 0x03U);
    TEST_ASSERT_EQUAL_UINT8(11, (quality >> 2) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(14, (quality >> 10) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(1, (quality >> 18) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(53, eventATxQualityMloaPercent(11, 14, 1));
    TEST_ASSERT_TRUE(eventATxQualityIsWarning(11, 14, 1));
    TEST_ASSERT_FALSE(eventATxQualityIsWarning(3, 4, 0)); // 표본 수가 10 미만
    TEST_ASSERT_EQUAL_STRING("WARN", eventSeverityName(eventSeverity(EV_A_TX_QUALITY, quality)));
    const uint32_t normalContention = eventATxQualityDetail(2, 11, 14, 0);
    const uint32_t noCompletion = eventATxQualityDetail(2, 0, 10, 0);
    TEST_ASSERT_EQUAL_STRING("INFO", eventSeverityName(eventSeverity(EV_A_TX_QUALITY, normalContention)));
    TEST_ASSERT_EQUAL_STRING("WARN", eventSeverityName(eventSeverity(EV_A_TX_QUALITY, noCompletion)));

    const uint32_t snapshot = eventBBusErrSnapshotDetail(1, 2, 3, 2, true,
                                                           true, 1, 9, 4);
    TEST_ASSERT_EQUAL_UINT8(1, snapshot & 0x03U);
    TEST_ASSERT_EQUAL_UINT8(2, (snapshot >> 2) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(3, (snapshot >> 10) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(2, (snapshot >> 18) & 0x03U);
    TEST_ASSERT_TRUE((snapshot & (1U << 20)) != 0);
    TEST_ASSERT_TRUE((snapshot & (1U << 21)) != 0);
    TEST_ASSERT_EQUAL_UINT8(1, (snapshot >> 22) & 0x03U);
    TEST_ASSERT_EQUAL_UINT8(9, (snapshot >> 24) & 0x0FU);
    TEST_ASSERT_EQUAL_UINT8(4, (snapshot >> 28) & 0x0FU);
}

void test_auto_session_details_preserve_start_end_context()
{
    const uint32_t summon = eventSummoningStateDetail(
        false, false, false, false, false, 3, 12, 2, 7);
    TEST_ASSERT_FALSE((summon & 1U) != 0);
    TEST_ASSERT_EQUAL_UINT8(3, (summon >> 5) & 0x07U);
    TEST_ASSERT_EQUAL_UINT8(12, (summon >> 8) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(2, (summon >> 16) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(7, (summon >> 24) & 0xFFU);

    const uint32_t nag = eventNagInjectionSessionDetail(true, 2, true, 2, 9, 1234);
    TEST_ASSERT_TRUE((nag & 1U) != 0);
    TEST_ASSERT_EQUAL_UINT8(2, (nag >> 1) & 0x03U);
    TEST_ASSERT_TRUE((nag & (1U << 3)) != 0);
    TEST_ASSERT_EQUAL_UINT16(1234, (nag >> 16) & 0xFFFFU);

    const uint32_t unlock = eventSummonUnlockActivityDetail(
        false, true, false, false, 2, 10, 8, 2);
    TEST_ASSERT_FALSE((unlock & 1U) != 0);
    TEST_ASSERT_TRUE((unlock & (1U << 1)) != 0);
    TEST_ASSERT_EQUAL_UINT8(2, (unlock >> 4) & 0x07U);
    TEST_ASSERT_EQUAL_UINT8(10, (unlock >> 8) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(8, (unlock >> 16) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(2, (unlock >> 24) & 0xFFU);

    const uint32_t gate = eventNagGateStateDetail(
        kNagDecisionHandsOn, 2, true, 3, 2, 923, 2);
    TEST_ASSERT_EQUAL_UINT8(kNagDecisionHandsOn, gate & 0x0FU);
    TEST_ASSERT_EQUAL_UINT8(2, (gate >> 4) & 0x03U);
    TEST_ASSERT_TRUE((gate & (1U << 6)) != 0);
    TEST_ASSERT_EQUAL_UINT8(3, (gate >> 7) & 0x03U);
    TEST_ASSERT_EQUAL_UINT16(923, (gate >> 13) & 0x07FFU);
    TEST_ASSERT_EQUAL_UINT8(2, (gate >> 24) & 0xFFU);
}

void test_a_tx_failure_detail_preserves_source_phase_buffer_and_controller_bits()
{
    const uint32_t detail = eventATxFailureDetail(
        1, true, 2, 0x30, 0); // SUMMON, 완료 폴링, TXB2, TXERR+MLOA

    TEST_ASSERT_EQUAL_UINT8(1, detail & 0x03U);
    TEST_ASSERT_TRUE((detail & (1U << 2)) != 0);
    TEST_ASSERT_EQUAL_UINT8(2, (detail >> 3) & 0x03U);
    TEST_ASSERT_EQUAL_HEX8(0x30, (detail >> 8) & 0xFFU);

    const uint32_t immediateDetail = eventATxFailureDetail(
        2, false, 1, 0x50, 4); // TSLLC, 즉시 결과, TXB1, TXERR+ABTF
    TEST_ASSERT_FALSE((immediateDetail & (1U << 2)) != 0);
    TEST_ASSERT_EQUAL_UINT8(1, (immediateDetail >> 3) & 0x03U);
    TEST_ASSERT_EQUAL_HEX8(0x50, (immediateDetail >> 8) & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(4, (immediateDetail >> 16) & 0xFFU);
}

void test_noisy_event_is_coalesced_with_first_last_time_and_count()
{
    eventLogPushAt(1000, EV_A_RX_OVERRUN, 0, 0, 0x80);
    eventLogPushAt(2000, EV_A_RX_OVERRUN, 0, 0, 0x80);

    TEST_ASSERT_EQUAL_UINT32(1, evtCount);
    TEST_ASSERT_EQUAL_UINT32(2, evtOccurrenceTotal);
    TEST_ASSERT_EQUAL_UINT32(1, evtCoalescedTotal);
    TEST_ASSERT_EQUAL_UINT32(1000, evtBuf[0].t_ms);
    TEST_ASSERT_EQUAL_UINT32(2000, evtBuf[0].last_ms);
    TEST_ASSERT_EQUAL_UINT32(2, evtBuf[0].occurrences);
}

void test_aggregate_window_or_detail_change_creates_new_record()
{
    eventLogPushAt(1000, EV_A_RX_OVERRUN, 0, 0, 0x80);
    eventLogPushAt(32000, EV_A_RX_OVERRUN, 0, 0, 0x80);
    eventLogPushAt(33000, EV_A_RX_OVERRUN, 0, 0, 0x40);

    TEST_ASSERT_EQUAL_UINT32(3, evtCount);
    TEST_ASSERT_EQUAL_UINT32(3, evtOccurrenceTotal);
    TEST_ASSERT_EQUAL_UINT32(0, evtCoalescedTotal);
}

void test_rx_overrun_aggregation_keeps_largest_loop_gap()
{
    eventLogPushAt(1000, EV_A_RX_OVERRUN, 0, 0, 0x80U | (1200U << 8));
    eventLogPushAt(2000, EV_A_RX_OVERRUN, 0, 0, 0x80U | (4200U << 8));

    TEST_ASSERT_EQUAL_UINT32(1, evtCount);
    TEST_ASSERT_EQUAL_UINT32(2, evtBuf[0].occurrences);
    TEST_ASSERT_EQUAL_UINT32(4200, evtBuf[0].detail >> 8);
    TEST_ASSERT_EQUAL_HEX8(0x80, evtBuf[0].detail & 0xFFU);
}

void test_nag_gate_chatter_is_coalesced_and_keeps_latest_state()
{
    const uint32_t handsOn = eventNagGateStateDetail(
        kNagDecisionHandsOn, 2, false, 1, 1, 923, 2);
    const uint32_t apBlocked = eventNagGateStateDetail(
        kNagDecisionApBlocked, 2, false, 0, 1, 923, 2);

    eventLogPushAt(1000, EV_NAG_GATE_STATE, 0, 0, handsOn);
    eventLogPushAt(1200, EV_NAG_GATE_STATE, 0, 0, apBlocked);
    eventLogPushAt(1400, EV_NAG_GATE_STATE, 0, 0, handsOn);

    TEST_ASSERT_EQUAL_UINT32(1, evtCount);
    TEST_ASSERT_EQUAL_UINT32(3, evtOccurrenceTotal);
    TEST_ASSERT_EQUAL_UINT32(2, evtCoalescedTotal);
    TEST_ASSERT_EQUAL_UINT32(1000, evtBuf[0].t_ms);
    TEST_ASSERT_EQUAL_UINT32(1400, evtBuf[0].last_ms);
    TEST_ASSERT_EQUAL_UINT32(3, evtBuf[0].occurrences);
    TEST_ASSERT_EQUAL_UINT32(handsOn, evtBuf[0].detail);
}

void test_quality_warning_is_coalesced_per_feature_and_keeps_latest_window()
{
    const uint32_t summonFirst = eventATxQualityDetail(1, 10, 12, 0);
    const uint32_t summonLast = eventATxQualityDetail(1, 8, 14, 0);
    const uint32_t tsllc = eventATxQualityDetail(2, 10, 11, 0);

    eventLogPushAt(1000, EV_A_TX_QUALITY, 0, 0, summonFirst);
    eventLogPushAt(6000, EV_A_TX_QUALITY, 0, 0, summonLast);
    eventLogPushAt(7000, EV_A_TX_QUALITY, 0, 0, tsllc);

    TEST_ASSERT_EQUAL_UINT32(2, evtCount);
    TEST_ASSERT_EQUAL_UINT32(3, evtOccurrenceTotal);
    TEST_ASSERT_EQUAL_UINT32(1, evtCoalescedTotal);
    TEST_ASSERT_EQUAL_UINT32(summonLast, evtBuf[0].detail);
    TEST_ASSERT_EQUAL_UINT32(2, evtBuf[0].occurrences);
    TEST_ASSERT_EQUAL_UINT32(tsllc, evtBuf[1].detail);
}

void test_quality_info_does_not_hide_warning_for_same_feature()
{
    const uint32_t info = eventATxQualityDetail(2, 5, 6, 0);
    const uint32_t warning = eventATxQualityDetail(2, 0, 10, 0);

    eventLogPushAt(1000, EV_A_TX_QUALITY, 0, 0, info);
    eventLogPushAt(2000, EV_A_TX_QUALITY, 0, 0, warning);

    TEST_ASSERT_EQUAL_UINT32(2, evtCount);
    TEST_ASSERT_EQUAL_STRING("INFO",
        eventSeverityName(eventSeverity(evtBuf[0].type, evtBuf[0].detail)));
    TEST_ASSERT_EQUAL_STRING("WARN",
        eventSeverityName(eventSeverity(evtBuf[1].type, evtBuf[1].detail)));
}

void test_non_noisy_events_preserve_each_occurrence_and_track_overwrite()
{
    for (uint32_t i = 0; i < EVT_CAP + 3; ++i) {
        eventLogPushAt(i, EV_USER_MARK, 0, 0, i & 1U);
    }

    TEST_ASSERT_EQUAL_UINT32(EVT_CAP, evtCount);
    TEST_ASSERT_EQUAL_UINT32(EVT_CAP + 3, evtOccurrenceTotal);
    TEST_ASSERT_EQUAL_UINT32(3, evtOverwrittenTotal);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_event_channel_and_severity_are_explicit);
    RUN_TEST(test_quality_and_bus_error_snapshot_details_preserve_context);
    RUN_TEST(test_summon_retry_session_details_preserve_results_and_timing);
    RUN_TEST(test_summon_policy_detail_preserves_reason_and_vehicle_context);
    RUN_TEST(test_a_tx_failure_detail_preserves_source_phase_buffer_and_controller_bits);
    RUN_TEST(test_auto_session_details_preserve_start_end_context);
    RUN_TEST(test_noisy_event_is_coalesced_with_first_last_time_and_count);
    RUN_TEST(test_aggregate_window_or_detail_change_creates_new_record);
    RUN_TEST(test_rx_overrun_aggregation_keeps_largest_loop_gap);
    RUN_TEST(test_nag_gate_chatter_is_coalesced_and_keeps_latest_state);
    RUN_TEST(test_quality_warning_is_coalesced_per_feature_and_keeps_latest_window);
    RUN_TEST(test_quality_info_does_not_hide_warning_for_same_feature);
    RUN_TEST(test_non_noisy_events_preserve_each_occurrence_and_track_overwrite);
    return UNITY_END();
}
