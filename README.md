> [!CAUTION]
> Tesla 차량 펌웨어와 CAN 동작은 예고 없이 바뀔 수 있습니다. 차량 OTA 뒤에는 반드시 정차 상태에서 CAN 수신·BUS-OFF 상태를 먼저 확인한 후 송신 기능을 활성화하십시오.

# Tesla Open CAN Mod — LILYGO T2-CAN HW3

[웹사이트](https://teslaopencanmod.org) | [문서](https://teslaopencanmod.org/docs/intro) | [커뮤니티 Discord](https://discord.gg/ZTQKAUTd2F)

LILYGO T2-CAN ESP32-S3에서 동작하는 Tesla HW3용 듀얼 CAN 펌웨어입니다. CAN-A는 MCP2515로 조건부 Summon Unlock과 TSLLC를 처리하고, CAN-B는 ESP32 TWAI로 Nag Killer를 처리합니다. Wi-Fi Web UI에서 상태 확인, 기능 제어, OTA 업데이트를 수행합니다.

## 면책 및 안전 고지

> [!WARNING]
> FSD는 구매 또는 구독으로 정상 활성화되어야 하는 유료 기능입니다. 이 펌웨어는 Tesla 서비스 권한이나 구독 상태를 생성·변경하지 않습니다.

> [!WARNING]
> CAN 메시지 수정은 차량 동작과 안전에 영향을 줄 수 있습니다. 내용을 이해하지 못했거나 CAN 상태가 불안정하면 차량에 연결하거나 송신 기능을 켜지 마십시오.

이 프로젝트는 개인 실험과 교육 목적입니다. 사용자는 법규, 보증, 차량 손상, 인명 피해를 포함한 모든 결과에 책임이 있습니다. 공도 주행 중 기능 검증을 하지 말고 항상 차량 제어에 집중하십시오.

## 지원 보드와 채널 역할

이 저장소의 기본 빌드는 **LILYGO T2-CAN ESP32-S3 및 Tesla HW3 전용**입니다.

| 채널 | 인터페이스 | 핀 | 역할 |
|---|---|---|---|
| CAN-A | MCP2515 SPI | CS=10, SCK=12, MISO=13, MOSI=11, RST=9 | 조건부 Summon Unlock, TSLLC, 수신 진단 |
| CAN-B | ESP32 TWAI | TX=7, RX=6 | Nag Killer, EPAS 수신·재송신 |
| Wi-Fi | AP 모드 | - | Web UI, OTA, 로그·진단 다운로드 |

> [!IMPORTANT]
> 차량 CAN 버스는 이미 종단되어 있습니다. 별도 CAN 트랜시버 보드의 120 Ω 종단 저항은 제거하거나 우회해야 합니다. 중복 종단은 통신 오류와 BUS-OFF를 유발할 수 있습니다.

## 기능

| 기능 | 채널 | 설명 |
|---|---|---|
| 조건부 Summon Unlock | CAN-A | HW3에서 `Parked || Summoning`일 때만 ID `0x3FD` mux 1을 수정해 재송신 |
| EU Unlock | CAN-A | Summon 수정 프레임의 bit 19를 0으로 하는 동작. 독립 토글·상시 송신 기능이 아님 |
| TSLLC | CAN-A | ID `0x3FD` mux 0의 TSLLC·녹색 신호 계속 주행 비트 수정 |
| Nag Killer | CAN-B | EPAS 토크 프레임 ID 880을 조건에 맞춰 수정·재송신 |
| Web UI·OTA | Wi-Fi | 실시간 CAN 상태, 런타임 토글, OTA 및 복구 UI |

## 빠른 시작

### 1. PlatformIO 설치

```bash
pip install platformio
```

### 2. 빌드

```bash
git clone https://github.com/alzza/t2-can-board.git
cd t2-can-board
pio run -e lilygo_t2can
```

### 3. USB 업로드

```bash
pio run -e lilygo_t2can -t upload
```

### 4. Web UI 연결

- Wi-Fi AP **`TeslaCAN`**에 연결합니다. 기본 비밀번호는 없습니다.
- 브라우저에서 `http://192.168.4.1`을 엽니다.
- 처음에는 CAN-A/B 수신, TEC, REC, BUS-OFF를 확인하고 송신 기능은 OFF로 유지합니다.

## 배선

차량 전면 트렁크 아래 X179 커넥터의 CAN 백본에 연결합니다.

| X179 핀 | 신호 | T2-CAN 연결 |
|---|---|---|
| 13 | CAN-H | CAN-H |
| 14 | CAN-L | CAN-L |

차량과 연식에 따라 커넥터·핀 배치가 다를 수 있으므로 실제 배선도와 차량 신호를 먼저 확인하십시오.

## Web UI

대시보드는 다음 기능을 제공합니다.

- A/B 채널별 프레임 속도, RX/TX, TEC/REC, BUS-OFF와 A채널 EFLG 원인 실시간 표시.
- Summon Unlock, TSLLC, Nag Killer와 A TX 마스터 런타임 제어.
- A/B 시계열·CAN 자가 진단·BUS-OFF 이벤트 CSV 다운로드.
- 차량 재수신부터 첫 Summon TX 성공까지의 지연 표시.
- 프레임을 송신하지 않는 Signal Observer JSON 업로드.
- OTA 업로드, 60초 확인, rollback·복구 UI.

## HW3 Summon Unlock과 EU Unlock 사용법

### 먼저 알아둘 점

이 기능은 차량에 Summon 명령을 새로 보내지 않습니다. 차량이 CAN-A에 보낸 ID `0x3FD`의 mux 1 프레임을 관찰하고, 아래 모든 조건이 맞을 때만 해당 프레임을 수정하여 재송신합니다. 조건이 하나라도 맞지 않으면 Summon 수정 프레임은 전송하지 않습니다.

EU Unlock도 별도 버튼이나 독립된 송신 기능이 아닙니다. Summon Unlock이 실제 전송될 때 같은 프레임의 ECE R79 관련 bit 19를 0으로 만드는 동작을 가리킵니다.

### 활성화 순서

1. 차량을 정차한 뒤 `TeslaCAN` Wi-Fi와 Web UI에 연결합니다.
2. CAN-A 수신이 안정적이고 `BUS-OFF=0`인지 확인합니다.
3. **제어** 화면에서 **Conditional Summon Unlock (HW3)**을 ON으로 설정합니다.
4. CAN-A 카드의 **A TX 마스터**를 ON으로 설정합니다.
5. Summon 카드에서 `Enabled`, `Active`, `Gate OPEN`과 `TX OK / Fail`을 확인합니다.
6. 차량이 `PARKED` 또는 실제 `SUMMONING` 상태일 때만 수정 프레임이 전송되는지 확인합니다.

OTA 직후 첫 부팅에서는 Summon Unlock, TSLLC, Nag Killer, A TX 마스터가 모두 OFF/stock 값으로 저장됩니다. 수신 상태를 확인하고 OTA 확인을 완료한 후 필요한 기능을 하나씩 활성화하십시오.

### 실제 전송 조건

| 조건 | 근거 |
|---|---|
| `SUMMON_UNLOCK` 빌드 포함 | 기본 HW3 빌드에서 활성화됨 |
| 수신 프레임 | CAN-A ID `0x3FD`(10진수 1021), mux 1, DLC 8바이트 |
| 기능 토글 | Web UI의 Conditional Summon Unlock이 ON |
| A TX 마스터 | CAN-A 수정 송신이 ON |
| TX Guard | 보호 모드가 비활성 상태 |
| 상태 게이트 | `Parked || Summoning` |
| 드라이버 전송 | MCP2515 송신이 성공해야 `TX OK` 증가 |

`TX Guard`가 활성화되었거나 A TX 마스터가 OFF이면 Summon과 TSLLC 모두 수정 송신을 건너뜁니다. Summon 토글은 ON인데 게이트가 닫혀 있으면 `Blocked`만 증가하고 송신하지 않습니다.

### 게이트 판정 방식

| 상태 | CAN 근거 | 판정 |
|---|---|---|
| Parked | ID 280의 `DI_gear` | 기어값 P일 때 열림. R/N/D이면 닫힘 |
| Parked 보조 | ID 390의 `DIF_gear` | ID 280이 처음부터 없거나 5초 이상 없을 때만 보조 적용 |
| Summoning | ID 280 ACA + ID 1016 SPR | `DI_autonomyControlActive`와 `UI_selfParkRequest` 관측이 모두 있을 때 열림 |
| AP 상태 | ID 921 | 화면 진단 전용. 게이트를 열거나 닫지 않음 |

INO 기준과 동일하게 ID 280이 5초 이상 수신되지 않으면 보드는 Parked로 간주합니다. 따라서 센트리·절전 등으로 프레임이 끊긴 상태에서는 UI의 수신 카운터와 마지막 280 프레임 경과 시간을 함께 확인하십시오.

### HW3에서 수정되는 비트

| CAN ID | mux | 비트 | 값 | 의미 |
|---|---|---|---|---|
| `0x3FD` | 1 | 19 | 0 | EU Unlock에 해당하는 `UI_applyEceR79` 해제 |
| `0x3FD` | 1 | 46 | 1 | 검증된 HW3 Summon Unlock 비트 |

HW4 INO 경로는 bit 47을 사용하지만, 이 저장소의 기본 빌드는 HW3이므로 bit 46만 사용합니다.

### 즉시 정지 방법

- **Conditional Summon Unlock**을 OFF하면 다음 mux 1 프레임부터 Summon 수정 송신을 중단합니다.
- **A TX 마스터**를 OFF하면 Summon과 TSLLC의 CAN-A 수정 송신을 함께 중단합니다.

### A채널 `WARN` 확인 방법

메인 화면의 A채널 경고는 MCP2515의 현재 EFLG를 기준으로 `수신 오버런`, `오류 경고`, `에러 패시브`, `BUS-OFF`로 나뉩니다. 경고가 보이면 **진단** 화면에서 현재/피크 EFLG, TEC/REC, RX-OVR, MERRF, TX Fail을 확인하고 다음 파일을 저장하십시오.

1. **자가 진단 결과 CSV**를 저장합니다. 진단을 실행하지 않았다면 `NOT_RUN` 안내 행이 저장됩니다.
2. **A/B 상태 시계열 CSV**를 저장합니다.
3. **채널별 이벤트 CSV**를 저장합니다.
4. 가능하면 경고가 보인 즉시 `USER_MARK`를 누르고 **전체 로그 저장**도 실행합니다.

`RX_OVERRUN`은 MCP2515의 2개 수신 버퍼가 소프트웨어가 비우기 전에 가득 찼다는 뜻입니다. 경고를 숨기거나 임계값을 완화하지 않으며, 반복 발생 여부는 CSV의 `a_rx_overrun`, `a_eflg`, `a_loop_gap_last_us`, `a_loop_gap_peak_us`, `a_d_loop_gap_over_2ms` 열로 판단합니다. 오버런 이벤트의 `detail_text`에는 당시 최대 `loop_gap_us`가 함께 저장됩니다.

새 저장 형식은 모든 행에 실제 시각과 업타임을 함께 기록하고, `a_`, `b_`, `system_` 접두사로 채널을 구분합니다. 기본 상태에서는 최근 20분을 자동 보관합니다. **기록 시작**은 기존 로그를 지우고 수동 구간을 시작하며, **기록 정지**는 이후 샘플 추가를 멈춰 그 구간을 고정합니다.

실차에서 관찰된 `EFLG=0x80/0xC0`은 TEC/REC·MERRF 증가가 없는 RX 버퍼 오버런이었습니다. 펌웨어는 Core 1의 주기 USB 시리얼 출력을 제거하고 B채널 수신 burst 8프레임마다 A채널을 다시 처리하며, 1ms 태스크 양보 직전에도 A 버퍼를 비웁니다. TEC/REC 또는 MERRF가 함께 증가하지 않는 한 배선·종단 오류로 단정하지 마십시오.
- CAN 오류, TEC 상승, TX Fail, BUS-OFF가 보이면 A TX 마스터를 먼저 OFF하고 원인을 확인하십시오.

진단 화면은 A/B 상태 요약을 항상 표시하고, 채널별 상세 카운터와 BUS-OFF 이력은 필요할 때 펼칩니다. Web 실시간 로그는 기본적으로 경고·오류, BUS-OFF/복구, TX Guard, 기능 변경, `USER_MARK`만 보여줍니다. **전체**를 선택하면 최근 수신한 일반 상태 로그도 확인할 수 있으며, **전체 로그 저장** 파일에는 화면 필터와 관계없이 모든 로그가 포함됩니다.

Serial은 115200 baud에서 부팅·OTA·초기화 실패·BUS-OFF/복구·TEC/REC 임계값 전환만 출력합니다. 제거된 `Enable Log` 스위치와 `enable_print` API는 실제 출력 경로를 제어하지 않던 불용 항목이었습니다.

### B채널 `ARB` 해석

`ARB`는 Arbitration Lost 누적값으로, 다른 CAN 프레임에 우선권을 양보한 횟수입니다. 값이 증가해도 B채널 TEC/REC, BUS-ERR, TX-FAIL, BUS-OFF가 모두 0이면 물리 통신 오류로 판정하지 않습니다.

## TSLLC 사용 조건

TSLLC는 Summon 게이트와 별도입니다. Web UI에서 TSLLC를 ON으로 설정하고 A TX 마스터가 ON이며 TX Guard가 비활성일 때, ID `0x3FD` mux 0에서 bit 38과 bit 39를 수정합니다. Summon 토글이나 `Parked || Summoning` 상태는 TSLLC 전송 조건이 아닙니다.

## HW3 Nag Killer 사용 조건

Nag Killer는 CAN-B의 ID 880 원본 프레임을 선택한 모드에 따라 수정해 재송신합니다. 빌드에 기능이 포함되어 있어도 새 NVS, NVS 초기화, OTA 안전 초기화에서는 토글이 **OFF**, 선택 모드가 **MODE 3(기본)**으로 시작합니다. 일반 재부팅에서는 사용자가 저장한 MODE 1/2/3 선택을 보존합니다.

Web UI에서 Nag를 켜기 전에 CAN-B가 정상 수신 중이고 `BUS-OFF=0`, TEC/REC와 TX Fail이 증가하지 않는지 확인하십시오. 모드별 조건은 다음과 같습니다.

| 모드 | 동작 | 필수 조건 |
|---|---|---|
| MODE 1 | +1.80 Nm 고정 에코 | ID 880 DLC 8, 실제 `handsOn=0` |
| MODE 2 | `+1.80, +1.50, -1.50, -1.80 Nm` 200ms 순환, 1초 burst + 1.5초 pause | ID 880 DLC 8, 실제 `handsOn=0` |
| MODE 3(기본) | DAS state 2: 2초 후 0.50~1.80 Nm random walk, state 3: 1초 후 ±1.80 Nm 삼각파 | 921/923·297 age ≤1초, AP 3~6, 297 validity=1, 조향각 ±5°, DAS state 2/3 |

MODE 1/2는 검증된 원본과 같이 921/923·297 최근 수신 유효시간 게이트를 사용하지 않습니다. MODE 3만 어느 하나라도 마지막 수신 후 1초를 넘거나 297 validity가 1이 아니면 과거 상태를 재사용하지 않고 주입을 차단합니다. 모든 모드에 raw `0x74E~0x8B6`(-1.80~+1.80 Nm) 상한, 전체 프레임 자기 에코 차단, `twai_transmit()` 성공 후 카운터 집계가 공통 적용됩니다.

## Signal Observer JSON

Signal Observer는 CAN 프레임을 송신하지 않는 수신 전용 진단 기능입니다. 신호는 A 또는 B 한 채널만 지정해야 하며 혼합 채널 설정은 거부됩니다.

```bash
.venv/bin/python scripts/tcan_signal_observer_json.py SCCM_turnIndicatorStalkStatus UI_autoLaneChangeEnable --output docs/tcan_observer.json
```

Motorola big-endian 신호와 mux 신호도 지원합니다. CAN-A는 하드웨어 필터가 6개이며 기본 Summon 관련 ID가 이미 포함되므로 추가 관찰 ID 예산을 생성 도구의 `aFilterRemaining`으로 확인하십시오.

## OTA 안전 절차

OTA 업로드 전 펌웨어는 차량 기능과 A TX를 OFF로 저장합니다. 이어서 새 A/B 수정 송신을 원자적으로 막고 이미 시작된 `sendCheck()` 호출이 끝날 때까지 최대 250ms 기다린 뒤 B TX 큐를 정리합니다. 이 차단은 OTA 플래시 쓰기 동안 유지됩니다. 업로드가 시작 전 또는 기록 중 실패하면 차단만 해제하고, 차량 기능의 NVS·런타임 상태는 안전값 OFF로 유지합니다.

따라서 **OTA 전에 TX를 끄는 절차는 여전히 필수 권장사항**입니다. 이는 CAN 오류를 치료하는 기능이 아니라 플래시 쓰기·재부팅 경계에서 수정 프레임이 남지 않게 하는 보호 조치입니다. 차량이 정차한 상태에서 업로드하고, 재부팅 뒤에는 A/B 수신, EFLG/TWAI 오류, BUS-OFF가 정상임을 확인한 후 필요한 기능을 하나씩 다시 켜십시오.

| 상태 | 의미 |
|---|---|
| `pending=0` | 정상 동작 |
| `pending=1` | OTA 기록 완료. 다음 부팅에서 안전값 재기록 |
| `pending=2` | 새 펌웨어 60초 확인 창 |
| `pending=3/4` | 이전 펌웨어 rollback과 60초 확인 창 |
| `pending=5` | CAN을 시작하지 않는 복구 Web UI |

NVS, OTA metadata, fallback 파티션, boot 파티션 준비 중 오류가 나면 CAN 드라이버를 시작하지 않고 복구 UI로 전환합니다. 새 펌웨어가 정상 동작하면 60초 안에 Web UI에서 확인을 완료하십시오.

## 구조

```text
Core 1
  nagKillerTask 우선순위 10
    ├─ CAN-A poll → HW3Handler → Summon Unlock/TSLLC
    └─ CAN-B poll → NagHandler → EPAS Nag Killer

Core 0
  Wi-Fi AP, HTTP 서버, CAN alert, 상태 로그, 시계열 수집
```

설정은 NVS `canmod` namespace에 저장됩니다. 차량 영향 설정은 CAN 드라이버와 태스크를 시작하기 전에 모두 읽어 런타임에 반영합니다.

## 개발과 검증

```bash
# HW3 펌웨어 빌드
pio run -e lilygo_t2can

# USB 업로드
pio run -e lilygo_t2can -t upload

# 전체 native 테스트와 B채널 Nag 테스트
pio test -e native
pio test -e native_nag

# Web UI 원본·임베디드 헤더 동기화와 릴리스 메타데이터 검사
python3 scripts/sync_web_ui.py --check
python3 scripts/check_release_metadata.py

# printf 포맷과 diff 검사
python3 scripts/check_printf_formats.py
git diff --check
```

## 버전 정책

- 버전은 [VERSION](VERSION) 파일에서 Semantic Versioning으로 관리합니다.
- 릴리스 변경 이력은 [CHANGELOG.md](CHANGELOG.md)에 기록합니다.
- README, CHANGELOG, 새 공개 사용 문서는 한글로 작성합니다.

## 서드파티 라이선스

전체 라이선스 원문은 [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)에 있습니다.

| 라이브러리 | 라이선스 |
|---|---|
| [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) | MIT |
| [espressif/esp-idf](https://github.com/espressif/esp-idf) TWAI 드라이버 | Apache 2.0 |

## 라이선스

이 프로젝트는 [GNU GPL v3.0](https://www.gnu.org/licenses/gpl-3.0.html)을 따릅니다.
