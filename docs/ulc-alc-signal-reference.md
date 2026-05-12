# ULC/ALC 관련 CAN 신호 레퍼런스

> 이 문서는 `scripts/tcan_signal_detail.py`로 생성한 신호 메타데이터를 기반으로 작성했다.
> 생성 기준일: 2026-05-11, 플랫폼: ModelY, 버스: VEH, CH

---

## 개요

| 신호 | 프레임 | ID | 버스 | 구현 상태 |
|---|---|---|---|---|
| `UI_autoLaneChangeEnable` | `UI_chassisControl` | 0x293 | VEH + CH | **구현됨** |
| `UI_ulcOffHighway` | `UI_driverAssistControl` | 0x3F8 | CH | **구현됨** |
| `UI_alcOffHighwayEnable` | `UI_driverAssistControl` | 0x3F8 | CH | **구현됨** |
| `UI_ulcSpeedConfig` | `UI_driverAssistControl` | 0x3F8 | CH | **구현됨** |
| `UI_ulcBlindSpotConfig` | `UI_driverAssistControl` | 0x3F8 | CH | **구현됨** |

---

## 프레임 요약

### `UI_chassisControl` — 0x293 (659)

- 길이: 8 바이트
- 버스: CH, ETH, PARTY, VEH
- DBC 출처: ModelY_VEH, ModelY_CH, ModelY_PARTY (및 Model3, Poppyseed)

### `UI_driverAssistControl` — 0x3F8 (1016)

- 길이: 8 바이트
- 버스: CH, ETH
- DBC 출처: ModelY_CH

---

## 신호 상세

### 1. `UI_autoLaneChangeEnable`

자동 차선 변경(ALC) 기능 활성화 여부를 MCU가 DAS에 전달한다.
이 프로젝트에서는 `실험` 탭의 ON 토글이 활성화되면 raw `1`(ON)을 유지한다.

| 항목 | 값 |
|---|---|
| 프레임 | `UI_chassisControl` (0x293) |
| startBit | 24 |
| length | 2 bits |
| byteOrder | little-endian |
| byteSpan | byte 3 |
| rawMask | `0x3` |

**Enum 값**

| raw | 의미 |
|---|---|
| 0 | OFF |
| 1 | ON |
| 3 | SNA (신호 없음) |

**DBC 라인**
```
SG_ UI_autoLaneChangeEnable : 24|2@1+ (1,0) [0|0] "" Vector__XXX
```

**비트 추출 (little-endian)**
```c
// payload를 little-endian 64비트로 읽은 뒤
raw = (payload_le >> 24) & 0x3;
```

**T-CAN 링크**: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x293__UI_chassisControl

---

### 2. `UI_ulcOffHighway`

ULC(Universal Lane Control, 속도 제어 포함 오토파일럿)를 **고속도로 외** 구간에서도 허용하는 설정 비트다. `1`이면 고속도로 제한 없이 ULC 동작을 허용한다.
이 프로젝트에서는 `실험` 탭의 ON 토글이 활성화되면 bit15를 `1`로 유지한다.

| 항목 | 값 |
|---|---|
| 프레임 | `UI_driverAssistControl` (0x3F8) |
| startBit | 15 |
| length | 1 bit |
| byteOrder | little-endian |
| byteSpan | byte 1 |
| rawMask | `0x1` |

**값 의미**

| raw | 의미 |
|---|---|
| 0 | 고속도로 전용 (기본값) |
| 1 | 고속도로 외 허용 |

**DBC 라인**
```
SG_ UI_ulcOffHighway : 15|1@1+ (1,0) [0|0] "" Vector__XXX
```

**비트 추출**
```c
raw = (payload_le >> 15) & 0x1;
```

**T-CAN 링크**: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x3F8__UI_driverAssistControl

---

### 3. `UI_alcOffHighwayEnable`

ALC(Auto Lane Change, 자동 차선 변경)를 **고속도로 외** 구간에서도 허용하는 설정 비트다. 이 프로젝트에서 **현재 구현된 유일한 신호**다.

| 항목 | 값 |
|---|---|
| 프레임 | `UI_driverAssistControl` (0x3F8) |
| startBit | 56 |
| length | 1 bit |
| byteOrder | little-endian |
| byteSpan | byte 7 |
| rawMask | `0x1` |

**값 의미**

| raw | 의미 |
|---|---|
| 0 | 고속도로 전용 (기본값) |
| 1 | 고속도로 외 ALC 허용 |

**DBC 라인**
```
SG_ UI_alcOffHighwayEnable : 56|1@1+ (1,0) [0|0] "" Vector__XXX
```

**비트 추출**
```c
raw = (payload_le >> 56) & 0x1;
```

**펌웨어 구현**

```cpp
// include/can_helpers.h
inline Shared<bool> uiAlcOffHighwayEnableRuntime{kAlcOffHighwayEnableDefaultEnabled};

// include/web/web_server.h
static constexpr char kNvsKeyAlcOffHighway[] = "alc_offhwy";  // NVS 키

// include/handlers.h — 주입 시
setBit(frame, 56, true);  // UI_alcOffHighwayEnable=1: 비고속도로 ALC 허용
```

- NVS 키: `alc_offhwy`
- 웹 UI 토글: 제어 탭 또는 실험 탭의 `UI_alcOffHighwayEnable` 스위치
- `/api/ui-alc-off-highway-enable` 엔드포인트로 런타임 변경 가능
- `/api/status` 응답 필드: `ui_alc_off_highway_enable_enabled`
- 빌드 플래그: `kAlcOffHighwayEnableBuildEnabled` (HW3/HW4 설정에 따라 다름)

**실차 로그 예시**
```
🟣⚡ [A-CH] UI_driverAssistControl 주입 완료: UI_ulcStalkConfirm=0 UI_alcOffHighwayEnable=1
```

**T-CAN 링크**: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x3F8__UI_driverAssistControl

---

### 4. `UI_ulcSpeedConfig`

ULC 속도 제어 공격성 설정이다. DAS가 속도 유지/회복 시 얼마나 적극적으로 가속할지를 결정한다.
이 프로젝트에서는 `실험` 탭 라디오 버튼으로 순정 유지 또는 raw `0`~`3`을 선택한다.

| 항목 | 값 |
|---|---|
| 프레임 | `UI_driverAssistControl` (0x3F8) |
| startBit | 50 |
| length | 2 bits |
| byteOrder | little-endian |
| byteSpan | byte 6 |
| rawMask | `0x3` |

**Enum 값**

| raw | 의미 |
|---|---|
| 0 | DISABLED |
| 1 | MILD — 부드러운 속도 회복 |
| 2 | AVERAGE — 중간 |
| 3 | MAD_MAX — 최대 공격성 |

**DBC 라인**
```
SG_ UI_ulcSpeedConfig : 50|2@1+ (1,0) [0|0] "" Vector__XXX
```

**비트 추출**
```c
raw = (payload_le >> 50) & 0x3;
```

**T-CAN 링크**: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x3F8__UI_driverAssistControl

---

### 5. `UI_ulcBlindSpotConfig`

ULC 동작 중 사각지대 차량 감지 시 차선 변경 차단 강도 설정이다.
이 프로젝트에서는 `실험` 탭 라디오 버튼으로 순정 유지 또는 raw `0`~`2`를 선택한다.

| 항목 | 값 |
|---|---|
| 프레임 | `UI_driverAssistControl` (0x3F8) |
| startBit | 52 |
| length | 2 bits |
| byteOrder | little-endian |
| byteSpan | byte 6 |
| rawMask | `0x3` |

**Enum 값**

| raw | 의미 |
|---|---|
| 0 | STANDARD — 기본 사각지대 차단 |
| 1 | AGGRESSIVE — 더 넓은 범위 차단 |
| 2 | MAD_MAX — 최대 범위 차단 |

**DBC 라인**
```
SG_ UI_ulcBlindSpotConfig : 52|2@1+ (1,0) [0|0] "" Vector__XXX
```

**비트 추출**
```c
raw = (payload_le >> 52) & 0x3;
```

**T-CAN 링크**: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x3F8__UI_driverAssistControl

---

## byte 6 비트 레이아웃 (0x3F8, ulcSpeedConfig + ulcBlindSpotConfig)

byte 6(bit 48–55) 내에 두 신호가 인접해 있다.

```
bit:  55  54  53  52  51  50  49  48
      [  ulcBlindSpotConfig  ][ulcSpeedConfig ][  other  ]
           bits 52-53              bits 50-51
```

주입 시 두 값을 동시에 쓸 때는 마스크 처리가 필요하다.

```c
// byte 6 = data[6]
uint8_t b6 = frame.data[6];
b6 = (b6 & ~(0x3 << 2)) | ((speedCfg & 0x3) << 2);   // bits 50-51 (byte-local 2-3)
b6 = (b6 & ~(0x3 << 4)) | ((blindSpotCfg & 0x3) << 4); // bits 52-53 (byte-local 4-5)
frame.data[6] = b6;
```

---

## 스크립트 재생성 명령

```bash
# JSON 형식
.venv/bin/python scripts/tcan_signal_detail.py \
  UI_ulcOffHighway UI_alcOffHighwayEnable UI_autoLaneChangeEnable \
  UI_ulcSpeedConfig UI_ulcBlindSpotConfig \
  --platform ModelY --bus VEH,CH --format json

# Markdown 형식
.venv/bin/python scripts/tcan_signal_detail.py \
  UI_ulcOffHighway UI_alcOffHighwayEnable UI_autoLaneChangeEnable \
  UI_ulcSpeedConfig UI_ulcBlindSpotConfig \
  --platform ModelY --bus VEH,CH --format markdown \
  --output docs/ulc-alc-signal-reference-raw.md

# HTML 비트맵 확인
.venv/bin/python scripts/tcan_signal_detail.py \
  UI_ulcOffHighway UI_alcOffHighwayEnable UI_autoLaneChangeEnable \
  UI_ulcSpeedConfig UI_ulcBlindSpotConfig \
  --platform ModelY --bus VEH,CH --format html \
  --output docs/ulc-alc-signals.html
```
