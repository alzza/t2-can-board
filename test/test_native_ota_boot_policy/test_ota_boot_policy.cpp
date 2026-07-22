// OTA 부팅 상태표의 CAN 허용·차단 정책을 전체 범위로 회귀 검증한다.
#include <unity.h>
#include "ota_boot_policy.h"

void setUp() {}
void tearDown() {}

void test_pending_zero_is_normal_can_boot()
{
    const OtaBootPolicy policy = otaBootPolicy(0, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::Normal), static_cast<uint8_t>(policy.action));
    TEST_ASSERT_EQUAL_UINT8(0, policy.nextPending);
    TEST_ASSERT_TRUE(policy.canStartAfterAction);
}

void test_pending_one_requires_safe_reset_before_can()
{
    const OtaBootPolicy policy = otaBootPolicy(1, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::FirstBootSafeReset), static_cast<uint8_t>(policy.action));
    TEST_ASSERT_EQUAL_UINT8(2, policy.nextPending);
    TEST_ASSERT_TRUE(policy.canStartAfterAction);
}

void test_pending_two_rolls_back_without_starting_can()
{
    const OtaBootPolicy policy = otaBootPolicy(2, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::Rollback), static_cast<uint8_t>(policy.action));
    TEST_ASSERT_EQUAL_UINT8(3, policy.nextPending);
    TEST_ASSERT_FALSE(policy.canStartAfterAction);
}

void test_pending_three_verifies_recovery_partition()
{
    const OtaBootPolicy policy = otaBootPolicy(3, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::RecoveryVerification), static_cast<uint8_t>(policy.action));
    TEST_ASSERT_EQUAL_UINT8(4, policy.nextPending);
    TEST_ASSERT_TRUE(policy.canStartAfterAction);
}

void test_pending_four_and_five_are_recovery_only()
{
    const OtaBootPolicy pending4 = otaBootPolicy(4, false);
    const OtaBootPolicy pending5 = otaBootPolicy(5, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::RecoveryOnly), static_cast<uint8_t>(pending4.action));
    TEST_ASSERT_EQUAL_UINT8(5, pending4.nextPending);
    TEST_ASSERT_FALSE(pending4.canStartAfterAction);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::RecoveryOnly), static_cast<uint8_t>(pending5.action));
    TEST_ASSERT_EQUAL_UINT8(5, pending5.nextPending);
    TEST_ASSERT_FALSE(pending5.canStartAfterAction);
}

void test_usb_partition_mismatch_clears_only_pending_one_or_two()
{
    for (uint8_t pending = 1; pending <= 2; ++pending) {
        const OtaBootPolicy policy = otaBootPolicy(pending, true);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::ClearStaleOta), static_cast<uint8_t>(policy.action));
        TEST_ASSERT_EQUAL_UINT8(0, policy.nextPending);
        TEST_ASSERT_TRUE(policy.canStartAfterAction);
    }
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::Normal),
                            static_cast<uint8_t>(otaBootPolicy(0, true).action));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::RecoveryVerification),
                            static_cast<uint8_t>(otaBootPolicy(3, true).action));
}

void test_every_unsupported_pending_value_fails_closed()
{
    for (uint16_t pending = 6; pending <= 255; ++pending) {
        const OtaBootPolicy policy = otaBootPolicy(static_cast<uint8_t>(pending), false);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaBootAction::FailClosed), static_cast<uint8_t>(policy.action));
        TEST_ASSERT_FALSE(policy.canStartAfterAction);
    }
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_pending_zero_is_normal_can_boot);
    RUN_TEST(test_pending_one_requires_safe_reset_before_can);
    RUN_TEST(test_pending_two_rolls_back_without_starting_can);
    RUN_TEST(test_pending_three_verifies_recovery_partition);
    RUN_TEST(test_pending_four_and_five_are_recovery_only);
    RUN_TEST(test_usb_partition_mismatch_clears_only_pending_one_or_two);
    RUN_TEST(test_every_unsupported_pending_value_fails_closed);
    return UNITY_END();
}
