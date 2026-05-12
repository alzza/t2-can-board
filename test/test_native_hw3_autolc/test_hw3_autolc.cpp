// 0x293 UI_chassisControl AutoLC 주입의 counter/checksum 보정을 검증하는 native 테스트
#include <unity.h>
#include "can_frame_types.h"
#include "can_helpers.h"
#include "handlers.h"
#include "drivers/mock_driver.h"

static MockDriver mock;
static HW3Handler handler;

void setUp()
{
    mock.reset();
    handler = HW3Handler();
    enhancedAutopilotRuntime = true;
    uiAutoLaneChangeEnableRuntime = true;
    aChannelTxRuntime = true;
    aChannelDiag.autoLaneChangeModifiedCount = 0;
    aChannelDiag.autoLaneChangeSkipCount = 0;
}

void tearDown()
{
    uiAutoLaneChangeEnableRuntime = false;
}

void test_hw3_autolc_updates_counter_and_checksum()
{
    CanFrame f = {.id = 659, .dlc = 8};
    f.data[3] = 0x00;
    f.data[6] = 0xA5;

    handler.handleMessage(f, mock);

    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_EQUAL_HEX8(0x01, mock.sent[0].data[3] & 0x03);
    TEST_ASSERT_EQUAL_HEX8(0xB5, mock.sent[0].data[6]);
    TEST_ASSERT_EQUAL_HEX8(computeTeslaChecksum(mock.sent[0]), mock.sent[0].data[7]);
    TEST_ASSERT_EQUAL_UINT32(1, aChannelDiag.autoLaneChangeModifiedCount);
}

void test_hw3_autolc_runtime_off_passes_without_send()
{
    uiAutoLaneChangeEnableRuntime = false;
    CanFrame f = {.id = 659, .dlc = 8};
    f.data[3] = 0x00;
    f.data[6] = 0xA5;
    f.data[7] = 0xEE;

    handler.handleMessage(f, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0, aChannelDiag.autoLaneChangeModifiedCount);
    TEST_ASSERT_EQUAL_UINT32(0, aChannelDiag.autoLaneChangeSkipCount);
}

void test_hw3_autolc_skips_when_already_on()
{
    CanFrame f = {.id = 659, .dlc = 8};
    f.data[3] = 0x01;

    handler.handleMessage(f, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0, aChannelDiag.autoLaneChangeModifiedCount);
    TEST_ASSERT_EQUAL_UINT32(1, aChannelDiag.autoLaneChangeSkipCount);
}

void test_hw3_autolc_skips_short_frame()
{
    CanFrame f = {.id = 659, .dlc = 4};
    f.data[3] = 0x00;

    handler.handleMessage(f, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0, aChannelDiag.autoLaneChangeModifiedCount);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_hw3_autolc_updates_counter_and_checksum);
    RUN_TEST(test_hw3_autolc_runtime_off_passes_without_send);
    RUN_TEST(test_hw3_autolc_skips_when_already_on);
    RUN_TEST(test_hw3_autolc_skips_short_frame);
    return UNITY_END();
}