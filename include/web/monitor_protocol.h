#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

inline constexpr uint32_t kMonitorSchemaVersion = 1;

struct MonitorSnapshot {
    uint32_t uptimeS = 0;
    const char *firmwareVersion = "";
    const char *firmwareBuild = "";
    uint32_t otaState = 0;
    uint32_t apStations = 0;
    uint8_t aHealthLevel = 2;
    const char *aHealthState = "INIT";
    float aHz = 0.0f;
    uint32_t aFrameAgeMs = 0;
    uint32_t aBusoffCount = 0;
    uint32_t aEflg = 0;
    uint32_t aTec = 0;
    uint32_t aRec = 0;
    uint32_t aRxOverrun = 0;
    uint32_t aTxQueued = 0;
    uint32_t aTxBusy = 0;
    uint32_t aTxHard = 0;
    bool aTxGuard = false;
    uint8_t bHealthLevel = 2;
    const char *bHealthState = "INIT";
    float bHz = 0.0f;
    uint32_t bFrameAgeMs = 0;
    uint32_t bBusoffCount = 0;
    uint32_t bTwaiState = 0;
    uint32_t bTec = 0;
    uint32_t bRec = 0;
    uint32_t bRecoveryQuietMs = 0;
    uint32_t bArbLost = 0;
    uint32_t bBusError = 0;
    uint32_t bTxFailed = 0;
    uint32_t bEchoCount = 0;
    bool eceR79 = false;
    bool summon = false;
    bool tsllc = false;
    bool nag = false;
    uint32_t nagMode = 0;
    bool nagApOnly = false;
    bool nagReady = false;
    bool gateOpen = false;
    const char *gateReason = "UNKNOWN";
    bool apActive = false;
    uint32_t apState = 0;
    uint32_t apStableMs = 0;
    bool parked = false;
    bool summoning = false;
    uint32_t summonTxOk = 0;
    uint32_t summonTxFail = 0;
    uint32_t summonBlocked = 0;
    bool userMarkActive = false;
    uint32_t userMarkCount = 0;
};

inline size_t formatMonitorJson(char *out, size_t outSize, const MonitorSnapshot &s) {
    if (!out || outSize == 0) return 0;
    const int n = std::snprintf(
        out, outSize,
        "{\"schema\":%u,\"uptime_s\":%u,\"firmware\":\"%s\",\"build\":\"%s\","
        "\"ota_state\":%u,\"ap_stations\":%u,"
        "\"a\":{\"level\":%u,\"state\":\"%s\",\"hz\":%.1f,\"age_ms\":%u,"
        "\"busoff\":%u,\"eflg\":%u,\"tec\":%u,\"rec\":%u,\"rx_overrun\":%u,"
        "\"tx_q\":%u,\"tx_busy\":%u,\"tx_hard\":%u,\"tx_guard\":%s},"
        "\"b\":{\"level\":%u,\"state\":\"%s\",\"hz\":%.1f,\"age_ms\":%u,"
        "\"busoff\":%u,\"twai\":%u,\"tec\":%u,\"rec\":%u,\"recovery_quiet_ms\":%u,"
        "\"arb_lost\":%u,\"bus_error\":%u,\"tx_failed\":%u,\"echo\":%u},"
        "\"features\":{\"ece_r79\":%s,\"summon\":%s,\"tsllc\":%s,\"nag\":%s,"
        "\"nag_mode\":%u,\"nag_ap_only\":%s,\"nag_ready\":%s},"
        "\"gate\":{\"open\":%s,\"reason\":\"%s\",\"ap\":%s,\"ap_state\":%u,"
        "\"ap_stable_ms\":%u,\"parked\":%s,\"summoning\":%s,\"tx_ok\":%u,"
        "\"tx_fail\":%u,\"blocked\":%u},"
        "\"user_mark\":{\"active\":%s,\"count\":%u}}",
        (unsigned)kMonitorSchemaVersion, (unsigned)s.uptimeS,
        s.firmwareVersion, s.firmwareBuild, (unsigned)s.otaState, (unsigned)s.apStations,
        (unsigned)s.aHealthLevel, s.aHealthState, (double)s.aHz, (unsigned)s.aFrameAgeMs,
        (unsigned)s.aBusoffCount, (unsigned)s.aEflg, (unsigned)s.aTec, (unsigned)s.aRec,
        (unsigned)s.aRxOverrun, (unsigned)s.aTxQueued, (unsigned)s.aTxBusy,
        (unsigned)s.aTxHard, s.aTxGuard ? "true" : "false",
        (unsigned)s.bHealthLevel, s.bHealthState, (double)s.bHz, (unsigned)s.bFrameAgeMs,
        (unsigned)s.bBusoffCount, (unsigned)s.bTwaiState, (unsigned)s.bTec, (unsigned)s.bRec,
        (unsigned)s.bRecoveryQuietMs, (unsigned)s.bArbLost, (unsigned)s.bBusError,
        (unsigned)s.bTxFailed, (unsigned)s.bEchoCount,
        s.eceR79 ? "true" : "false", s.summon ? "true" : "false",
        s.tsllc ? "true" : "false", s.nag ? "true" : "false", (unsigned)s.nagMode,
        s.nagApOnly ? "true" : "false", s.nagReady ? "true" : "false",
        s.gateOpen ? "true" : "false", s.gateReason, s.apActive ? "true" : "false",
        (unsigned)s.apState, (unsigned)s.apStableMs, s.parked ? "true" : "false",
        s.summoning ? "true" : "false", (unsigned)s.summonTxOk,
        (unsigned)s.summonTxFail, (unsigned)s.summonBlocked,
        s.userMarkActive ? "true" : "false", (unsigned)s.userMarkCount);
    if (n < 0 || (size_t)n >= outSize) {
        out[0] = '\0';
        return 0;
    }
    return (size_t)n;
}
