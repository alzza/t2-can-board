// 제거된 A채널 실험 송신과 현재 HW3 핵심 경로를 검증하는 회귀 테스트
#include <unity.h>
#include "can_helpers.h"
#include "handlers.h"
#include "drivers/mock_driver.h"

static MockDriver mock;
static HW3Handler handler;

static void resetSummonState()
{
    summonGateDiag.parked = true;
    summonGateDiag.summoning = false;
    summonGateDiag.acaActive = false;
    summonGateDiag.sprSeen = false;
    summonGateDiag.last280Ms = 0;
    summonGateDiag.mux1Received = 0;
    summonGateDiag.txOk = 0;
    summonGateDiag.txFail = 0;
    summonGateDiag.blocked = 0;
}

void setUp()
{
    mock.reset();
    handler = HW3Handler();
    summonUnlockRuntime = true;
    summonConditionLimitRuntime = true;
    tsllcRuntime = true;
    aChannelTxRuntime = true;
    aTxGuardRuntime = false;
    signalObserverCount = 0;
    resetSummonState();
}

void tearDown() {}

void test_hw3_filter_is_exact_ino_five_ids_without_default_experiment()
{
    const uint32_t *ids = handler.filterIds();
    TEST_ASSERT_EQUAL_UINT8(5, handler.filterIdCount());
    TEST_ASSERT_EQUAL_UINT32(280, ids[0]);
    TEST_ASSERT_EQUAL_UINT32(390, ids[1]);
    TEST_ASSERT_EQUAL_UINT32(921, ids[2]);
    TEST_ASSERT_EQUAL_UINT32(1016, ids[3]);
    TEST_ASSERT_EQUAL_UINT32(1021, ids[4]);
}

void test_id1016_updates_summon_spr_without_transmitting()
{
    CanFrame frame = {.id = 1016, .dlc = 8};
    frame.data[3] = 0x40;
    handler.handleMessage(frame, mock);

    TEST_ASSERT_TRUE(summonGateDiag.sprSeen);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_removed_id1001_experiment_is_ignored()
{
    CanFrame frame = {.id = 1001, .dlc = 8};
    setBit(frame, 28, false);
    handler.handleMessage(frame, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_tsllc_mux0_sets_only_validated_bits()
{
    CanFrame frame = {.id = 1021, .dlc = 8};
    handler.handleMessage(frame, mock);

    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_TRUE((mock.sent[0].data[4] >> 6) & 0x01U);
    TEST_ASSERT_TRUE((mock.sent[0].data[4] >> 7) & 0x01U);
    TEST_ASSERT_FALSE((mock.sent[0].data[5] >> 6) & 0x01U);
}

void test_summon_mux1_sets_hw3_bit46_and_clears_bit19()
{
    CanFrame frame = {.id = 1021, .dlc = 8};
    frame.data[0] = 1;
    setBit(frame, 19, true);
    handler.handleMessage(frame, mock);

    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_FALSE((mock.sent[0].data[2] >> 3) & 0x01U);
    TEST_ASSERT_TRUE((mock.sent[0].data[5] >> 6) & 0x01U);
    TEST_ASSERT_FALSE((mock.sent[0].data[5] >> 7) & 0x01U);
}

void test_short_1021_frame_never_transmits()
{
    CanFrame frame = {.id = 1021, .dlc = 4};
    frame.data[0] = 1;
    handler.handleMessage(frame, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_hw3_filter_is_exact_ino_five_ids_without_default_experiment);
    RUN_TEST(test_id1016_updates_summon_spr_without_transmitting);
    RUN_TEST(test_removed_id1001_experiment_is_ignored);
    RUN_TEST(test_tsllc_mux0_sets_only_validated_bits);
    RUN_TEST(test_summon_mux1_sets_hw3_bit46_and_clears_bit19);
    RUN_TEST(test_short_1021_frame_never_transmits);
    return UNITY_END();
}
