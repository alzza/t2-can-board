# Steering Torque Injection Logic Porting Guide

## 1. Overview

This document describes the current Smart Torque steering torque injection strategy. It covers only the B-channel Autosteer Nag Killer path that rewrites ID `880` EPAS frames. It does not describe unrelated A-channel features, OTA, BLE, or full CAN transport setup.

The current implementation is a gated state machine. It watches `DAS_status` from ID `921` or `923`, reads steering angle from ID `297`, and conditionally sends an echoed ID `880` frame with modified `EPAS3P_torsionBarTorque` and `EPAS3P_handsOnLevel`. The goal is not a constant force. The logic combines AP-state gating, DAS hands-on demand states, profile-specific delay, optional burst/pause windows, steering-angle-based torque direction, and real driver-feedback bypass.

Current behavior summary.

1. Injection is possible only when Autosteer Nag Killer runtime is ON and `DAS_autopilotState` is one of `3, 4, 5, 6`.
2. `DAS_autopilotState` values `0, 1, 2, 8, 9, 14, 15` do not inject. In logs this usually appears as `AP_BLOCK` when 880 frames are present.
3. `DAS_autopilotHandsOnState` values `0, 8, 15`, and missing `0xFF`, do not inject.
4. HandsOnState `1` is idle. Some profiles keep the previous generated torque briefly as a transition cushion.
5. HandsOnState `2` is the mild path. It waits the selected profile delay, then applies mild random-walk torque in the direction opposite the steering angle.
6. HandsOnState `3`, `4`, or `5` is the strong path. It waits the selected profile delay, then applies a ramp-and-hold waveform up to `2.10 Nm`.
7. If the incoming EPAS frame already reports real `EPAS3P_handsOnLevel != 0`, injection stops and the original frame passes through.
8. Echo frames increment the 880 counter nibble and recompute the checksum after torque and HandsOnLevel bits are changed.

The active implementation is Smart Torque only. Older Mode A or stealth PRNG behavior should not be used as the reference for current firmware.

## 2. Current Smart Torque Profiles

The firmware stores the selected profile in `nagConfig.smartProfile`. The profile values below are the current source of truth from `NagSmartProfileSettings`.

| Profile | Intent | State 1 grace | State 2 delay | State 2 mild torque | State 2 burst/pause | Strong delay | Strong ramp | Strong burst/pause | Status |
|---------|--------|---------------|---------------|---------------------|----------------------|--------------|-------------|--------------------|--------|
| `0` 기본 | Current verified baseline | `500 ms` | `700 ms` | `±0.5..1.5 Nm` | continuous | `400 ms` | `500 ms` to `2.10 Nm` | continuous | 실차 로그로 700/400ms 반응 확인 |
| `1` A안 | Short grace plus observation pauses | `150 ms` | `700 ms` | `±0.5..1.5 Nm` | `250/750 ms` | `400 ms` | `500 ms` to `2.10 Nm` | `500/1000 ms` | 주입량 감소 실험 후보 |
| `2` B안 | Most conservative sparse injection | `0 ms` | `900 ms` | `±0.5..1.5 Nm` | `150/1350 ms` | `600 ms` | `500 ms` to `2.10 Nm` | `300/1700 ms` | 주입량 최소화 실험 후보 |
| `3` C안 | Delay plus mild torque 1st candidate | `500 ms` | `600 ms` | `±0.5..1.7 Nm` | continuous | `400 ms` | `500 ms` to `2.10 Nm` | continuous | 구현 및 mock 검증 완료, 실차 검증 전 |

`burst/pause` 값이 `0/0`이면 window gate가 항상 열린다. 값이 있으면 burst 시간 동안만 echo를 보내고 pause 시간 동안은 880을 받아도 echo를 보내지 않는다.

## 3. Vehicle Log Basis

The current profile values came from two rounds of vehicle log analysis.

### 2026-05-09 baseline issue

`canmod_20260509_165201.txt` showed that the original Smart Torque timing was too slow during AP-active windows.

```text
State 2 first echo: mostly 2000/2001 ms
Strong path first echo: 1000 ms
Final no-injection section: AP state 1 or 2, therefore AP_BLOCK
```

The fix reduced only the two wait windows. It did not expand the AP gate. AP state values outside `3, 4, 5, 6` still block injection.

### 2026-05-10 700/400 ms verification

`canmod_20260510_071256.txt` confirmed that the new baseline reacts at the intended timing while AP state is active.

```text
Mode B section: 104 five-second samples
d880: 52003
dEcho: 14899
dModeBInject: 14607
dSkipAP: 25477
dSkipHO: 209
dSkipDAS: 0
First echo delays: 700 ms x16, 400 ms x4, 1 ms x10
TX-Fail: 0
TEC/REC: 0/0
BUS-OFF: none
EchoDrop: 0
```

The same log also showed the remaining limitation. From `07:09:45.231` to `07:12:55.231`, AP state was `1` or `2`, so the firmware produced `AP_BLOCK` for `19500` received 880 frames and injected nothing. That was not a timing failure.

Torque distribution from the same analysis.

```text
Overall injected torque abs median: 0.94 Nm
Overall injected torque abs p75: 1.18 Nm
Overall injected torque abs max: 2.10 Nm
State 2 mild median: 0.93 Nm
State 2 mild max in baseline log: 1.50 Nm
Strong path median/max: 2.10 Nm
```

C안 uses this data as a first candidate. It keeps strong timing at `400 ms / 2.10 Nm`, moves state2 delay from `700 ms` to `600 ms`, and increases state2 mild max from `1.5 Nm` to `1.7 Nm`.

### Profile Experiment Value Review

The 2026-05-10 log was re-parsed from the raw bundle before writing this section. The profile review below uses that log as evidence, not only the summarized session notes.

Direct parsing results.

```text
10-minute samples: 120
Mode B samples: 104
Mode B totals: d880=52003, dEcho=14899, dModeBInject=14607, dSkipAP=25477, dSkipHO=209, dSkipDAS=0, dDrop=0
AP-active Mode B samples: 53
AP-active totals: d880=26502, dEcho=14516, dModeBInject=14224, dSkipAP=749, dSkipHO=137, dDrop=0
Zero-injection AP-block samples: 48
Zero-injection AP-block totals: d880=24001, dModeBInject=0, dSkipAP=23975
Timeseries first-echo delay samples: 1 ms x10, 400 ms x4, 700 ms x16, 701 ms x1, 710 ms x1
Injected torque abs samples: median=0.945 Nm, p75=1.18 Nm, max=2.10 Nm
State 2 torque abs samples: median=0.94 Nm, p75=1.06 Nm, max=1.50 Nm
Strong torque abs samples: median=2.10 Nm, max=2.10 Nm
CAN health: BUS-OFF=0, TEC=0, REC=0, TX-Fail=0, EchoDrop=0
```

Interpretation.

1. The current `700 ms / 400 ms` baseline is no longer the obvious failure point. It produced echoes during AP-active windows and kept CAN health clean.
2. The largest no-injection cause in the latest log is still the AP gate. A profile change cannot solve `AP_BLOCK` when `DAS_autopilotState` is `1` or `2`.
3. State 2 mild torque reached the current `1.50 Nm` ceiling in the baseline log. That does not prove the ceiling is too low, but it gives C안 a concrete reason to test `1.70 Nm`.
4. A `1 ms` first-echo delay does not mean the profile target is `1 ms`. It means the state or phase was already eligible when the sample or event was recorded. Use the `400 ms` and `700 ms` clusters to verify configured delay targets.

Profile-by-profile judgment.

| Profile | Latest-log judgment | Experiment value | Why this is the judgment |
|---------|---------------------|------------------|--------------------------|
| `0` 기본 | Keep as the control profile | Very high as baseline, not necessarily final | The latest log proves it can inject at the intended `700/400 ms` timing with `TX-Fail=0`, `TEC/REC=0`, and no BUS-OFF. It also shows continuous eligible injection and state2 torque touching `1.50 Nm`, so it is a reference point rather than an optimized endpoint. |
| `1` A안 | Worth testing as a duty-reduction profile | High, after 기본 and C안 | It keeps the proven `700/400 ms` delays and `1.50 Nm` state2 ceiling, so the main variable is duty cycle. The `250/750 ms` state2 window and `500/1000 ms` strong window can show whether the vehicle accepts shorter observation bursts without re-triggering warnings. |
| `2` B안 | Lower-priority conservative boundary test | Medium to low as an effectiveness candidate, useful as a negative/control run | It changes several values in the conservative direction at once: state1 grace `0 ms`, state2 delay `900 ms`, strong delay `600 ms`, and sparse burst windows. Since the latest log already shows useful response at `700/400 ms`, B안 may be too late or too sparse. It is still useful to learn the lower injection boundary. |
| `3` C안 | Best next response-improvement candidate | Very high, first candidate after collecting a new 기본 control run | It changes only the two values directly suggested by the latest log: state2 delay `700 -> 600 ms` and mild ceiling `1.50 -> 1.70 Nm`. It keeps state1 grace, strong delay, strong ramp, and strong max torque the same as 기본, so the result is easier to compare. |

Recommended experiment order.

1. Run `0` 기본 first on the same route to create a fresh control log.
2. Run `3` C안 next because it tests the most direct log-derived hypothesis.
3. Run `1` A안 after that to measure whether burst/pause reduces injection duty without losing effectiveness.
4. Run `2` B안 last, or as a short boundary test, because it is intentionally conservative and has the highest chance of being too sparse.

Success criteria for each profile comparison.

```text
Safety and bus health:
    BUS-OFF = 0
    TX-Fail = 0
    TEC/REC stay at 0 or return immediately to 0
    EchoDrop = 0

Profile timing:
    smartProfile matches the selected UI profile
    modeBFirstEchoDelayMs clusters around the selected state2/strong delay target
    dModeBInject changes in the expected direction compared with d880

Behavior interpretation:
    dSkipAP is separated from profile failure
    dSkipHO is treated as real driver input, not a profile miss
    dSkipDAS is checked before judging delay or torque
    modeBLastNm stays inside the selected profile range

Field result:
    userMark is pressed when a steering-wheel warning appears, clears, or feels delayed
    each profile run uses the same route and similar AP-active conditions where possible
```

Do not promote any candidate to the new default until it beats the fresh 기본 control on both behavior and bus-health metrics. C안 is the strongest next hypothesis, but it is still a hypothesis until a vehicle log confirms it.

## 4. Required Signals

### DAS Autopilot State

```text
CAN: Private CAN
Message ID: 921 or 923, depending on DBC/source bus
Signal: DAS_autopilotState
Usage: Global allow/deny gate for torque injection.
DBC: 0|4@1+ (1,0)
Implementation: frame.data[0] & 0x0F
```

The repository contains both ID variants. `ModelY_PARTY.dbc` defines `BO_ 923 DAS_status`, while `ModelY_CH.dbc.txt` defines `BO_ 921 DAS_status`. Current firmware accepts both and records the last source ID as `dasStatusSourceId`.

Known state values.

| DEC | HEX | Label | Injection result |
|-----|-----|-------|------------------|
| 0 | `0x0` | `DISABLED` | Block |
| 1 | `0x1` | `UNAVAILABLE` | Block |
| 2 | `0x2` | `AVAILABLE` | Block |
| 3 | `0x3` | `ACTIVE_NOMINAL` | Allow |
| 4 | `0x4` | `ACTIVE_RESTRICTED` | Allow |
| 5 | `0x5` | `ACTIVE_NAV` | Allow |
| 6 | `0x6` | `ACTIVE_FSD` | Allow |
| 8 | `0x8` | `ABORTING` | Block |
| 9 | `0x9` | `ABORTED` | Block |
| 14 | `0xE` | `FAULT` | Block |
| 15 | `0xF` | `SNA` | Block |

Injection is allowed only when `DAS_autopilotState` is one of:

```text
3, 4, 5, 6
```

For all other values, pass through original torque and original EPAS HandsOnLevel.

### DAS Hands-On Demand State

```text
CAN: Private CAN
Message ID: 921 or 923, matching DAS_status source
Signal: DAS_autopilotHandsOnState
Usage: Select the injection pattern.
DBC: 42|4@1+ (1,0)
Implementation: (frame.data[5] >> 2) & 0x0F
```

Known demand values.

| DEC | HEX | Label | Current firmware behavior |
|-----|-----|-------|---------------------------|
| 0 | `0x0` | `NOT_REQD` | No injection |
| 1 | `0x1` | `REQD_DETECTED` | State 1 grace, then no injection |
| 2 | `0x2` | `REQD_NOT_DETECTED` | State 2 mild path |
| 3 | `0x3` | `REQD_VISUAL` | Strong path |
| 4 | `0x4` | `REQD_CHIME_1` | Strong path |
| 5 | `0x5` | `REQD_CHIME_2` | Strong path |
| 6 | `0x6` | `REQD_SLOWING` | Currently not handled, falls through to no injection |
| 7 | `0x7` | `REQD_STRUCK_OUT` | Currently not handled, falls through to no injection |
| 8 | `0x8` | `SUSPENDED` | No injection |
| 9 | `0x9` | `REQD_ESCALATED_CHIME_1` | Currently not handled, falls through to no injection |
| 10 | `0xA` | `REQD_ESCALATED_CHIME_2` | Currently not handled, falls through to no injection |
| 15 | `0xF` | `SNA` | No injection |

This value chooses the state `1`, `2`, or `3/4/5` behavior described below.

### EPAS Torsion Bar Torque

Default path:

```text
CAN: Private CAN
Message ID: 880
Signal: EPAS3P_torsionBarTorque
DBC: 19|12@0+
Usage: Main torque injection target.
```

The current Smart Torque path uses ID `880` only.

Current raw center:

```text
2048
```

Conversion model:

```text
Injection generator model: raw = 2048 + torqueNm * 100
Injected log model:       torqueNm = (raw - 2048) * 0.01
DBC physical model:       torqueNm = raw * 0.01 - 20.5
```

The firmware currently uses raw `2048` as its injection center for generated echo frames and for `modeBLastNm`. The incoming real torque telemetry `realTorqueNm` uses the DBC physical formula.

Examples:

```text
+0.5 Nm -> raw 2098
+2.0 Nm -> raw 2248
+2.1 Nm -> raw 2258
-0.5 Nm -> raw 1998
-2.0 Nm -> raw 1848
-2.1 Nm -> raw 1838
```

### EPAS HandsOnLevel

Default path:

```text
CAN: Private CAN
Message ID: 880
Signal: EPAS3P_handsOnLevel
DBC: 39|2@0+
Usage: HandsOnLevel injection target and driver-bypass feedback.
```

Incoming `EPAS3P_handsOnLevel` is also observed. In the current implementation, a non-zero incoming value means the driver is assumed to be steering, so no echo frame is sent.

### Steering Angle

Current implementation uses:

```text
CAN: Private CAN
Message ID: 297
Signal: SCCM_steeringAngle
DBC: 16|14@1+ (0.1,-819.2)
Usage: Decide injected torque direction.
```

Conversion:

```text
angleDeg = raw * 0.1 - 819.2
```

Direction rule:

```text
angleDeg > 0  -> inject negative torque
angleDeg <= 0 -> inject positive torque
```

There is a retained safety gate for pausing torque above a steering-angle threshold, but it is currently disabled in code. Do not assume angle-based pause is active unless explicitly re-enabled.

## 5. Global Enable Conditions

Injection is allowed only when all conditions are true:

```text
nagKillerActive == true
nagKillerRuntime == true
Current frame ID == 880
Current frame DLC >= 8
DAS_autopilotState in [3, 4, 5, 6]
DAS_autopilotHandsOnState is not 0, 8, 15, or 0xFF
incoming EPAS3P_handsOnLevel == 0
```

If any condition fails, the firmware does not send an echo frame for that 880 input. The original bus traffic is left alone.

Decision mapping used in logs.

```text
runtime off or Nag disabled -> OFF
AP state outside 3..6       -> AP_BLOCK
HandsOnState idle/missing   -> DAS_IDLE or NO_921
real EPAS HandsOnLevel != 0 -> HANDS_ON
delay or closed burst gate  -> NO_ECHO
echo sent                   -> ECHO
```

The implementation does not expand the AP gate for state `2` even though the DBC label is `AVAILABLE`. That choice is intentional and conservative until vehicle logs prove that state `2` still needs injection.

## 6. HandsOnState Logic

### State 0, 8, 15

Do not inject.

```text
torque = original
EPAS_handsOnLevel = original
```

### State 1

When entering state `1`, the current implementation can keep the most recent generated torque and spoofed HandsOnLevel for the selected profile's `state1GraceMs`. After that short grace period, injection stops.

State `1` itself is intended to be idle. The grace period exists only to avoid an abrupt cutoff when torque was already being injected in state `2` or a stronger state and the demand drops to `1`. B안 sets this grace to `0 ms`, so it never injects during state `1`.

```text
if now - state1EnterTime < profile.state1GraceMs:
    torque = lastGeneratedTorque
    EPAS_handsOnLevel = lastSpoofedHandsOnLevel
else:
    torque = original
    EPAS_handsOnLevel = original
```

Intent:

```text
When no hands-on demand is active, remove injected torque and create an idle period.
```

### State 2

When entering state `2`, pause injection for the selected profile's `state2DelayMs`.

```text
if now - state2EnterTime < profile.state2DelayMs:
    no echo
```

After the delay, apply mild organic torque only while the profile's state2 burst window is open.

```text
state2ActiveMs = elapsed - profile.state2DelayMs
if burst/pause is disabled:
    window is always open
elif state2ActiveMs % (state2BurstMs + state2PauseMs) < state2BurstMs:
    window is open
else:
    no echo
```

When the window is open, the firmware uses a persistent random-walk raw torque value. The torque range is profile-specific.

Torque range:

```text
if steeringAngle > 0:
    torque range = -profile.state2MildMaxNm to -profile.state2MildMinNm
else:
    torque range = +profile.state2MildMinNm to +profile.state2MildMaxNm
```

Current profile ranges.

```text
기본/A/B: ±0.5..1.5 Nm
C안:      ±0.5..1.7 Nm
```

Implementation style:

```text
Keep a persistent raw torque value.
Move it by a small random-walk step.
Clamp it inside the selected range.
```

HandsOnLevel:

```text
if abs(torqueNm) >= 2.0:
    EPAS_handsOnLevel = 2
elif abs(torqueNm) >= 1.0:
    EPAS_handsOnLevel = 1
else:
    EPAS_handsOnLevel = 0
```

Under the current profiles, state `2` normally produces HandsOnLevel `1`, because the mild max is `1.5 Nm` or `1.7 Nm`. The level-2 hold path below is retained in code, but it is dormant unless a future profile raises state2 mild torque to at least `2.0 Nm`.

Additional state-2 hold behavior:

```text
When HandsOnLevel first reaches 2:
    hold the current torque and HandsOnLevel=2 for 1000ms
```

### States 3, 4, 5

States `3`, `4`, and `5` are treated as the same strong hands-on demand group.

When entering this group from outside the group, pause injection for the selected profile's `strongDelayMs`.

```text
if now - strongStateEnterTime < profile.strongDelayMs:
    no echo
```

Moving between `3`, `4`, and `5` does not reset this timer. Leaving the group and re-entering starts a new cycle.

After the delay, apply a stronger ramp-and-hold torque in the direction opposite the steering angle. If the profile has a strong burst/pause window, echo frames are sent only while that window is open. The strong waveform phase itself is not reset by the burst window.

Torque pattern:

```text
cycle = 1500ms
phase = activeMs % 1500

if phase < 500ms:
    magnitude ramps from 0.0 Nm to 2.1 Nm
else:
    magnitude holds at 2.1 Nm

if steeringAngle > 0:
    torque = -magnitude
else:
    torque = +magnitude
```

Default and C안 have no strong burst/pause gate, so they produce a continuous ramp-and-hold cycle while the strong state remains active. A안 and B안 intentionally reduce the echo duty cycle. A안's `500/1000 ms` strong gate mostly samples the ramp portion. B안's `300/1700 ms` strong gate is sparse and may sample different parts of the 1500 ms strong waveform over time.

HandsOnLevel:

```text
if abs(torqueNm) >= 2.0:
    EPAS_handsOnLevel = 2
elif abs(torqueNm) >= 1.0:
    EPAS_handsOnLevel = 1
else:
    EPAS_handsOnLevel = 0
```

When the state leaves `3/4/5`, stop this strong pattern immediately and return to the current state's rule.

## 7. State Transition Memory

Store at least these values:

```text
lastDasHandsOnState
state1EnterTime
state2EnterTime
strongStateEnterTime
lastGeneratedTorqueRaw
lastSpoofedHandsOnLevel
state2HoldUntilTime
state2HoldTorqueRaw
state2HoldHandsLevel
modeBPhase
modeBStateEnterTime
modeBPhaseEnterTime
modeBFirstEchoDelayMs
```

Transition rules:

```text
if previousHandsOnState != 1 and currentHandsOnState == 1:
    state1EnterTime = now
    state1HoldTorque = lastGeneratedTorqueRaw
    state1HoldHandsLevel = lastSpoofedHandsOnLevel

if currentHandsOnState != 1:
    clear state1 memory

if previousHandsOnState != 2 and currentHandsOnState == 2:
    state2EnterTime = now

if currentHandsOnState != 2:
    clear state2 delay and hold memory

if previousHandsOnState not in [3,4,5] and currentHandsOnState in [3,4,5]:
    strongStateEnterTime = now

if currentHandsOnState not in [3,4,5]:
    clear strong-state memory
```

## 8. Echo Frame Construction

The current firmware sends an echo of the received ID `880` frame. It changes only the torque, HandsOnLevel, counter, and checksum-related bytes.

```text
torqRaw is a 12-bit value centered around 2048.
hoLevel is a 2-bit value.

echo.data[2] low nibble <- torqRaw high nibble
echo.data[3]            <- torqRaw low byte
echo.data[4] bits 6..7  <- hoLevel
echo.data[6] low nibble <- original counter + 1
echo.data[7]            <- (sum echo.data[0..6] + 0x73) & 0xFF
```

The original ID `880` frame has already been received from the vehicle. Smart Torque does not mutate that received object in place for pass-through. It either sends a separate echo frame or sends nothing.

## 9. Runtime Diagnostics And Log Interpretation

These fields are the most useful when comparing profiles or diagnosing why injection did not happen.

| Field | Meaning | How to read it |
|-------|---------|----------------|
| `smartProfile` | Selected profile id | `0` 기본, `1` A안, `2` B안, `3` C안 |
| `apState` | Last `DAS_autopilotState` | Only `3..6` can inject |
| `dasSource` | Last DAS_status source ID | `921` or `923` |
| `modeBPhase` | Internal Smart Torque phase | See phase table below |
| `modeBFirstEchoDelayMs` | First echo delay after current hands-on state entry | Main timing verification field |
| `modeBLastNm` | Last injected torque using implementation model | Compare with profile torque range |
| `dModeBInject` | Echo count in current 5-second interval | Compare with `d880` for duty ratio |
| `dSkipAP` | 880 frames blocked by AP gate | High value means timing is not the issue |
| `dSkipHO` | 880 frames skipped because real driver input was detected | Expected when the driver touches the wheel |
| `dSkipDAS` | DAS hands-on state blocked injection | Usually idle, suspended, SNA, or missing |
| `lastDecision` | Last per-frame decision | Shows the most recent branch |
| `intervalDecision` | 5-second summary decision | Better for log overview |
| `TEC/REC`, `TX-Fail`, `BUS-OFF` | CAN health | Must stay quiet while comparing profiles |

Current phase mapping.

| Phase | Meaning | Notes |
|-------|---------|-------|
| `0` | idle, blocked, or no echo | Not a delay tuning phase |
| `1` | state1 grace | Sends held previous torque only if profile grace is non-zero |
| `2` | state2 delay or state2 closed burst window | No echo |
| `3` | state2 mild | Mild random-walk echo |
| `4` | strong delay or strong closed burst window | No echo |
| `5` | strong ramp | Ramp toward `2.10 Nm` |
| `6` | strong hold | Holds `2.10 Nm` |

Profile comparison should use at least these columns together.

```text
smartProfile
apState
dasSource
modeBPhase
modeBFirstEchoDelayMs
modeBLastNm
d880
dModeBInject
dSkipAP
dSkipHO
dSkipDAS
lastDecision
intervalDecision
TEC/REC
TX-Fail
BUS-OFF
```

Do not interpret `dModeBInject == 0` as a timing failure until `apState`, `dSkipAP`, `dSkipHO`, and `dSkipDAS` are checked. The 2026-05-10 log had a long zero-injection period caused by AP state `1/2`, not by slow state2 or strong delays.

## 10. Implementation Direction For Coding Agents

1. Extract these signals from the CAN receive layer:

```text
DAS_autopilotState
DAS_autopilotHandsOnState
EPAS3P_torsionBarTorque
EPAS3P_handsOnLevel
SCCM_steeringAngle
```

2. Convert steering angle into a physical value and keep it as internal state:

```text
steeringAngleDeg = raw * 0.1 - 819.2
```

3. During each EPAS frame handling cycle, evaluate global injection conditions first:

```text
nagKillerActive
nagKillerRuntime
autopilotState in [3, 4, 5, 6]
DAS hands-on demand is not idle/missing
incoming EPAS HandsOnLevel == 0
```

4. If any condition fails, do not send an echo frame.

5. If conditions pass, apply state `1`, `2`, or `3/4/5` logic based on `DAS_autopilotHandsOnState`.

6. Apply torque and HandsOnLevel changes to an echoed EPAS frame.

7. Increment the EPAS counter nibble and recompute the checksum after all signal changes are applied.

8. Preserve profile-specific behavior. Do not hard-code `700 ms`, `400 ms`, or `1.5 Nm` unless the target only supports the default profile.

## 11. Important Notes

- State `1` is an idle/no-injection state. Profile grace is only a transition cushion after state `2` or stronger states.
- State `2` delay is profile-specific. Current values are `700 ms`, `900 ms`, or `600 ms` depending on profile.
- State `2` uses mild random-walk torque in the direction opposite the steering angle.
- State `2` mild max is profile-specific. Current max is `1.5 Nm` for 기본/A/B and `1.7 Nm` for C안.
- States `3`, `4`, and `5` share the strong ramp-and-hold pattern.
- Strong demand delay is profile-specific. Current values are `400 ms` or `600 ms` depending on profile.
- Strong torque ramps toward `2.10 Nm` over `500 ms`, then holds when the burst window allows echo frames.
- If the target project observes real driver hands-on feedback, do not inject when the driver appears to be actively steering.
- Torque direction should oppose steering angle direction.
- ID `297` is not a hard gate in current code. If no fresh steering-angle frame has arrived, the last stored angle is reused, initially `0.0 deg`.
- C안 is a log-derived candidate. It is implemented, but it still needs vehicle comparison against the default profile.
