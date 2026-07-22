// Validated INO Summon Unlock gate port regression tests for the HW3 A-channel handler.
#include <unity.h>
#include "can_helpers.h"
#include "handlers.h"
#include "drivers/mock_driver.h"

static MockDriver mock;
static HW3Handler handler;

static void resetSummonState()
{
    summonGateDiag.apActive = false;
    summonGateDiag.parked = true;
    summonGateDiag.summoning = false;
    summonGateDiag.acaActive = false;
    summonGateDiag.sprSeen = false;
    summonGateDiag.last280Ms = 0;
    summonGateDiag.frames280 = 0;
    summonGateDiag.frames390 = 0;
    summonGateDiag.frames921 = 0;
    summonGateDiag.frames1016 = 0;
    summonGateDiag.mux1Received = 0;
    summonGateDiag.txOk = 0;
    summonGateDiag.txFail = 0;
    summonGateDiag.blocked = 0;
}

static CanFrame mux1Frame()
{
    CanFrame frame = {.id = 1021, .dlc = 8};
    frame.data[0] = 1;
    setBit(frame, 19, true);
    return frame;
}

static CanFrame mux0Frame()
{
    CanFrame frame = {.id = 1021, .dlc = 8};
    frame.data[0] = 0;
    return frame;
}

static CanFrame frame280(uint8_t gear, bool aca)
{
    CanFrame frame = {.id = 280, .dlc = 8};
    frame.data[2] = static_cast<uint8_t>((gear & 0x07U) << 5);
    if (aca) frame.data[6] |= 0x04U;
    return frame;
}

void setUp()
{
    mock.reset();
    handler = HW3Handler();
    handler.enablePrint = false;
    summonUnlockRuntime = true;
    tsllcRuntime = true;
    aChannelTxRuntime = true;
    aTxGuardRuntime = false;
    resetSummonState();
}

void tearDown() {}

void test_filter_contains_validated_gate_ids_and_no_659()
{
    const uint32_t *ids = handler.filterIds();
    const uint8_t count = handler.filterIdCount();
    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(5, count);
    TEST_ASSERT_EQUAL_UINT32(280, ids[0]);
    TEST_ASSERT_EQUAL_UINT32(390, ids[1]);
    TEST_ASSERT_EQUAL_UINT32(921, ids[2]);
    TEST_ASSERT_EQUAL_UINT32(1016, ids[3]);
    TEST_ASSERT_EQUAL_UINT32(1021, ids[4]);
    for (uint8_t i = 0; i < count; ++i) TEST_ASSERT_NOT_EQUAL(659, ids[i]);
}

void test_boot_parked_gate_injects_hw3_bit46_only()
{
    CanFrame frame = mux1Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_FALSE((mock.sent[0].data[2] >> 3) & 0x01U);
    TEST_ASSERT_TRUE((mock.sent[0].data[5] >> 6) & 0x01U);
    TEST_ASSERT_FALSE((mock.sent[0].data[5] >> 7) & 0x01U);
    TEST_ASSERT_EQUAL_UINT32(1, summonGateDiag.txOk);
    TEST_ASSERT_EQUAL_UINT32(1, summonGateDiag.mux1Received);
}

void test_disabled_toggle_blocks_even_when_parked()
{
    summonUnlockRuntime = false;
    CanFrame frame = mux1Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0, summonGateDiag.mux1Received);
}

void test_drive_without_summon_blocks_mux1_injection()
{
    CanFrame drive = frame280(4, false);
    handler.handleMessage(drive, mock);
    CanFrame frame = mux1Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_FALSE(summonGateDiag.parked);
    TEST_ASSERT_FALSE(summonGateDiag.summoning);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(1, summonGateDiag.blocked);
    TEST_ASSERT_EQUAL_UINT32(0, summonGateDiag.mux1Received);
}

void test_aca_plus_spr_opens_summoning_gate_while_in_drive()
{
    CanFrame driveAca = frame280(4, true);
    handler.handleMessage(driveAca, mock);
    CanFrame spr = {.id = 1016, .dlc = 8};
    spr.data[3] = 0x40;
    handler.handleMessage(spr, mock);
    CanFrame frame = mux1Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_TRUE(summonGateDiag.sprSeen);
    TEST_ASSERT_TRUE(summonGateDiag.summoning);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_TRUE((mock.sent[0].data[5] >> 6) & 0x01U);
}

void test_aca_falling_edge_clears_spr_and_closes_drive_gate()
{
    CanFrame driveAca = frame280(4, true);
    handler.handleMessage(driveAca, mock);
    CanFrame spr = {.id = 1016, .dlc = 8};
    spr.data[3] = 0xB0;
    handler.handleMessage(spr, mock);
    CanFrame driveNoAca = frame280(4, false);
    handler.handleMessage(driveNoAca, mock);
    CanFrame frame = mux1Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_FALSE(summonGateDiag.sprSeen);
    TEST_ASSERT_FALSE(summonGateDiag.summoning);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_ap_status_is_diagnostic_only_and_does_not_open_gate()
{
    CanFrame drive = frame280(4, false);
    handler.handleMessage(drive, mock);
    CanFrame ap = {.id = 921, .dlc = 8};
    ap.data[0] = 3;
    handler.handleMessage(ap, mock);
    CanFrame frame = mux1Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_TRUE(summonGateDiag.apActive);
    TEST_ASSERT_FALSE(summonGateOpen());
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_390_fallback_and_280_watchdog_match_ino()
{
    summonGateDiag.parked = false;
    summonGateDiag.last280Ms = 100;
    CanFrame park390 = {.id = 390, .dlc = 8};
    park390.data[2] = 1U << 5;
    summonHandle390(park390, 200);
    TEST_ASSERT_FALSE(summonGateDiag.parked);

    summonHandle390(park390, 5201);
    TEST_ASSERT_TRUE(summonGateDiag.parked);
    summonGateDiag.parked = false;
    summonGateMaintain(5201);
    TEST_ASSERT_TRUE(summonGateDiag.parked);
}

void test_removed_id_659_is_ignored()
{
    CanFrame frame = {.id = 659, .dlc = 8};
    handler.handleMessage(frame, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_tsllc_mux0_remains_active_when_summon_gate_is_closed()
{
    CanFrame drive = frame280(4, false);
    handler.handleMessage(drive, mock);
    CanFrame frame = mux0Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_FALSE(summonGateOpen());
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_TRUE((mock.sent[0].data[4] >> 6) & 0x01U);
    TEST_ASSERT_TRUE((mock.sent[0].data[4] >> 7) & 0x01U);
    TEST_ASSERT_FALSE((mock.sent[0].data[5] >> 6) & 0x01U);
    TEST_ASSERT_EQUAL_UINT32(0, summonGateDiag.mux1Received);
}

void test_a_channel_tx_master_blocks_tsllc_and_summon()
{
    aChannelTxRuntime = false;
    CanFrame tsllc = mux0Frame();
    handler.handleMessage(tsllc, mock);
    CanFrame summon = mux1Frame();
    handler.handleMessage(summon, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(1, summonGateDiag.mux1Received);
    TEST_ASSERT_EQUAL_UINT32(0, summonGateDiag.txOk);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_filter_contains_validated_gate_ids_and_no_659);
    RUN_TEST(test_boot_parked_gate_injects_hw3_bit46_only);
    RUN_TEST(test_disabled_toggle_blocks_even_when_parked);
    RUN_TEST(test_drive_without_summon_blocks_mux1_injection);
    RUN_TEST(test_aca_plus_spr_opens_summoning_gate_while_in_drive);
    RUN_TEST(test_aca_falling_edge_clears_spr_and_closes_drive_gate);
    RUN_TEST(test_ap_status_is_diagnostic_only_and_does_not_open_gate);
    RUN_TEST(test_390_fallback_and_280_watchdog_match_ino);
    RUN_TEST(test_removed_id_659_is_ignored);
    RUN_TEST(test_tsllc_mux0_remains_active_when_summon_gate_is_closed);
    RUN_TEST(test_a_channel_tx_master_blocks_tsllc_and_summon);
    return UNITY_END();
}
