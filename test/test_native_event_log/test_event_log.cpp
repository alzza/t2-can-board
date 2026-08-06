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
}

void test_a_tx_failure_detail_preserves_source_phase_buffer_and_controller_bits()
{
    const uint32_t detail = eventATxFailureDetail(
        1, true, 2, 0x30, 0); // SUMMON, 완료 폴링, TXB2, TXERR+MLOA

    TEST_ASSERT_EQUAL_UINT8(1, detail & 0x03U);
    TEST_ASSERT_TRUE((detail & (1U << 2)) != 0);
    TEST_ASSERT_EQUAL_UINT8(2, (detail >> 3) & 0x03U);
    TEST_ASSERT_EQUAL_HEX8(0x30, (detail >> 8) & 0xFFU);
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
    RUN_TEST(test_a_tx_failure_detail_preserves_source_phase_buffer_and_controller_bits);
    RUN_TEST(test_noisy_event_is_coalesced_with_first_last_time_and_count);
    RUN_TEST(test_aggregate_window_or_detail_change_creates_new_record);
    RUN_TEST(test_rx_overrun_aggregation_keeps_largest_loop_gap);
    RUN_TEST(test_non_noisy_events_preserve_each_occurrence_and_track_overwrite);
    return UNITY_END();
}
