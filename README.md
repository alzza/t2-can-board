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
| Wi-Fi | AP 모드 | - | Web UI, OTA, 로그·진단 다운로드, 읽기 전용 외부 모니터 API |

> [!IMPORTANT]
> 차량 CAN 버스는 이미 종단되어 있습니다. 별도 CAN 트랜시버 보드의 120 Ω 종단 저항은 제거하거나 우회해야 합니다. 중복 종단은 통신 오류와 BUS-OFF를 유발할 수 있습니다.

## 기능

| 기능 | 채널 | 설명 |
|---|---|---|
| ECE R79 / Summon 제한 해제 | CAN-A | HW3에서 ID `0x3FD` mux 1의 bit 19와 bit 46을 함께 수정. 송신 허용 조건 ON/OFF로 실험 범위를 선택 |
| ECE R79 해제 | CAN-A | 같은 수정 프레임에서 bit 19를 0으로 하는 동작. AP 주행 중에도 AP 안정 1초 후 적용 |
| TSLLC | CAN-A | ID `0x3FD` mux 0의 TSLLC·녹색 신호 계속 주행 비트 수정 |
| Nag Killer | CAN-B | EPAS 토크 프레임 ID 880을 조건에 맞춰 수정·재송신 |
| Web UI·OTA | Wi-Fi | 실시간 CAN 상태, 런타임 토글, OTA 및 복구 UI |
| T-Display-S3 모니터 | Wi-Fi | 경량 `GET /api/monitor`로 A/B·기능·게이트 상태를 읽기 전용 제공 |

### 외부 검증 원본

CAN 로직을 변경하거나 실차 로그를 판정할 때 다음 최신 원본을 먼저 비교합니다.

- [Nag Killer 저장소](https://github.com/06066060606060/nag-killer) / [기준 INO](https://github.com/06066060606060/nag-killer/blob/main/Nag-killer.ino)
- [Summon Unlock 저장소](https://github.com/06066060606060/Summon-Unlock) / [기준 INO](https://github.com/06066060606060/Summon-Unlock/blob/main/summon_unlock.ino)

Web UI·BLE·Wi-Fi 구현 차이와 CAN 알고리즘 차이를 분리해서 검토하며, 원본에
없는 CAN ID·비트·주기·송신 조건은 사용자 확인 없이 추가하지 않습니다.

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

- Wi-Fi AP **`TeslaCAN`**에 연결합니다. 비밀번호는 **`asdf1234`**입니다.
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

대시보드는 `상태 / 제어 / 진단 / 기록 / OTA` 5개 화면으로 구성합니다.

- A/B 채널별 프레임 속도, RX/TX, TEC/REC, BUS-OFF와 A채널 EFLG 원인 실시간 표시.
- Summon Unlock, TSLLC, Nag Killer와 A TX 마스터 런타임 제어.
- A/B 시계열·CAN 자가 진단·BUS-OFF 이벤트 CSV 다운로드.
- 차량 재수신부터 첫 Summon TX 성공까지의 지연 표시.
- OTA 업로드, 60초 확인, rollback·복구 UI.
- iPhone Safari Safe Area와 글자 크기 보정을 적용하고, PC와 동일한 색상·카드·상태 체계를 모바일 한 열 배치로 제공.

Web UI의 일반 본문·보조 정보는 최소 12px, 주요 조작 영역은 44px 이상을
사용합니다. iPhone에서는 채널과 제어 카드를 한 열로 재배치하지만 상태 색상,
정보 우선순위와 조작 방식은 PC 화면과 동일합니다.

### T-Display-S3 읽기 전용 모니터

별도 저장소 `/Users/akanus/T-Display-S3`의 LILYGO T-Display-S3가 이 보드의 AP에 STA로 자동 연결할 수 있습니다.

| 항목 | 값 |
|---|---|
| 상태 주소 | `GET http://192.168.4.1/api/monitor` |
| 스키마 | `1` |
| 권장 폴링 | 1초 |
| 응답 방식 | 2048바이트 고정 버퍼 JSON, `Cache-Control: no-store` |
| 외부 모니터 권한 | 읽기 전용 |

이 API는 A/B 채널 상태·오류, ECE R79/Summon/TSLLC/Nag 상태, AP·Summon 게이트, USER_MARK와 펌웨어 정보를 제공합니다. 요청을 처리할 때 NVS·로그를 기록하거나 CAN 송신을 수행하지 않습니다. T-Display-S3 쪽에도 기능 제어, CAN 송신, 설정 POST, OTA 요청 경로를 두지 않습니다.

`/api/monitor`는 Web UI의 상세 `/api/status`보다 작고 고정된 응답입니다. 외부 화면에서 `/api/status`를 빠르게 반복 호출해 Web 서버·힙·로그 다운로드에 부담을 주는 것을 피하기 위한 전용 경로입니다.

## HW3 ECE R79 / Summon 제한 해제 사용법

### 먼저 알아둘 점

이 기능은 차량에 Summon 명령을 새로 보내지 않습니다. 차량이 CAN-A에 보낸 ID `0x3FD`의 mux 1 프레임을 관찰하고, 아래 모든 조건이 맞을 때만 해당 프레임을 수정하여 재송신합니다. 조건이 하나라도 맞지 않으면 Summon 수정 프레임은 전송하지 않습니다.

이 기능은 같은 ID `0x3FD` mux 1 프레임에서 두 비트를 함께 적용합니다. bit 19는 ECE R79 적용을 해제하고, HW3 bit 46은 Summon 제한을 해제합니다. 두 비트는 별도 토글로 나누지 않으며 검증 원본과 ev-open-can-tools HW3 방식처럼 함께 송신합니다.

### 활성화 순서

1. 차량을 정차한 뒤 `TeslaCAN` Wi-Fi와 Web UI에 연결합니다.
2. CAN-A 수신이 안정적이고 `BUS-OFF=0`인지 확인합니다.
3. **제어** 화면에서 **ECE R79 / Summon 제한 해제 (HW3)**를 ON으로 설정합니다.
4. CAN-A 카드의 **A TX 마스터**를 ON으로 설정합니다.
5. Summon 카드에서 `Enabled`, `Active`, `Gate OPEN`과 `Q / Busy / Hard`를 확인합니다.
6. 기본값인 **송신 허용 조건 ON**에서 `PARKED`, 실제 `SUMMONING`, 또는 AP 상태 3~6이 1초 이상 안정된 경우에만 수정 프레임이 전송되는지 확인합니다.

OTA 직후 첫 부팅에서는 Summon Unlock, TSLLC, Nag Killer, A TX 마스터가 모두 OFF/stock 값으로 저장됩니다. 수신 상태를 확인하고 OTA 확인을 완료한 후 필요한 기능을 하나씩 활성화하십시오.

OTA 업로드 후 보드가 재부팅되어 다시 응답하면 Web UI가 자동으로 새로 고침됩니다. 이후 녹색 `새 펌웨어 확정` 또는 빨간색 `복구 확정` 화면에서 한 번만 선택하며, 같은 내용의 브라우저 기본 확인창은 추가로 표시되지 않습니다. OTA 시작 전 CAN TX 차단 안내창은 안전 절차로 계속 표시됩니다.

### 실제 전송 조건

| 조건 | 근거 |
|---|---|
| `SUMMON_UNLOCK` 빌드 포함 | 기본 HW3 빌드에서 활성화됨 |
| 수신 프레임 | CAN-A ID `0x3FD`(10진수 1021), mux 1, DLC 8바이트 |
| 기능 토글 | Web UI의 ECE R79 / Summon 제한 해제가 ON |
| A TX 마스터 | CAN-A 수정 송신이 ON |
| TX Guard | 보호 모드가 비활성 상태 |
| 송신 허용 조건 ON(기본·권장) | `Parked || Summoning || AP 상태 3~6 안정 1초` |
| 송신 허용 조건 OFF(실험) | 주행 상태와 관계없이 수신한 mux 1마다 수정 송신 시도 |
| 드라이버 전송 | MCP2515 큐 등록 `Q`, 버퍼 사용 중 `Busy`, 하드 오류 `Hard`, 실제 완료 `Done`을 별도 집계 |

`TX Guard`가 활성화되었거나 A TX 마스터가 OFF이면 Summon과 TSLLC 모두 수정 송신을 건너뜁니다. Summon 토글은 ON인데 게이트가 닫혀 있으면 `Blocked`만 증가하고 송신하지 않습니다.

### 게이트 판정 방식

| 상태 | CAN 근거 | 판정 |
|---|---|---|
| Parked | ID 280의 `DI_gear` | 기어값 P일 때 열림. R/N/D이면 닫힘 |
| Parked 보조 | ID 390의 `DIF_gear` | ID 280이 처음부터 없거나 5초 이상 없을 때만 보조 적용 |
| Summoning | ID 280 ACA + ID 1016 SPR | `DI_autonomyControlActive`와 `UI_selfParkRequest` 관측이 모두 있을 때 열림 |
| AP 상태 | ID 921 | 상태 3~6이 연속 1초 이상 유지되면 ON 조건 게이트를 엶 |

INO 기준과 동일하게 ID 280이 5초 이상 수신되지 않으면 보드는 Parked로 간주합니다. 따라서 센트리·절전 등으로 프레임이 끊긴 상태에서는 UI의 수신 카운터와 마지막 280 프레임 경과 시간을 함께 확인하십시오.

**송신 허용 조건 OFF**는 Tesla 펌웨어별 동작 차이를 비교하기 위한 실험값입니다. OFF여도 기능 마스터, A TX 마스터, TX Guard 조건은 그대로 적용됩니다. 정상 동작 조합을 확인하기 전까지는 ON을 권장합니다.

### HW3에서 수정되는 비트

| CAN ID | mux | 비트 | 값 | 의미 |
|---|---|---|---|---|
| `0x3FD` | 1 | 19 | 0 | `UI_applyEceR79` 해제. AP 주행 중 ECE R79 제한 해제 |
| `0x3FD` | 1 | 46 | 1 | 검증된 HW3 Summon 제한 해제 비트 |

HW4 INO 경로는 bit 47을 사용하지만, 이 저장소의 기본 빌드는 HW3이므로 bit 46만 사용합니다.

### 즉시 정지 방법

- **ECE R79 / Summon 제한 해제**를 OFF하면 다음 mux 1 프레임부터 bit 19/46 수정 송신을 중단합니다.
- **A TX 마스터**를 OFF하면 Summon과 TSLLC의 CAN-A 수정 송신을 함께 중단합니다.

### A채널 `WARN` 확인 방법

메인 화면의 A채널 경고는 MCP2515의 현재 EFLG를 기준으로 `수신 오버런`, `오류 경고`, `에러 패시브`, `BUS-OFF`로 나뉩니다. 경고가 보이면 **진단** 화면에서 현재/피크 EFLG, TEC/REC, RX-OVR, MERRF, TX Fail을 확인하고 다음 파일을 저장하십시오.

1. **자가 진단 결과 CSV**를 저장합니다. 진단을 실행하지 않았다면 `NOT_RUN` 안내 행이 저장됩니다.
2. **A/B 상태 시계열 CSV**를 저장합니다.
3. **채널별 이벤트 CSV**를 저장합니다.
4. 분석할 이벤트나 구간의 시작과 종료 지점에서 각각 `USER_MARK`를 누른 뒤 **전체 로그 저장**도 실행합니다.

`RX_OVERRUN`은 MCP2515의 2개 수신 버퍼가 소프트웨어가 비우기 전에 가득 찼다는 뜻입니다. 경고를 숨기거나 임계값을 완화하지 않으며, 반복 발생 여부는 CSV의 `a_rx_overrun`, `a_rx0_overrun`, `a_rx1_overrun`, `a_eflg`, `a_loop_gap_last_us`, `a_loop_gap_peak_us`, `a_d_loop_gap_over_2ms` 열로 판단합니다. 오버런 이벤트의 `detail_text`에는 당시 최대 `loop_gap_us`가 함께 저장됩니다. 오버런 정리 시에는 EFLG의 오버런 비트와 ERRIF만 지우고, 새 프레임 도착을 알리는 RX0IF/RX1IF는 보존합니다.

새 저장 형식은 모든 행에 실제 시각과 업타임을 함께 기록하고, `a_`, `b_`, `system_` 접두사로 채널을 구분합니다. 기본 상태에서는 최근 20분을 자동 보관합니다. **기록 시작**은 기존 로그를 지우고 수동 구간을 시작하며, **기록 정지**는 이후 샘플 추가를 멈춰 그 구간을 고정합니다.

실차에서 관찰된 `EFLG=0x80/0xC0`은 TEC/REC·MERRF 증가가 없는 RX 버퍼 오버런이었습니다. 현재 펌웨어는 ID 1021을 MCP2515 RXB0/RXB1 양쪽에 배치해 필터 부하를 분산하고, 프레임 해석 전에 하드웨어 수신 버퍼를 32프레임 RAM 큐로 먼저 회수합니다. B채널 프레임 하나마다 A채널을 다시 처리하며, CAN 태스크는 고정 1ms 대기 대신 50~100us 짧은 대기와 20ms 주기 RTOS 양보를 사용합니다. 5초 상태 문자열 생성은 별도 Core 0 태스크에서 처리합니다. TEC/REC 또는 MERRF가 함께 증가하지 않는 한 배선·종단 오류로 단정하지 마십시오.

시계열 CSV 스키마 4에서는 `a_rx_buffer0_frames`/`a_rx_buffer1_frames`로 두 하드웨어 버퍼의 수신 편중을, `a_rx_queue_high_water`/`a_rx_queue_drops`로 RAM 큐 여유를, `a_loop_gap_over_250us`/`500us`/`1ms`/`2ms`로 처리 공백 분포를 확인합니다. `a_last_overrun_phase`는 마지막 오버런을 발견한 처리 단계를 나타냅니다. `a_rx_drain_calls`는 빈 폴링을 제외하고 실제 프레임을 회수한 배치 수입니다.

MCP2515 one-shot과 TX Guard는 계속 켜는 것을 권장합니다. One-shot에서는 단발 TX 결과가 TEC/MERRF 증가 없이 기록될 수 있으므로, 큐 등록이나 과거 누적값만으로 현재 통신 장애를 판정하지 않습니다. `Busy`는 MCP2515의 TX 버퍼 3개가 사용 중이라는 뜻이며 하드 오류나 Guard 트리거로 집계하지 않습니다. `Hard`가 최근 1초에 **2회 이상** 발생하면 Guard를 시작하고, BUS-OFF/EFLG 또는 TEC 임계값 이상도 즉시 Guard를 시작합니다.

- CAN 오류, TEC 상승, TX Fail, BUS-OFF가 보이면 A TX 마스터를 먼저 OFF하고 원인을 확인하십시오.

진단 화면은 A/B 상태 요약을 항상 표시하고, 채널별 상세 카운터와 BUS-OFF 이력은 필요할 때 펼칩니다. Web 실시간 로그는 기본적으로 경고·오류, BUS-OFF/복구, TX Guard, 기능 변경, `USER_MARK`만 보여줍니다. `USER_MARK`는 차량 상태를 자동 판정하지 않는 일반 분석 마커입니다. 첫 클릭은 `USER_MARK_START`, 다음 클릭은 `USER_MARK_END`를 남기며 START↔END 한 쌍이 끝날 때 완료 횟수가 1회 증가합니다. 로그 초기화나 새 기록 시작으로 마커 상태·횟수·원문은 지워지지 않고 보드 재부팅 때만 초기화됩니다. **전체**를 선택하면 최근 수신한 일반 상태 로그도 확인할 수 있으며, **전체 로그 저장** 파일에는 화면 필터와 관계없이 모든 로그가 포함됩니다.

B채널의 `BUS-ERR`는 CAN 프로토콜 오류 누적값이고 `BUS-OFF` 진입 횟수가 아닙니다. Web UI는 두 값을 `BUS-OFF / BUS-ERR` 순서로 함께 표시합니다. 실제 BUS-OFF가 발생하면 진입 이벤트는 즉시 이벤트 CSV에 남고, 복구 성공 또는 실패가 확정된 뒤 BUS-OFF 전용 이력에 한 행이 추가됩니다. BUS-ERR만 증가한 경우 전용 BUS-OFF 이력이 비어 있는 것이 정상입니다.

Serial은 115200 baud에서 부팅·OTA·초기화 실패·BUS-OFF/복구·TEC/REC 임계값 전환만 출력합니다. 제거된 `Enable Log` 스위치와 `enable_print` API는 실제 출력 경로를 제어하지 않던 불용 항목이었습니다.

### B채널 `ARB` 해석

`ARB`는 Arbitration Lost 누적값으로, 다른 CAN 프레임에 우선권을 양보한 횟수입니다. 값이 증가해도 B채널 TEC/REC, BUS-ERR, TX-FAIL, BUS-OFF가 모두 0이면 물리 통신 오류로 판정하지 않습니다.

2026-08-04 실차 로그에서는 Nag 주입 5,858회 동안 `ARB=3,446`, `BUS-ERR=5`가 기록됐지만 저장된 20분 구간의 `BUS-OFF/TEC/REC/TX-FAIL`은 모두 0이었습니다. 따라서 그 파일만으로 화면에서 보였던 BUS-OFF를 확정할 수는 없습니다. 당시 코드에서 TWAI 경보가 기본값 `NONE`으로 남아 있어 별도 경보 태스크가 실제 이벤트를 받지 못한 기록 결함을 확인했습니다.

현재 펌웨어는 BUS-OFF·복구·에러 패시브·BUS-ERR·TX-FAIL·RX 큐 포화 경보만 활성화하고, 고빈도 `ARB_LOST` 경보는 누적 카운터로만 관찰합니다. BUS-OFF 상태는 수신 큐가 빌 때까지 기다리지 않고 CAN 태스크가 즉시 확인·복구하며, 복구 성공 후 3초 동안 B채널 Nag TX만 정지합니다. 이 안정화 대기는 수신을 중단하지 않으며 Web UI의 `RECOVERY QUIET`과 전체 로그의 `RecoveryQuiet`에서 잔여시간·억제 횟수를 확인할 수 있습니다.

## TSLLC 사용 조건

TSLLC는 Summon 게이트와 별도입니다. Web UI에서 TSLLC를 ON으로 설정하고 A TX 마스터가 ON이며 TX Guard가 비활성일 때, ID `0x3FD` mux 0에서 bit 38과 bit 39를 수정합니다. Summon 토글이나 `Parked || Summoning` 상태는 TSLLC 전송 조건이 아닙니다.

## HW3 Nag Killer 사용 조건

Nag Killer는 CAN-B의 ID 880 원본 프레임을 선택한 모드에 따라 수정해 재송신합니다. 빌드에 기능이 포함되어 있어도 새 NVS, NVS 초기화, OTA 안전 초기화에서는 토글이 **OFF**, 선택 모드는 **MODE 2**, 주입 범위는 **오토파일럿 전용(기본)**으로 시작합니다. 일반 재부팅에서는 사용자가 저장한 모드와 주입 범위를 보존합니다.

Web UI에서 Nag를 켜기 전에 CAN-B가 정상 수신 중이고 `BUS-OFF=0`, TEC/REC와 TX Fail이 증가하지 않는지 확인하십시오. 모든 모드는 ID 880의 실제 EPAS `handsOn=0`일 때만 주입하며, 운전자의 손이 감지되면 즉시 감시 전용으로 전환합니다.

| 모드 | 동작 | 필수 조건 |
|---|---|---|
| MODE 1 | +1.80 Nm 고정 에코 | ID 880 DLC 8, 실제 `handsOn=0`, 선택한 주입 범위 |
| MODE 2(기본·권장) | `+1.80, +1.50, -1.50, -1.80 Nm` 200ms 순환, 1초 주입 + 1.5초 휴지 | ID 880 DLC 8, 실제 `handsOn=0`, 선택한 주입 범위 |

MODE 1/2의 **주입 범위** 토글은 다음 두 동작을 제공합니다.

- **오토파일럿 전용 ON(기본·권장)**: 최근 1초 안에 수신한 DAS AP 상태가 3~6일 때만 원본 토크 패턴을 주입합니다. 일반주행에서는 감시만 합니다.
- **오토파일럿 전용 OFF**: 검증 원본처럼 DAS·조향각 조건 없이 ID 880과 실제 `handsOn=0`만으로 선제 패턴을 사용합니다.

최신 Nag Killer 원본에서 제거된 MODE 3과 실험용 ID 297 경로는 펌웨어, Web UI, NVS 설정에서 삭제했습니다. 과거 NVS에 MODE 3 값이 남아 있으면 부팅 시 MODE 2 기본값으로 치환해 저장합니다.

모든 모드에 raw `0x74E~0x8B6`(-1.80~+1.80 Nm) 상한, 최근 송신 전체 프레임 자기 에코 차단, `twai_transmit()` 성공 후 카운터 집계가 공통 적용됩니다. 원본의 `handsOn<=1`보다 보수적으로 실제 `handsOn=0`만 허용하는 것은 사용자가 확정한 공통 안전 조건입니다.

시계열 CSV는 각 5초 구간의 Summon/TSLLC/Nag 실제 송신 증분, Summon 게이트,
허용 여부·차단 사유·AP 상태·AP 활성·안정시간·주차·소환 중 상태, TX Guard,
AP 전용 설정을 함께 기록합니다. 이벤트 CSV의
`FEATURE_STATE`는 스위치 변경을, `FEATURE_ACTIVITY`는 실제 송신 활동 전이를
기록하며, 전체 로그와 자가 진단에도 같은 기능 설정·준비·송신 카운터가 포함됩니다.

`기록` 화면의 전체 로그 저장은 시계열 전체를 별도 메모리에 복사하지 않고 한 행씩
전송합니다. 저장을 누른 뒤 화면 상태 갱신은 최대 15초만 멈추며 자동으로 다시
시작합니다. 다운로드가 진행되는 동안 버튼을 반복해서 누르지 말고 Safari의 다운로드
표시가 나타날 때까지 기다리십시오.

## OTA 안전 절차

OTA 업로드 전 펌웨어는 차량 기능과 A TX를 OFF로 저장합니다. 이어서 새 A/B 수정 송신을 원자적으로 막고 이미 시작된 송신 호출이 끝날 때까지 최대 250ms 기다립니다. 그 다음 CAN-A MCP2515는 Listen-Only, CAN-B TWAI는 정지 상태로 전환해 물리 TX를 차단합니다. OTA 시작·수신·기록·검증 중 하나라도 실패하면 TX를 다시 켜지 않고 재부팅 전까지 fail-closed 상태를 유지합니다.

따라서 **OTA 전에 사용자가 Web UI의 A TX와 Nag Killer를 먼저 끄는 절차도 계속 권장**합니다. 자동 차단은 이중 보호이며 CAN 오류를 사후 치료하는 기능은 아닙니다. 차량을 P에 두고 업로드한 뒤, 새 펌웨어는 기능 OFF 안전값을 CAN 초기화 전에 적용합니다. 재부팅 직후에는 어떤 송신 기능도 켜지 말고 A/B 수신, EFLG/TWAI 오류와 BUS-OFF가 정상임을 확인한 후 OTA 확인을 완료하고 필요한 기능을 하나씩 다시 켜십시오.

2026-08-01 실차 OTA에서는 위 순서를 적용한 뒤 차량 통신에러가 발생하지 않았습니다. OTA 관련 코드를 바꿀 때는 `새 송신 차단 → 기능 NVS OFF 저장 → 기존 송신 종료 확인 → A Listen-Only/B Stopped → 실패 시 fail-closed → 첫 부팅 기능 OFF` 순서를 회귀 검증 기준으로 유지하십시오.

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
python3 scripts/check_timeseries_csv_schema.py
git diff --check
```

### 펌웨어 bin 산출물 이름 규칙

펌웨어 테스트 후 OTA 또는 실차 테스트에 사용할 `.bin` 복사본은 아래 형식으로 이름을 남깁니다.

```text
YYYY-MM-DD_짧은변경요약.bin
```

예시입니다.

```text
2026-07-31_ece-r79-gate-log.bin
2026-07-31_ota-tx-safe.bin
```

날짜는 KST 기준 빌드·테스트 당일 날짜를 쓰고, 변경 요약은 오늘 수정한 핵심 내용을 공백 없이 최대한 짧게 기록합니다. PlatformIO 원본 산출물 `.pio/build/lilygo_t2can/firmware.bin`은 그대로 두고, OTA 업로드·보관용 복사본에만 이 이름을 붙입니다.

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
