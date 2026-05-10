# T-CAN Signal Detail Reference

Generated at: `2026-05-10T19:24:59+09:00`
Source: `https://tcan.latency.is/data/manifest.json`
Filter: platform `ModelY`, bus `VEH, CH`
Requested signals: `DAS_autopilotState, DAS_autopilotHandsOnState, EPAS3P_torsionBarTorque, EPAS3P_handsOnLevel, SCCM_steeringAngle`

이 문서는 T-CAN Explorer의 공개 JSON 데이터를 기준으로 생성했다.
Derived DBC line과 byte span은 읽기 편의를 위한 보조 정보다.

## DAS_autopilotState

### 0x399 DAS_status

- T-CAN URL: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x399__DAS_status
- Frame key: `0x399__DAS_status`
- Address: decimal `921`, hex `0x399`
- Frame name: `DAS_status`
- Module: `DAS`
- Buses in T-CAN: `CH, ETH`
- DLC: `8` bytes
- Frame signal count: `26`
- Matching frame sources: `dbc:ModelY_CH`
- Matching signal sources: `dbc:ModelY_CH`
- All frame sources: `dbc:Model3_CH, dbc:ModelY_CH, dbc:Poppyseed_CH, json:mcu2, json:mcu3, json:msx-amd, json:msx-intel`
- All signal sources: `dbc:Model3_CH, dbc:ModelY_CH, dbc:Poppyseed_CH, json:mcu2, json:mcu3, json:msx-amd, json:msx-intel`

Signal layout.

- Start bit: `0`
- Length: `4` bits
- Byte order: `little`
- Signed: `False`
- Byte span: `byte 0`
- Bit positions: `[0, 1, 2, 3]`
- Scale: `1`
- Offset: `0`
- Min/Max from source: `0` / `0`
- Raw mask: `0xF`
- Raw range by bit width: `0` / `15`
- Physical range by bit width: `0` / `15`
- Unit: ``
- Mux: `None`
- Physical formula: `physical = raw * 1`

Visual bit map.

`raw[0]` is the least significant bit of the extracted raw value. Columns are payload bit positions inside each byte.

```text
  byte  bit7  bit6  bit5  bit4    bit3    bit2    bit1    bit0
byte 0     .     .     .     .  raw[3]  raw[2]  raw[1]  raw[0]
```

Bit layout table.

| Byte | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
|------|------|------|------|------|------|------|------|------|
| `0` | `.` | `.` | `.` | `.` | `raw[3]` | `raw[2]` | `raw[1]` | `raw[0]` |

DBC-style signal line.

```dbc
SG_ DAS_autopilotState : 0|4@1+ (1,0) [0|0] "" Vector__XXX
```

Raw extraction pseudocode.

```text
payload_le = data[0] | (data[1] << 8) | ... | (data[7] << 56)
raw = (payload_le >> 0) & 0xF
```

Enum symbol: `Diag_DAS_autopilotState_map`

| Dec | Hex | Label |
|-----|-----|-------|
| `0` | `0x0` | `DISABLED` |
| `1` | `0x1` | `UNAVAILABLE` |
| `2` | `0x2` | `AVAILABLE` |
| `3` | `0x3` | `ACTIVE_NOMINAL` |
| `4` | `0x4` | `ACTIVE_RESTRICTED` |
| `5` | `0x5` | `ACTIVE_NAV` |
| `6` | `0x6` | `ACTIVE_FSD` |
| `8` | `0x8` | `ABORTING` |
| `9` | `0x9` | `ABORTED` |
| `14` | `0xE` | `FAULT` |
| `15` | `0xF` | `SNA` |
| `4294967295` | `0xFFFFFFFF` | `` |

## DAS_autopilotHandsOnState

### 0x399 DAS_status

- T-CAN URL: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x399__DAS_status
- Frame key: `0x399__DAS_status`
- Address: decimal `921`, hex `0x399`
- Frame name: `DAS_status`
- Module: `DAS`
- Buses in T-CAN: `CH, ETH`
- DLC: `8` bytes
- Frame signal count: `26`
- Matching frame sources: `dbc:ModelY_CH`
- Matching signal sources: `dbc:ModelY_CH`
- All frame sources: `dbc:Model3_CH, dbc:ModelY_CH, dbc:Poppyseed_CH, json:mcu2, json:mcu3, json:msx-amd, json:msx-intel`
- All signal sources: `dbc:Model3_CH, dbc:ModelY_CH, dbc:Poppyseed_CH, json:mcu2, json:mcu3, json:msx-amd, json:msx-intel`

Signal layout.

- Start bit: `42`
- Length: `4` bits
- Byte order: `little`
- Signed: `False`
- Byte span: `byte 5`
- Bit positions: `[42, 43, 44, 45]`
- Scale: `1`
- Offset: `0`
- Min/Max from source: `0` / `0`
- Raw mask: `0xF`
- Raw range by bit width: `0` / `15`
- Physical range by bit width: `0` / `15`
- Unit: ``
- Mux: `None`
- Physical formula: `physical = raw * 1`

Visual bit map.

`raw[0]` is the least significant bit of the extracted raw value. Columns are payload bit positions inside each byte.

```text
  byte  bit7  bit6    bit5    bit4    bit3    bit2  bit1  bit0
byte 5     .     .  raw[3]  raw[2]  raw[1]  raw[0]     .     .
```

Bit layout table.

| Byte | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
|------|------|------|------|------|------|------|------|------|
| `5` | `.` | `.` | `raw[3]` | `raw[2]` | `raw[1]` | `raw[0]` | `.` | `.` |

DBC-style signal line.

```dbc
SG_ DAS_autopilotHandsOnState : 42|4@1+ (1,0) [0|0] "" Vector__XXX
```

Raw extraction pseudocode.

```text
payload_le = data[0] | (data[1] << 8) | ... | (data[7] << 56)
raw = (payload_le >> 42) & 0xF
```

Enum symbol: `Diag_LC_autopilotHandsOnState_map`

| Dec | Hex | Label |
|-----|-----|-------|
| `0` | `0x0` | `NOT_REQD` |
| `1` | `0x1` | `REQD_DETECTED` |
| `2` | `0x2` | `REQD_NOT_DETECTED` |
| `3` | `0x3` | `REQD_VISUAL` |
| `4` | `0x4` | `REQD_CHIME_1` |
| `5` | `0x5` | `REQD_CHIME_2` |
| `6` | `0x6` | `REQD_SLOWING` |
| `7` | `0x7` | `REQD_STRUCK_OUT` |
| `8` | `0x8` | `SUSPENDED` |
| `9` | `0x9` | `REQD_ESCALATED_CHIME_1` |
| `10` | `0xA` | `REQD_ESCALATED_CHIME_2` |
| `15` | `0xF` | `SNA` |
| `4294967295` | `0xFFFFFFFF` | `` |

## EPAS3P_torsionBarTorque

### 0x52 EPAS3P_sysStatus

- T-CAN URL: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x52__EPAS3P_sysStatus
- Frame key: `0x52__EPAS3P_sysStatus`
- Address: decimal `82`, hex `0x52`
- Frame name: `EPAS3P_sysStatus`
- Module: `EPAS3P`
- Buses in T-CAN: `CH`
- DLC: `8` bytes
- Frame signal count: `13`
- Matching frame sources: `dbc:ModelY_CH`
- Matching signal sources: `dbc:ModelY_CH`
- All frame sources: `dbc:Model3_CH, dbc:ModelY_CH, dbc:Poppyseed_CH`
- All signal sources: `dbc:Model3_CH, dbc:ModelY_CH, dbc:Poppyseed_CH`

Signal layout.

- Start bit: `19`
- Length: `12` bits
- Byte order: `big`
- Signed: `False`
- Byte span: `bytes 2..3`
- Bit positions: `[19, 18, 17, 16, 31, 30, 29, 28, 27, 26, 25, 24]`
- Scale: `0.01`
- Offset: `-20.5`
- Min/Max from source: `0` / `0`
- Raw mask: `0xFFF`
- Raw range by bit width: `0` / `4095`
- Physical range by bit width: `-20.5` / `20.45`
- Unit: ``
- Mux: `None`
- Physical formula: `physical = raw * 0.01 - 20.5`

Visual bit map.

`raw[0]` is the least significant bit of the extracted raw value. Columns are payload bit positions inside each byte.

```text
  byte    bit7    bit6    bit5    bit4     bit3     bit2    bit1    bit0
byte 2       .       .       .       .  raw[11]  raw[10]  raw[9]  raw[8]
byte 3  raw[7]  raw[6]  raw[5]  raw[4]   raw[3]   raw[2]  raw[1]  raw[0]
```

Bit layout table.

| Byte | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
|------|------|------|------|------|------|------|------|------|
| `2` | `.` | `.` | `.` | `.` | `raw[11]` | `raw[10]` | `raw[9]` | `raw[8]` |
| `3` | `raw[7]` | `raw[6]` | `raw[5]` | `raw[4]` | `raw[3]` | `raw[2]` | `raw[1]` | `raw[0]` |

DBC-style signal line.

```dbc
SG_ EPAS3P_torsionBarTorque : 19|12@0+ (0.01,-20.5) [0|0] "" Vector__XXX
```

Raw extraction pseudocode.

```text
raw = 0
for bit_position in [19, 18, 17, 16, 31, 30, 29, 28, 27, 26, 25, 24]:
    raw = (raw << 1) | ((data[bit_position // 8] >> (bit_position % 8)) & 1)
```

Enum map: not present in T-CAN data.

## EPAS3P_handsOnLevel

### 0x52 EPAS3P_sysStatus

- T-CAN URL: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x52__EPAS3P_sysStatus
- Frame key: `0x52__EPAS3P_sysStatus`
- Address: decimal `82`, hex `0x52`
- Frame name: `EPAS3P_sysStatus`
- Module: `EPAS3P`
- Buses in T-CAN: `CH`
- DLC: `8` bytes
- Frame signal count: `13`
- Matching frame sources: `dbc:ModelY_CH`
- Matching signal sources: `dbc:ModelY_CH`
- All frame sources: `dbc:Model3_CH, dbc:ModelY_CH, dbc:Poppyseed_CH`
- All signal sources: `dbc:Model3_CH, dbc:ModelY_CH, dbc:Poppyseed_CH`

Signal layout.

- Start bit: `39`
- Length: `2` bits
- Byte order: `big`
- Signed: `False`
- Byte span: `byte 4`
- Bit positions: `[39, 38]`
- Scale: `1`
- Offset: `0`
- Min/Max from source: `0` / `0`
- Raw mask: `0x3`
- Raw range by bit width: `0` / `3`
- Physical range by bit width: `0` / `3`
- Unit: ``
- Mux: `None`
- Physical formula: `physical = raw * 1`

Visual bit map.

`raw[0]` is the least significant bit of the extracted raw value. Columns are payload bit positions inside each byte.

```text
  byte    bit7    bit6  bit5  bit4  bit3  bit2  bit1  bit0
byte 4  raw[1]  raw[0]     .     .     .     .     .     .
```

Bit layout table.

| Byte | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
|------|------|------|------|------|------|------|------|------|
| `4` | `raw[1]` | `raw[0]` | `.` | `.` | `.` | `.` | `.` | `.` |

DBC-style signal line.

```dbc
SG_ EPAS3P_handsOnLevel : 39|2@0+ (1,0) [0|0] "" Vector__XXX
```

Raw extraction pseudocode.

```text
raw = 0
for bit_position in [39, 38]:
    raw = (raw << 1) | ((data[bit_position // 8] >> (bit_position % 8)) & 1)
```

Enum map: not present in T-CAN data.

## SCCM_steeringAngle

### 0x129 SCCM_steeringAngleSensor

- T-CAN URL: https://tcan.latency.is/?plat=ModelY&bus=VEH%2CCH&frame=0x129__SCCM_steeringAngleSensor
- Frame key: `0x129__SCCM_steeringAngleSensor`
- Address: decimal `297`, hex `0x129`
- Frame name: `SCCM_steeringAngleSensor`
- Module: `SCCM`
- Buses in T-CAN: `CH, ETH, PARTY, VEH`
- DLC: `8` bytes
- Frame signal count: `10`
- Matching frame sources: `dbc:ModelY_VEH, dbc:ModelY_CH`
- Matching signal sources: `dbc:ModelY_VEH, dbc:ModelY_CH`
- All frame sources: `dbc:Model3_VEH, dbc:Model3_CH, dbc:Model3_PARTY, dbc:ModelY_VEH, dbc:ModelY_CH, dbc:ModelY_PARTY, dbc:Poppyseed_VEH, dbc:Poppyseed_CH, dbc:Poppyseed_PARTY, json:mcu2, json:mcu3, json:msx-amd`
- All signal sources: `dbc:Model3_VEH, dbc:Model3_CH, dbc:Model3_PARTY, dbc:ModelY_VEH, dbc:ModelY_CH, dbc:ModelY_PARTY, dbc:Poppyseed_VEH, dbc:Poppyseed_CH, dbc:Poppyseed_PARTY, json:mcu2, json:mcu3, json:msx-amd`

Signal layout.

- Start bit: `16`
- Length: `14` bits
- Byte order: `little`
- Signed: `False`
- Byte span: `bytes 2..3`
- Bit positions: `[16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29]`
- Scale: `0.1`
- Offset: `-819.2`
- Min/Max from source: `0` / `0`
- Raw mask: `0x3FFF`
- Raw range by bit width: `0` / `16383`
- Physical range by bit width: `-819.2` / `819.1`
- Unit: ``
- Mux: `None`
- Physical formula: `physical = raw * 0.1 - 819.2`

Visual bit map.

`raw[0]` is the least significant bit of the extracted raw value. Columns are payload bit positions inside each byte.

```text
  byte    bit7    bit6     bit5     bit4     bit3     bit2    bit1    bit0
byte 2  raw[7]  raw[6]   raw[5]   raw[4]   raw[3]   raw[2]  raw[1]  raw[0]
byte 3       .       .  raw[13]  raw[12]  raw[11]  raw[10]  raw[9]  raw[8]
```

Bit layout table.

| Byte | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
|------|------|------|------|------|------|------|------|------|
| `2` | `raw[7]` | `raw[6]` | `raw[5]` | `raw[4]` | `raw[3]` | `raw[2]` | `raw[1]` | `raw[0]` |
| `3` | `.` | `.` | `raw[13]` | `raw[12]` | `raw[11]` | `raw[10]` | `raw[9]` | `raw[8]` |

DBC-style signal line.

```dbc
SG_ SCCM_steeringAngle : 16|14@1+ (0.1,-819.2) [0|0] "" Vector__XXX
```

Raw extraction pseudocode.

```text
payload_le = data[0] | (data[1] << 8) | ... | (data[7] << 56)
raw = (payload_le >> 16) & 0x3FFF
```

VAPI metadata.

- Alias: `ETH_SCCM_steeringAngle`
- Source: `libQtCarVAPI.so.1.0.0`

Enum symbol: `Diag_SCCM_steeringAngle_map`

| Dec | Hex | Label |
|-----|-----|-------|
| `16383` | `0x3FFF` | `SNA` |
| `4294967295` | `0xFFFFFFFF` | `` |
