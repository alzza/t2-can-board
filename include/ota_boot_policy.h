// OTA pending 상태를 CAN 시작 가능 여부와 부팅 동작으로 변환한다.
#pragma once

#include <cstdint>

enum class OtaBootAction : uint8_t {
    Normal,
    ClearStaleOta,
    FirstBootSafeReset,
    Rollback,
    RecoveryVerification,
    RecoveryOnly,
    FailClosed,
};

struct OtaBootPolicy {
    OtaBootAction action;
    uint8_t nextPending;
    bool canStartAfterAction;
};

constexpr OtaBootPolicy otaBootPolicy(uint8_t pending, bool expectedPartitionMismatch)
{
    if ((pending == 1 || pending == 2) && expectedPartitionMismatch) {
        return {OtaBootAction::ClearStaleOta, 0, true};
    }

    switch (pending) {
        case 0: return {OtaBootAction::Normal, 0, true};
        case 1: return {OtaBootAction::FirstBootSafeReset, 2, true};
        case 2: return {OtaBootAction::Rollback, 3, false};
        case 3: return {OtaBootAction::RecoveryVerification, 4, true};
        case 4: return {OtaBootAction::RecoveryOnly, 5, false};
        case 5: return {OtaBootAction::RecoveryOnly, 5, false};
        default: return {OtaBootAction::FailClosed, pending, false};
    }
}
