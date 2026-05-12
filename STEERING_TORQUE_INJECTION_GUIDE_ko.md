# 스티어링 토크 주입 로직 포팅 가이드

## 1. 개요

이 문서는 현재 Smart Torque 스티어링 토크 주입 전략을 설명한다. 범위는 ID `880` EPAS 프레임을 재작성하는 B채널 Autosteer Nag Killer 경로로 한정한다. 관련 없는 A채널 기능, OTA, BLE, 전체 CAN 전송 계층 설정은 다루지 않는다.

현재 구현은 gate가 있는 상태머신이다. ID `921` 또는 `923`의 `DAS_status`를 감시하고, ID `297`에서 조향각을 읽은 뒤, 조건이 맞으면 `EPAS3P_torsionBarTorque`와 `EPAS3P_handsOnLevel`을 수정한 ID `880` echo 프레임을 보낸다. 목표는 일정한 힘을 계속 넣는 것이 아니다. 이 로직은 AP 상태 gate, DAS hands-on demand 상태, 프로파일별 delay, 선택적 burst/pause window, 조향각 기반 토크 방향, 실제 운전자 입력 bypass를 함께 사용한다.

현재 동작 요약.

1. Autosteer Nag Killer runtime이 ON이고 `DAS_autopilotState`가 `3, 4, 5, 6` 중 하나일 때만 주입 가능하다.
2. `DAS_autopilotState` 값 `0, 1, 2, 8, 9, 14, 15`에서는 주입하지 않는다. 로그에서는 880 프레임이 들어오는 동안 보통 `AP_BLOCK`으로 보인다.
3. `DAS_autopilotHandsOnState` 값 `0, 8, 15`, 그리고 미수신 상태인 `0xFF`에서는 주입하지 않는다.
4. HandsOnState `1`은 idle 상태다. 일부 프로파일은 직전에 생성했던 토크를 짧게 유지해 전환 cushion으로 사용한다.
5. HandsOnState `2`는 mild 경로다. 선택된 프로파일 delay만큼 기다린 뒤, 조향각 반대 방향으로 mild random-walk 토크를 적용한다.
6. HandsOnState `3`, `4`, `5`는 strong 경로다. 선택된 프로파일 delay만큼 기다린 뒤, `2.10 Nm`까지 ramp-and-hold 파형을 적용한다.
7. 수신한 EPAS 프레임이 이미 실제 `EPAS3P_handsOnLevel != 0`을 보고하면 주입을 멈추고 원본 프레임을 그대로 둔다.
8. Echo 프레임은 880 counter nibble을 증가시키고, torque와 HandsOnLevel bit를 수정한 뒤 checksum을 다시 계산한다.

현재 활성 구현은 Smart Torque 단일 경로다. 예전 Mode A 또는 stealth PRNG 동작을 현재 펌웨어 기준으로 사용하면 안 된다.

## 2. 현재 Smart Torque 프로파일

펌웨어는 선택된 프로파일을 `nagConfig.smartProfile`에 저장한다. 아래 프로파일 값은 현재 `NagSmartProfileSettings`의 source of truth다.

| Profile | 의도 | State 1 grace | State 2 delay | State 2 mild torque | State 2 burst/pause | Strong delay | Strong ramp | Strong burst/pause | 상태 |
|---------|------|---------------|---------------|---------------------|----------------------|--------------|-------------|--------------------|------|
| `0` 기본 | 현재 검증된 baseline | `500 ms` | `700 ms` | `±0.5..1.5 Nm` | continuous | `400 ms` | `500 ms` to `2.10 Nm` | continuous | 실차 로그로 700/400ms 반응 확인 |
| `1` A안 | 짧은 grace와 observation pause | `150 ms` | `700 ms` | `±0.5..1.5 Nm` | `250/750 ms` | `400 ms` | `500 ms` to `2.10 Nm` | `500/1000 ms` | 주입량 감소 실험 후보 |
| `2` B안 | 가장 보수적인 sparse injection | `0 ms` | `900 ms` | `±0.5..1.5 Nm` | `150/1350 ms` | `600 ms` | `500 ms` to `2.10 Nm` | `300/1700 ms` | 주입량 최소화 실험 후보 |
| `3` C안 | delay와 mild torque 조정 1차 후보 | `500 ms` | `600 ms` | `±0.5..1.7 Nm` | continuous | `400 ms` | `500 ms` to `2.10 Nm` | continuous | 구현 및 mock 검증 완료, 실차 검증 전 |

`burst/pause` 값이 `0/0`이면 window gate는 항상 열린다. 값이 있으면 burst 시간 동안만 echo를 보내고, pause 시간 동안은 880을 받아도 echo를 보내지 않는다.

## 3. 실차 로그 근거

현재 프로파일 값은 두 차례의 실차 로그 분석에서 나왔다.

### 2026-05-09 baseline issue

`canmod_20260509_165201.txt`는 기존 Smart Torque timing이 AP-active window에서 너무 느렸음을 보여줬다.

```text
State 2 first echo: mostly 2000/2001 ms
Strong path first echo: 1000 ms
Final no-injection section: AP state 1 or 2, therefore AP_BLOCK
```

수정은 두 대기 window만 줄였다. AP gate를 넓히지는 않았다. AP state 값이 `3, 4, 5, 6` 밖이면 여전히 주입을 막는다.

### 2026-05-10 700/400 ms verification

`canmod_20260510_071256.txt`는 AP state가 active일 때 새 baseline이 의도한 timing으로 반응함을 확인해줬다.

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

같은 로그는 남은 한계도 보여준다. `07:09:45.231`부터 `07:12:55.231`까지 AP state가 `1` 또는 `2`였기 때문에, 펌웨어는 수신한 880 프레임 `19500`개에 대해 `AP_BLOCK`을 만들었고 아무것도 주입하지 않았다. 이는 timing failure가 아니다.

같은 분석의 토크 분포.

```text
Overall injected torque abs median: 0.94 Nm
Overall injected torque abs p75: 1.18 Nm
Overall injected torque abs max: 2.10 Nm
State 2 mild median: 0.93 Nm
State 2 mild max in baseline log: 1.50 Nm
Strong path median/max: 2.10 Nm
```

C안은 이 데이터를 첫 후보로 사용한다. Strong timing은 `400 ms / 2.10 Nm`로 유지하고, state2 delay를 `700 ms`에서 `600 ms`로 당기며, state2 mild max를 `1.5 Nm`에서 `1.7 Nm`로 올린다.

### 프로파일 실험 가치 검토

이 섹션을 작성하기 전에 2026-05-10 로그를 raw bundle에서 직접 다시 파싱했다. 아래 프로파일 검토는 요약된 세션 노트뿐 아니라 그 원본 로그 재파싱 결과를 근거로 한다.

직접 파싱 결과.

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

해석.

1. 현재 `700 ms / 400 ms` baseline은 더 이상 명백한 실패 지점이 아니다. AP-active window에서 echo를 만들었고 CAN health도 깨끗했다.
2. 최신 로그에서 가장 큰 무주입 원인은 여전히 AP gate다. `DAS_autopilotState`가 `1` 또는 `2`일 때의 `AP_BLOCK`은 프로파일 변경으로 해결할 수 없다.
3. State 2 mild torque는 baseline 로그에서 현재 ceiling인 `1.50 Nm`에 도달했다. 이것만으로 ceiling이 낮다고 증명되지는 않지만, C안의 `1.70 Nm` 테스트에는 구체적인 근거가 된다.
4. `1 ms` first-echo delay는 프로파일 목표가 `1 ms`라는 뜻이 아니다. 샘플 또는 이벤트가 기록된 시점에 state나 phase가 이미 eligible이었다는 뜻이다. 설정된 delay target 검증에는 `400 ms`와 `700 ms` cluster를 사용해야 한다.

프로파일별 판단.

| Profile | 최신 로그 기준 판단 | 실험 가치 | 판단 이유 |
|---------|--------------------|----------|----------|
| `0` 기본 | control profile로 유지 | baseline으로서 매우 높음, 최종값이라고 단정하지 않음 | 최신 로그는 기본값이 `700/400 ms` timing에서 주입할 수 있고 `TX-Fail=0`, `TEC/REC=0`, BUS-OFF 없음 상태를 유지함을 보여준다. 동시에 eligible 상태에서 continuous injection이 많고 state2 torque가 `1.50 Nm`에 닿았으므로 최적화 종착점보다는 기준점이다. |
| `1` A안 | duty-reduction profile로 테스트 가치 있음 | 기본과 C안 이후 높음 | 검증된 `700/400 ms` delay와 `1.50 Nm` state2 ceiling을 유지하므로 핵심 변수는 duty cycle이다. `250/750 ms` state2 window와 `500/1000 ms` strong window로 차량이 짧은 observation burst를 경고 재발 없이 받아들이는지 볼 수 있다. |
| `2` B안 | 우선순위 낮은 보수적 boundary test | 효과 후보로는 중간 이하, negative/control run으로 유용 | state1 grace `0 ms`, state2 delay `900 ms`, strong delay `600 ms`, sparse burst window처럼 여러 값을 한 번에 보수적으로 바꾼다. 최신 로그가 이미 `700/400 ms`에서 유효 반응을 보였으므로 B안은 너무 늦거나 너무 sparse할 수 있다. 그래도 주입 하한선을 배우는 데는 유용하다. |
| `3` C안 | 다음 response-improvement 후보로 가장 적합 | 새 기본 control run 뒤 첫 후보로 매우 높음 | 최신 로그가 직접 가리키는 두 값만 바꾼다. state2 delay는 `700 -> 600 ms`, mild ceiling은 `1.50 -> 1.70 Nm`로 조정한다. State1 grace, strong delay, strong ramp, strong max torque는 기본과 같으므로 결과 비교가 쉽다. |

권장 실험 순서.

1. 같은 경로에서 `0` 기본을 먼저 실행해 fresh control log를 만든다.
2. 그 다음 `3` C안을 실행한다. 최신 로그에서 나온 가장 직접적인 가설을 검증하기 때문이다.
3. 그 다음 `1` A안을 실행해 burst/pause가 효과를 잃지 않으면서 injection duty를 줄이는지 측정한다.
4. `2` B안은 마지막에 실행하거나 짧은 boundary test로 실행한다. 의도적으로 보수적이고 너무 sparse할 가능성이 가장 높기 때문이다.

각 프로파일 비교 성공 기준.

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

새 기본값으로 승격하기 전에는 어떤 후보든 fresh 기본 control보다 behavior와 bus-health metrics 모두에서 나아야 한다. C안은 가장 강한 다음 가설이지만, 실차 로그로 확인되기 전까지는 여전히 가설이다.

## 4. 필요한 신호

### DAS Autopilot State

```text
CAN: Private CAN
Message ID: 921 or 923, depending on DBC/source bus
Signal: DAS_autopilotState
Usage: Global allow/deny gate for torque injection.
DBC: 0|4@1+ (1,0)
Implementation: frame.data[0] & 0x0F
```

이 저장소에는 두 ID variant가 모두 있다. `ModelY_PARTY.dbc`는 `BO_ 923 DAS_status`를 정의하고, `ModelY_CH.dbc.txt`는 `BO_ 921 DAS_status`를 정의한다. 현재 펌웨어는 둘 다 받아들이며 마지막 source ID를 `dasStatusSourceId`로 기록한다.

알려진 state 값.

| DEC | HEX | Label | 주입 결과 |
|-----|-----|-------|----------|
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

주입은 `DAS_autopilotState`가 아래 값 중 하나일 때만 허용된다.

```text
3, 4, 5, 6
```

그 외 모든 값에서는 원본 torque와 원본 EPAS HandsOnLevel을 그대로 둔다.

### DAS Hands-On Demand State

```text
CAN: Private CAN
Message ID: 921 or 923, matching DAS_status source
Signal: DAS_autopilotHandsOnState
Usage: Select the injection pattern.
DBC: 42|4@1+ (1,0)
Implementation: (frame.data[5] >> 2) & 0x0F
```

알려진 demand 값.

| DEC | HEX | Label | 현재 펌웨어 동작 |
|-----|-----|-------|----------------|
| 0 | `0x0` | `NOT_REQD` | No injection |
| 1 | `0x1` | `REQD_DETECTED` | State 1 grace 이후 no injection |
| 2 | `0x2` | `REQD_NOT_DETECTED` | State 2 mild path |
| 3 | `0x3` | `REQD_VISUAL` | Strong path |
| 4 | `0x4` | `REQD_CHIME_1` | Strong path |
| 5 | `0x5` | `REQD_CHIME_2` | Strong path |
| 6 | `0x6` | `REQD_SLOWING` | 현재 처리하지 않으며 no injection으로 fall through |
| 7 | `0x7` | `REQD_STRUCK_OUT` | 현재 처리하지 않으며 no injection으로 fall through |
| 8 | `0x8` | `SUSPENDED` | No injection |
| 9 | `0x9` | `REQD_ESCALATED_CHIME_1` | 현재 처리하지 않으며 no injection으로 fall through |
| 10 | `0xA` | `REQD_ESCALATED_CHIME_2` | 현재 처리하지 않으며 no injection으로 fall through |
| 15 | `0xF` | `SNA` | No injection |

이 값은 아래에서 설명하는 state `1`, `2`, 또는 `3/4/5` 동작을 선택한다.

### EPAS Torsion Bar Torque

기본 경로.

```text
CAN: Private CAN
Message ID: 880
Signal: EPAS3P_torsionBarTorque
DBC: 19|12@0+
Usage: Main torque injection target.
```

현재 Smart Torque 경로는 ID `880`만 사용한다.

현재 raw center.

```text
2048
```

변환 모델.

```text
Injection generator model: raw = 2048 + torqueNm * 100
Injected log model:       torqueNm = (raw - 2048) * 0.01
DBC physical model:       torqueNm = raw * 0.01 - 20.5
```

펌웨어는 현재 생성된 echo frame과 `modeBLastNm`에 대해 raw `2048`을 injection center로 사용한다. 수신한 실제 torque telemetry인 `realTorqueNm`은 DBC physical formula를 사용한다.

예시.

```text
+0.5 Nm -> raw 2098
+2.0 Nm -> raw 2248
+2.1 Nm -> raw 2258
-0.5 Nm -> raw 1998
-2.0 Nm -> raw 1848
-2.1 Nm -> raw 1838
```

### EPAS HandsOnLevel

기본 경로.

```text
CAN: Private CAN
Message ID: 880
Signal: EPAS3P_handsOnLevel
DBC: 39|2@0+
Usage: HandsOnLevel injection target and driver-bypass feedback.
```

수신한 `EPAS3P_handsOnLevel`도 관찰한다. 현재 구현에서는 수신값이 0이 아니면 운전자가 조향 중이라고 보고 echo frame을 보내지 않는다.

### Steering Angle

현재 구현은 아래 신호를 사용한다.

```text
CAN: Private CAN
Message ID: 297
Signal: SCCM_steeringAngle
DBC: 16|14@1+ (0.1,-819.2)
Usage: Decide injected torque direction.
```

변환식.

```text
angleDeg = raw * 0.1 - 819.2
```

방향 규칙.

```text
angleDeg > 0  -> inject negative torque
angleDeg <= 0 -> inject positive torque
```

조향각 threshold를 넘으면 torque를 멈추는 safety gate는 코드에 남아 있지만 현재 비활성화되어 있다. 명시적으로 다시 활성화하지 않는 한 angle-based pause가 동작한다고 가정하지 말아야 한다.

## 5. 전역 활성 조건

주입은 아래 조건이 모두 참일 때만 허용된다.

```text
nagKillerActive == true
nagKillerRuntime == true
Current frame ID == 880
Current frame DLC >= 8
DAS_autopilotState in [3, 4, 5, 6]
DAS_autopilotHandsOnState is not 0, 8, 15, or 0xFF
incoming EPAS3P_handsOnLevel == 0
```

조건 하나라도 실패하면 펌웨어는 해당 880 입력에 대해 echo frame을 보내지 않는다. 원본 bus traffic은 건드리지 않는다.

로그에서 쓰는 decision mapping.

```text
runtime off or Nag disabled -> OFF
AP state outside 3..6       -> AP_BLOCK
HandsOnState idle/missing   -> DAS_IDLE or NO_921
real EPAS HandsOnLevel != 0 -> HANDS_ON
delay or closed burst gate  -> NO_ECHO
echo sent                   -> ECHO
```

DBC label이 `AVAILABLE`이더라도 구현은 state `2`에 대해 AP gate를 넓히지 않는다. 실차 로그가 state `2`에서도 주입이 필요함을 증명하기 전까지는 의도적으로 보수적인 선택이다.

## 6. HandsOnState 로직

### State 0, 8, 15

주입하지 않는다.

```text
torque = original
EPAS_handsOnLevel = original
```

### State 1

State `1`에 진입할 때, 현재 구현은 선택된 프로파일의 `state1GraceMs` 동안 가장 최근에 생성한 torque와 spoofed HandsOnLevel을 유지할 수 있다. 이 짧은 grace period가 지나면 주입을 멈춘다.

State `1` 자체는 idle 상태로 의도되어 있다. Grace period는 state `2` 또는 더 강한 state에서 이미 토크를 주입하던 중 demand가 `1`로 내려갔을 때 갑작스러운 끊김을 피하기 위한 것이다. B안은 이 grace를 `0 ms`로 설정하므로 state `1`에서는 절대 주입하지 않는다.

```text
if now - state1EnterTime < profile.state1GraceMs:
    torque = lastGeneratedTorque
    EPAS_handsOnLevel = lastSpoofedHandsOnLevel
else:
    torque = original
    EPAS_handsOnLevel = original
```

의도.

```text
When no hands-on demand is active, remove injected torque and create an idle period.
```

### State 2

State `2`에 진입하면 선택된 프로파일의 `state2DelayMs` 동안 주입을 멈춘다.

```text
if now - state2EnterTime < profile.state2DelayMs:
    no echo
```

Delay 이후에는 프로파일의 state2 burst window가 열려 있을 때만 mild organic torque를 적용한다.

```text
state2ActiveMs = elapsed - profile.state2DelayMs
if burst/pause is disabled:
    window is always open
elif state2ActiveMs % (state2BurstMs + state2PauseMs) < state2BurstMs:
    window is open
else:
    no echo
```

Window가 열려 있으면 펌웨어는 persistent random-walk raw torque 값을 사용한다. Torque range는 프로파일별로 다르다.

Torque range.

```text
if steeringAngle > 0:
    torque range = -profile.state2MildMaxNm to -profile.state2MildMinNm
else:
    torque range = +profile.state2MildMinNm to +profile.state2MildMaxNm
```

현재 프로파일 범위.

```text
기본/A/B: ±0.5..1.5 Nm
C안:      ±0.5..1.7 Nm
```

구현 방식.

```text
Keep a persistent raw torque value.
Move it by a small random-walk step.
Clamp it inside the selected range.
```

HandsOnLevel.

```text
if abs(torqueNm) >= 2.0:
    EPAS_handsOnLevel = 2
elif abs(torqueNm) >= 1.0:
    EPAS_handsOnLevel = 1
else:
    EPAS_handsOnLevel = 0
```

현재 프로파일에서는 state `2`가 보통 HandsOnLevel `1`을 만든다. Mild max가 `1.5 Nm` 또는 `1.7 Nm`이기 때문이다. 아래 level-2 hold 경로는 코드에 남아 있지만, 향후 프로파일이 state2 mild torque를 `2.0 Nm` 이상으로 올리기 전에는 dormant 상태다.

추가 state-2 hold 동작.

```text
When HandsOnLevel first reaches 2:
    hold the current torque and HandsOnLevel=2 for 1000ms
```

### States 3, 4, 5

State `3`, `4`, `5`는 같은 strong hands-on demand group으로 처리된다.

이 group 밖에서 안으로 진입할 때, 선택된 프로파일의 `strongDelayMs` 동안 주입을 멈춘다.

```text
if now - strongStateEnterTime < profile.strongDelayMs:
    no echo
```

`3`, `4`, `5` 사이의 이동은 이 timer를 reset하지 않는다. Group을 벗어났다가 다시 들어오면 새 cycle을 시작한다.

Delay 이후에는 조향각 반대 방향으로 더 강한 ramp-and-hold torque를 적용한다. 프로파일에 strong burst/pause window가 있으면 window가 열려 있을 때만 echo frame을 보낸다. Strong waveform phase 자체는 burst window에 의해 reset되지 않는다.

Torque pattern.

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

Default와 C안에는 strong burst/pause gate가 없으므로 strong state가 유지되는 동안 continuous ramp-and-hold cycle을 만든다. A안과 B안은 의도적으로 echo duty cycle을 줄인다. A안의 `500/1000 ms` strong gate는 대부분 ramp 구간을 sample한다. B안의 `300/1700 ms` strong gate는 sparse하며 시간이 지나면서 1500 ms strong waveform의 서로 다른 부분을 sample할 수 있다.

HandsOnLevel.

```text
if abs(torqueNm) >= 2.0:
    EPAS_handsOnLevel = 2
elif abs(torqueNm) >= 1.0:
    EPAS_handsOnLevel = 1
else:
    EPAS_handsOnLevel = 0
```

State가 `3/4/5`를 벗어나면 strong pattern을 즉시 멈추고 현재 state의 규칙으로 돌아간다.

## 7. 상태 전환 메모리

최소한 아래 값을 저장한다.

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

전환 규칙.

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

## 8. Echo Frame 구성

현재 펌웨어는 수신한 ID `880` 프레임의 echo를 보낸다. 이때 torque, HandsOnLevel, counter, checksum 관련 byte만 바꾼다.

```text
torqRaw is a 12-bit value centered around 2048.
hoLevel is a 2-bit value.

echo.data[2] low nibble <- torqRaw high nibble
echo.data[3]            <- torqRaw low byte
echo.data[4] bits 6..7  <- hoLevel
echo.data[6] low nibble <- original counter + 1
echo.data[7]            <- (sum echo.data[0..6] + 0x73) & 0xFF
```

원본 ID `880` 프레임은 이미 차량에서 수신된 상태다. Smart Torque는 pass-through를 위해 수신 object를 제자리에서 mutate하지 않는다. 별도의 echo frame을 보내거나 아무것도 보내지 않는다.

## 9. Runtime Diagnostics와 로그 해석

프로파일을 비교하거나 주입이 일어나지 않은 이유를 진단할 때 아래 필드가 가장 유용하다.

| Field | 의미 | 읽는 법 |
|-------|------|--------|
| `smartProfile` | 선택된 profile id | `0` 기본, `1` A안, `2` B안, `3` C안 |
| `apState` | 마지막 `DAS_autopilotState` | `3..6`만 주입 가능 |
| `dasSource` | 마지막 DAS_status source ID | `921` 또는 `923` |
| `modeBPhase` | 내부 Smart Torque phase | 아래 phase table 참조 |
| `modeBFirstEchoDelayMs` | 현재 hands-on state 진입 후 첫 echo delay | 핵심 timing 검증 필드 |
| `modeBLastNm` | 구현 모델 기준 마지막 주입 torque | profile torque range와 비교 |
| `dModeBInject` | 현재 5초 interval의 echo count | duty ratio를 위해 `d880`과 비교 |
| `dSkipAP` | AP gate가 막은 880 frame 수 | 값이 크면 timing 문제가 아니다 |
| `dSkipHO` | 실제 운전자 입력이 감지되어 skip한 880 frame 수 | 운전자가 핸들을 만질 때 정상 |
| `dSkipDAS` | DAS hands-on state가 주입을 막은 수 | 대개 idle, suspended, SNA, missing 상태 |
| `lastDecision` | 마지막 per-frame decision | 가장 최근 분기 표시 |
| `intervalDecision` | 5초 summary decision | 로그 개요 확인에 더 좋음 |
| `TEC/REC`, `TX-Fail`, `BUS-OFF` | CAN health | 프로파일 비교 중 조용해야 함 |

현재 phase mapping.

| Phase | 의미 | 비고 |
|-------|------|------|
| `0` | idle, blocked, or no echo | delay tuning phase가 아님 |
| `1` | state1 grace | profile grace가 0이 아닐 때만 held previous torque 전송 |
| `2` | state2 delay 또는 state2 closed burst window | No echo |
| `3` | state2 mild | Mild random-walk echo |
| `4` | strong delay 또는 strong closed burst window | No echo |
| `5` | strong ramp | `2.10 Nm`로 ramp |
| `6` | strong hold | `2.10 Nm` 유지 |

프로파일 비교에는 최소한 아래 column을 함께 사용해야 한다.

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

`apState`, `dSkipAP`, `dSkipHO`, `dSkipDAS`를 확인하기 전에는 `dModeBInject == 0`을 timing failure로 해석하지 말아야 한다. 2026-05-10 로그의 긴 zero-injection 구간은 느린 state2나 strong delay가 아니라 AP state `1/2` 때문에 생겼다.

## 10. Coding Agent를 위한 구현 방향

1. CAN receive layer에서 아래 신호를 추출한다.

```text
DAS_autopilotState
DAS_autopilotHandsOnState
EPAS3P_torsionBarTorque
EPAS3P_handsOnLevel
SCCM_steeringAngle
```

2. 조향각을 물리값으로 변환하고 내부 state로 유지한다.

```text
steeringAngleDeg = raw * 0.1 - 819.2
```

3. 각 EPAS frame handling cycle에서 전역 주입 조건을 먼저 평가한다.

```text
nagKillerActive
nagKillerRuntime
autopilotState in [3, 4, 5, 6]
DAS hands-on demand is not idle/missing
incoming EPAS HandsOnLevel == 0
```

4. 조건이 하나라도 실패하면 echo frame을 보내지 않는다.

5. 조건을 통과하면 `DAS_autopilotHandsOnState`에 따라 state `1`, `2`, 또는 `3/4/5` 로직을 적용한다.

6. Echo EPAS frame에 torque와 HandsOnLevel 변경을 적용한다.

7. 모든 signal 변경을 적용한 뒤 EPAS counter nibble을 증가시키고 checksum을 다시 계산한다.

8. 프로파일별 동작을 보존한다. 대상이 default profile만 지원하는 경우가 아니라면 `700 ms`, `400 ms`, `1.5 Nm`을 hard-code하지 않는다.

## 11. 중요 메모

- State `1`은 idle/no-injection 상태다. Profile grace는 state `2` 또는 더 강한 state 이후의 transition cushion일 뿐이다.
- State `2` delay는 profile-specific이다. 현재 값은 프로파일에 따라 `700 ms`, `900 ms`, `600 ms`다.
- State `2`는 조향각 반대 방향의 mild random-walk torque를 사용한다.
- State `2` mild max는 profile-specific이다. 현재 max는 기본/A/B의 `1.5 Nm`, C안의 `1.7 Nm`이다.
- State `3`, `4`, `5`는 strong ramp-and-hold pattern을 공유한다.
- Strong demand delay는 profile-specific이다. 현재 값은 프로파일에 따라 `400 ms` 또는 `600 ms`다.
- Strong torque는 `500 ms` 동안 `2.10 Nm`까지 ramp하고, burst window가 echo frame을 허용하는 동안 hold한다.
- 대상 프로젝트가 실제 driver hands-on feedback을 관찰한다면, 운전자가 적극적으로 조향 중인 것으로 보일 때는 주입하지 않는다.
- Torque direction은 steering angle direction의 반대여야 한다.
- ID `297`은 현재 코드에서 hard gate가 아니다. Fresh steering-angle frame이 도착하지 않았으면 마지막 저장 angle을 재사용하며, 초기값은 `0.0 deg`다.
- C안은 로그 기반 후보로 구현되어 있지만, default profile과의 실차 비교가 아직 필요하다.