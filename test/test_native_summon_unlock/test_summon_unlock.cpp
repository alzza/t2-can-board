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
    summonGateDiag.apState = 0;
    summonGateDiag.apActiveSinceMs = 0;
    summonGateDiag.last921Ms = 0;
    summonGateDiag.parked = true;
    summonGateDiag.summoning = false;
    summonGateDiag.acaActive = false;
    summonGateDiag.sprSeen = false;
    summonGateDiag.sessionAllowed = false;
    summonGateDiag.sessionReason = SUMMON_SESSION_IDLE;
    summonGateDiag.diGear = 0;
    summonGateDiag.secondaryGear = 0;
    summonGateDiag.selfParkRequest = 0;
    summonGateDiag.vehicleSpeedRaw = kSummonSpeedSnaRaw;
    summonGateDiag.last280Ms = 0;
    summonGateDiag.last390Ms = 0;
    summonGateDiag.last599Ms = 0;
    summonGateDiag.last1016Ms = 0;
    summonGateDiag.sprConfirmedMs = 0;
    summonGateDiag.frames280 = 0;
    summonGateDiag.frames390 = 0;
    summonGateDiag.frames599 = 0;
    summonGateDiag.frames921 = 0;
    summonGateDiag.frames1016 = 0;
    summonGateDiag.mux1Received = 0;
    summonGateDiag.txOk = 0;
    summonGateDiag.txBusy = 0;
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

static void primeSummonValidationContext()
{
    CanFrame ap = {.id = 921, .dlc = 8};
    ap.data[0] = 0;
    handler.handleMessage(ap, mock);
}

static void primeTsllcReadyContext()
{
    const uint32_t now = millis();
    aCanStartedMs = now - kTsllcStartupDelayMs;
    aChannelDiag.framesReceivedTotal = kTsllcMinValidAFrames + 1U;
    summonGateDiag.apState = 3;
    summonGateDiag.apActive = true;
    summonGateDiag.last921Ms = now;
    summonGateDiag.apActiveSinceMs = now - kTsllcApStableRequiredMs;
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
    aMcpOneShotRuntime = true;
    aChannelDiag.aTxGuardUntilMs = 0;
    aChannelDiag.tsllcSuppressedSummoningCount = 0;
    aChannelDiag.aSummonSessionTsllcSuppressed = 0;
    aChannelDiag.aSummonSessionStartMs = 0;
    aChannelDiag.tsllcSkippedUnchanged = 0;
    aChannelDiag.framesReceivedTotal = 0;
    aCanStartedMs = 0;
    tsllcRearmRequired = false;
    aChannelDiag.aSafetyHold = false;
    aChannelDiag.aSafetyLatched = false;
    canTxInFlight = 0;
    canTxQuiesceCancel();
    resetSummonState();
}

void tearDown() {}

void test_filter_restores_five_ids_without_257_or_659()
{
    const uint32_t *ids = handler.filterIds();
    const uint8_t count = handler.filterIdCount();
    TEST_ASSERT_EQUAL_UINT8(5, count);
    TEST_ASSERT_EQUAL_UINT32(1021, ids[0]);
    TEST_ASSERT_EQUAL_UINT32(280, ids[1]);
    TEST_ASSERT_EQUAL_UINT32(921, ids[2]);
    TEST_ASSERT_EQUAL_UINT32(1016, ids[3]);
    TEST_ASSERT_EQUAL_UINT32(390, ids[4]);
    for (uint8_t i = 0; i < count; ++i) {
        TEST_ASSERT_NOT_EQUAL(599, ids[i]);
        TEST_ASSERT_NOT_EQUAL(659, ids[i]);
    }
}

void test_unknown_boot_state_blocks_mux1_until_park_is_received()
{
    CanFrame frame = mux1Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());

    CanFrame park = frame280(1, false);
    handler.handleMessage(park, mock);
    handler.handleMessage(frame, mock);

    TEST_ASSERT_EQUAL(1, mock.sent.size());
    // 신선한 P만 있는 경우에는 Summon bit46만 바꾸며, AP 근거가 없는
    // bit19(ECE R79)는 원본값을 그대로 보존한다.
    TEST_ASSERT_TRUE((mock.sent[0].data[2] >> 3) & 0x01U);
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
    primeSummonValidationContext();
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
    primeSummonValidationContext();
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

void test_ap_requires_one_second_stability_before_opening_gate()
{
    summonGateDiag.parked = false;
    summonGateDiag.apState = 3;
    summonGateDiag.apActive = true;
    summonGateDiag.apActiveSinceMs = 100;
    summonGateDiag.last921Ms = 1099;
    TEST_ASSERT_TRUE(summonGateDiag.apActive);
    TEST_ASSERT_FALSE(summonGateOpen(1099));
    TEST_ASSERT_TRUE(summonGateOpen(1100));
    TEST_ASSERT_EQUAL_STRING("AP_STABLE", summonGateReasonName(1100));
}

void test_ap_state_two_is_available_not_active()
{
    summonGateDiag.parked = false;
    CanFrame ap = {.id = 921, .dlc = 8};
    ap.data[0] = 2;
    summonHandle921(ap, 100);
    TEST_ASSERT_FALSE(summonGateDiag.apActive);
    TEST_ASSERT_FALSE(summonGateOpen(5000));
}

void test_condition_limit_off_cannot_bypass_fail_closed_gate()
{
    summonGateDiag.parked = false;
    summonConditionLimitRuntime = false;
    CanFrame frame = mux1Frame();
    handler.handleMessage(frame, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_STRING("STATE_UNKNOWN", summonGateReasonName(100));
}

void test_390_is_used_only_while_fresh_and_stale_state_is_cleared()
{
    summonGateDiag.parked = false;
    summonGateDiag.last280Ms = 100;
    CanFrame park390 = {.id = 390, .dlc = 8};
    park390.data[7] = 1U << 3;
    summonHandle390(park390, 200);
    TEST_ASSERT_FALSE(summonGateDiag.parked);

    summonHandle390(park390, 601);
    TEST_ASSERT_TRUE(summonGateDiag.parked);
    summonGateDiag.parked = false;
    summonGateMaintain(602);
    TEST_ASSERT_FALSE(summonGateDiag.parked);
}

void test_removed_id_659_is_ignored()
{
    CanFrame frame = {.id = 659, .dlc = 8};
    handler.handleMessage(frame, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
}

void test_tsllc_mux0_is_blocked_when_ap_is_inactive()
{
    CanFrame drive = frame280(4, false);
    handler.handleMessage(drive, mock);
    CanFrame frame = mux0Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_FALSE(summonGateOpen());
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_STARTUP_TIME,
                            (uint8_t)aChannelDiag.tsllcLastBlockReason);
    TEST_ASSERT_EQUAL_UINT32(0, summonGateDiag.mux1Received);
}

void test_tsllc_mux0_transmits_only_after_ap_and_startup_gates()
{
    primeTsllcReadyContext();
    CanFrame frame = mux0Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_TRUE((mock.sent[0].data[4] >> 6) & 0x01U);
    TEST_ASSERT_TRUE((mock.sent[0].data[4] >> 7) & 0x01U);
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_READY,
                            (uint8_t)aChannelDiag.tsllcLastBlockReason);
}

void test_tsllc_does_not_send_when_validated_bits_are_already_set()
{
    primeTsllcReadyContext();
    CanFrame frame = mux0Frame();
    setBit(frame, 38, true);
    setBit(frame, 39, true);
    handler.handleMessage(frame, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(1, aChannelDiag.tsllcSkippedUnchanged);
}

void test_tsllc_mux0_is_held_while_actual_summoning()
{
    const uint32_t now = millis();
    aCanStartedMs = now - kTsllcStartupDelayMs;
    aChannelDiag.framesReceivedTotal = kTsllcMinValidAFrames + 1U;
    primeSummonValidationContext();
    CanFrame driveAca = frame280(4, true);
    handler.handleMessage(driveAca, mock);
    CanFrame spr = {.id = 1016, .dlc = 8};
    spr.data[3] = 0x40;
    handler.handleMessage(spr, mock);
    CanFrame frame = mux0Frame();
    handler.handleMessage(frame, mock);

    TEST_ASSERT_TRUE((bool)summonGateDiag.summoning);
    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(1, aChannelDiag.tsllcSuppressedSummoningCount);
    TEST_ASSERT_EQUAL_UINT32(1, aChannelDiag.aSummonSessionTsllcSuppressed);
}

void test_tsllc_mux0_resumes_after_summoning_ends()
{
    primeSummonValidationContext();
    CanFrame driveAca = frame280(4, true);
    handler.handleMessage(driveAca, mock);
    CanFrame spr = {.id = 1016, .dlc = 8};
    spr.data[3] = 0x40;
    handler.handleMessage(spr, mock);
    CanFrame held = mux0Frame();
    handler.handleMessage(held, mock);
    TEST_ASSERT_EQUAL(0, mock.sent.size());

    CanFrame driveNoAca = frame280(4, false);
    handler.handleMessage(driveNoAca, mock);
    primeTsllcReadyContext();
    CanFrame resumed = mux0Frame();
    handler.handleMessage(resumed, mock);

    TEST_ASSERT_FALSE((bool)summonGateDiag.summoning);
    TEST_ASSERT_EQUAL(1, mock.sent.size());
    TEST_ASSERT_TRUE((mock.sent[0].data[4] >> 6) & 0x01U);
    TEST_ASSERT_TRUE((mock.sent[0].data[4] >> 7) & 0x01U);
}

void test_summon_retry_policy_requires_actual_summoning_and_safety_gates()
{
    summonGateDiag.summoning = false;
    TEST_ASSERT_FALSE(summonRetryPolicyAllowed(1000));

    summonGateDiag.summoning = true;
    summonGateDiag.acaActive = true;
    summonGateDiag.sprSeen = true;
    summonGateDiag.last280Ms = 1000;
    summonGateDiag.last1016Ms = 1000;
    TEST_ASSERT_TRUE(summonRetryPolicyAllowed(1000));

    aTxGuardRuntime = true;
    aChannelDiag.aTxGuardUntilMs = 2000;
    TEST_ASSERT_FALSE(summonRetryPolicyAllowed(1000));
    aTxGuardRuntime = false;

    canTxQuiesceBegin();
    TEST_ASSERT_FALSE(summonRetryPolicyAllowed(1000));
    canTxQuiesceCancel();

    aMcpOneShotRuntime = false;
    TEST_ASSERT_FALSE(summonRetryPolicyAllowed(1000));
}

void test_tsllc_rearm_and_a_safety_gates_fail_closed()
{
    primeTsllcReadyContext();
    const uint32_t now = millis();
    tsllcRearmRequired = true;
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_REARM_REQUIRED,
                            tsllcInjectionBlockReason(now));
    tsllcRearmRequired = false;
    aChannelDiag.aSafetyHold = true;
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_A_SAFETY_HOLD,
                            tsllcInjectionBlockReason(now));
    aChannelDiag.aSafetyHold = false;
    aChannelDiag.aSafetyLatched = true;
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_A_SAFETY_LATCH,
                            tsllcInjectionBlockReason(now));
}

void test_tsllc_time_frame_ap_stability_and_freshness_boundaries()
{
    tsllcRuntime = true;
    aChannelTxRuntime = true;
    aCanStartedMs = 100;
    aChannelDiag.framesReceivedTotal = kTsllcMinValidAFrames + 1U;
    summonGateDiag.apState = 3;
    summonGateDiag.apActive = true;
    summonGateDiag.apActiveSinceMs = 14100;
    summonGateDiag.last921Ms = 15100;

    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_STARTUP_TIME,
                            tsllcInjectionBlockReason(15099));
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_READY,
                            tsllcInjectionBlockReason(15100));

    aChannelDiag.framesReceivedTotal = kTsllcMinValidAFrames;
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_STARTUP_FRAMES,
                            tsllcInjectionBlockReason(15100));
    aChannelDiag.framesReceivedTotal = kTsllcMinValidAFrames + 1U;

    summonGateDiag.apActiveSinceMs = 14101;
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_AP_STABILIZING,
                            tsllcInjectionBlockReason(15100));
    summonGateDiag.apActiveSinceMs = 14100;
    summonGateDiag.last921Ms = 14599;
    TEST_ASSERT_EQUAL_UINT8(TSLLC_BLOCK_AP_STALE,
                            tsllcInjectionBlockReason(15100));
}

void test_summon_candidate_uses_aca_plus_spr_without_speed_filter()
{
    CanFrame driveAca = frame280(4, true);
    handler.handleMessage(driveAca, mock);
    CanFrame spr = {.id = 1016, .dlc = 8};
    spr.data[3] = 0xB0;
    handler.handleMessage(spr, mock);

    TEST_ASSERT_TRUE((bool)summonGateDiag.sprSeen);
    TEST_ASSERT_TRUE((bool)summonGateDiag.summoning);
    TEST_ASSERT_EQUAL_UINT8(SUMMON_SESSION_ALLOWED,
                            (uint8_t)summonGateDiag.sessionReason);
    TEST_ASSERT_TRUE(summonGateOpen(millis()));
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)summonGateDiag.frames599);
    TEST_ASSERT_EQUAL_UINT16(kSummonSpeedSnaRaw,
                             (uint16_t)summonGateDiag.vehicleSpeedRaw);
}

void test_active_ap_does_not_override_aca_spr_summoning_session()
{
    primeSummonValidationContext();
    CanFrame driveAca = frame280(4, true);
    handler.handleMessage(driveAca, mock);
    CanFrame spr = {.id = 1016, .dlc = 8};
    spr.data[3] = 0xB0;
    handler.handleMessage(spr, mock);
    TEST_ASSERT_TRUE((bool)summonGateDiag.summoning);

    CanFrame ap = {.id = 921, .dlc = 8};
    ap.data[0] = 3;
    handler.handleMessage(ap, mock);
    TEST_ASSERT_TRUE((bool)summonGateDiag.summoning);
    TEST_ASSERT_EQUAL_UINT8(SUMMON_SESSION_ALLOWED,
                            (uint8_t)summonGateDiag.sessionReason);
    TEST_ASSERT_TRUE(summonGateOpen((uint32_t)summonGateDiag.last921Ms));
}

void test_spr_cancel_closes_session_and_cancels_pending_summon_tx()
{
    CanFrame driveAca = frame280(4, true);
    handler.handleMessage(driveAca, mock);
    CanFrame spr = {.id = 1016, .dlc = 8};
    spr.data[3] = 0x40;
    handler.handleMessage(spr, mock);
    TEST_ASSERT_TRUE((bool)summonGateDiag.summoning);
    TEST_ASSERT_EQUAL(0, mock.canceledSources.size());

    CanFrame cancel = {.id = 1016, .dlc = 8};
    cancel.data[3] = 0x30;
    handler.handleMessage(cancel, mock);

    TEST_ASSERT_FALSE((bool)summonGateDiag.summoning);
    TEST_ASSERT_EQUAL(1, mock.canceledSources.size());
    TEST_ASSERT_EQUAL_UINT8((uint8_t)CanTxSource::Summon,
                            (uint8_t)mock.canceledSources[0]);
}

void test_a_channel_tx_master_blocks_tsllc_and_summon()
{
    aChannelTxRuntime = false;
    CanFrame tsllc = mux0Frame();
    handler.handleMessage(tsllc, mock);
    CanFrame summon = mux1Frame();
    handler.handleMessage(summon, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0, summonGateDiag.mux1Received);
    TEST_ASSERT_EQUAL_UINT32(0, summonGateDiag.txOk);
}

void test_ota_quiesce_blocks_new_a_channel_modification_tx()
{
    canTxQuiesceBegin();
    CanFrame tsllc = mux0Frame();
    handler.handleMessage(tsllc, mock);
    CanFrame summon = mux1Frame();
    handler.handleMessage(summon, mock);

    TEST_ASSERT_EQUAL(0, mock.sent.size());
    TEST_ASSERT_TRUE(canTxQuiesceIdle());
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_filter_restores_five_ids_without_257_or_659);
    RUN_TEST(test_unknown_boot_state_blocks_mux1_until_park_is_received);
    RUN_TEST(test_disabled_toggle_blocks_even_when_parked);
    RUN_TEST(test_drive_without_summon_blocks_mux1_injection);
    RUN_TEST(test_aca_plus_spr_opens_summoning_gate_while_in_drive);
    RUN_TEST(test_aca_falling_edge_clears_spr_and_closes_drive_gate);
    RUN_TEST(test_ap_requires_one_second_stability_before_opening_gate);
    RUN_TEST(test_ap_state_two_is_available_not_active);
    RUN_TEST(test_condition_limit_off_cannot_bypass_fail_closed_gate);
    RUN_TEST(test_390_is_used_only_while_fresh_and_stale_state_is_cleared);
    RUN_TEST(test_removed_id_659_is_ignored);
    RUN_TEST(test_tsllc_mux0_is_blocked_when_ap_is_inactive);
    RUN_TEST(test_tsllc_mux0_transmits_only_after_ap_and_startup_gates);
    RUN_TEST(test_tsllc_does_not_send_when_validated_bits_are_already_set);
    RUN_TEST(test_tsllc_mux0_is_held_while_actual_summoning);
    RUN_TEST(test_tsllc_mux0_resumes_after_summoning_ends);
    RUN_TEST(test_summon_retry_policy_requires_actual_summoning_and_safety_gates);
    RUN_TEST(test_tsllc_rearm_and_a_safety_gates_fail_closed);
    RUN_TEST(test_tsllc_time_frame_ap_stability_and_freshness_boundaries);
    RUN_TEST(test_summon_candidate_uses_aca_plus_spr_without_speed_filter);
    RUN_TEST(test_active_ap_does_not_override_aca_spr_summoning_session);
    RUN_TEST(test_spr_cancel_closes_session_and_cancels_pending_summon_tx);
    RUN_TEST(test_a_channel_tx_master_blocks_tsllc_and_summon);
    RUN_TEST(test_ota_quiesce_blocks_new_a_channel_modification_tx);
    return UNITY_END();
}
