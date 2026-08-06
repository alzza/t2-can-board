# 변경 이력

이 프로젝트의 주요 변경 사항을 이 파일에 기록합니다.

형식은 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)을 따르며, 버전은 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)을 사용합니다. 이후 새 항목은 한글로 작성합니다.

## [Unreleased]

## [1.3.8] - 2026-08-07

### Summon TX 진단 보강

- Summon/TSLLC가 요청한 A채널 수정 프레임에 송신 출처를 붙여 MCP2515 TX 버퍼 완료 결과까지 연결했다. 기존 송신 조건·비트·주기·TX Guard 임계값과 15초 보호 시간은 변경하지 않았다.
- 이벤트 CSV와 통합 로그에 `A_TX_FAILURE`를 추가했다. 오류마다 `SUMMON`/`TSLLC`, 즉시 큐 거절 또는 TX 버퍼 완료 단계, `TXB0~2`, `TXBnCTRL`의 `TXERR`·`MLOA`·`ABTF`를 기록한다.
- `A_TX_GUARD_SET/CLEAR`의 상세값과 진단 화면에 Guard를 촉발한 기능 조합(`SUMMON`, `TSLLC`, `SUMMON+TSLLC`)을 표시한다.

### Web UI와 기록

- B채널 상태에 `BUS-OFF / BUS-ERR`를 함께 표시하고, BUS-ERR는 CAN 프로토콜 오류 누적이지 BUS-OFF 진입 횟수가 아님을 메인·진단·전용 이력 화면에 명시했다.
- LILYGO T-Display-S3 전용 읽기 API `GET /api/monitor` 스키마 1을 추가했다. 고정 2048바이트 버퍼로 A/B 상태·오류, 기능 토글, AP·Summon 게이트, USER_MARK와 펌웨어 정보를 제공하며 NVS·로그·CAN 송신은 수행하지 않는다.
- T-Display-S3는 별도 저장소에서 `TeslaCAN`에 자동 연결해 1초마다 상태를 읽고, 3.2초 이상 정상 응답이 없으면 `NO SIGNAL`을 표시하도록 연동했다.
- OTA 재부팅 후 보드가 다시 응답하면 브라우저가 캐시 방지 주소로 페이지를 자동 재로드하도록 수정했다.
- 녹색 새 펌웨어 확정·빨간색 복구 확정 화면의 버튼이 브라우저 기본 확인창을 한 번 더 띄우던 중복 확인을 제거했다. OTA 시작 전 CAN TX 차단 안내는 안전을 위해 유지한다.
- 사용하지 않는 신호 관찰기와 관련 Web API, A채널 추가 필터 경로, mock·테스트·생성 도구를 제거했다. A채널 하드웨어 필터는 Summon 게이트에 필요한 5개 ID만 사용한다.
- Web UI를 `상태 / 제어 / 진단 / 기록 / OTA` 5개 화면으로 재구성하고, 중복 제어 스위치와 상단 저장 버튼을 정리했다.
- USER_MARK와 완료 횟수, HW3·펌웨어 버전·빌드 정보를 모든 화면에서 보이는 최상단 요약 영역에 배치했다.
- 통합 로그 다운로드가 약 79.7KB 시계열 스냅샷을 한 번에 할당하던 방식을 제거하고, 링버퍼를 한 행씩 복사·전송하도록 바꿨다.
- 통합 로그 저장 중 화면 갱신 정지를 180초에서 최대 15초로 줄이고 기록 화면에 다운로드 진행·복귀 상태를 표시한다.

### 안전성

- 2026-08-04 B채널 BUS-OFF 제보 로그를 검토해 TWAI 기본 경보값이 `NONE`으로 남아 있던 결함을 수정했다. 장애·복구 경보만 선택 활성화하고 고빈도 ARB_LOST는 누적값으로만 유지한다.
- B채널 BUS-OFF 상태를 수신 큐 고갈 뒤가 아니라 CAN 루프 선두에서 확인해 즉시 복구하고, 복구 성공 후 3초 동안 Nag TX만 정지해 연속 BUS-OFF 재진입을 완화한다. TWAI 상태 확인·경보 폴링·복구는 동일 Core 1 CAN 태스크에서 수행한다.
- 2026-08-03 실차 로그에서 TEC·REC·MERRF가 모두 0인 단발 A TX_FAIL 8건이 각 15초 Guard를 시작한 것을 반영했다. TX_FAIL 사유는 최근 1초에 2회 이상일 때만 Guard를 시작하며, EFLG·TEC 기반 즉시 보호와 15초 보호 시간은 유지한다.
- 2026-08-01 실차 로그에서 `RX1OVR`만 반복된 패턴을 반영해 MCP2515 필터 배치를 조정했다. 주차 상태에서도 계속 들어오는 ID 1021을 RXB0/RXB1 양쪽에 배치하고, ID 921·1016이 RXB1에만 몰리던 구성을 분산했다.
- CAN-A 수신을 프레임 처리보다 먼저 고정 32프레임 RAM 큐로 회수하고, 처리한 프레임마다 MCP2515 RXB0/RXB1을 다시 비우도록 변경했다. CAN-B 프레임도 한 개마다 A채널을 재서비스한다.
- CAN 전용 Core 1의 고정 1ms 대기를 활동 시 50us·유휴 시 100us 짧은 대기로 바꾸고, Idle/WDT 실행을 위해 20ms마다 RTOS 1 tick만 양보하도록 변경했다.
- 5초 상태 문자열 생성과 로그 기록을 CAN 처리 루프에서 Core 0 전용 태스크로 분리해 수신 중 긴 포맷 작업으로 생기던 A 폴링 공백을 줄였다.

### 진단

- BUS-OFF 진입과 복구 결과 기록을 순수 기록기로 분리하고, 복구 성공·실패마다 전용 이력이 정확히 한 행만 추가되며 중복 완료가 추가되지 않는 native 회귀 테스트를 추가했다. CAN 드라이버 호출 순서와 송신·복구 조건은 변경하지 않았다.
- Web UI와 전체 로그에 BUS-OFF 복구 후 B채널 TX 안정화 대기의 잔여시간과 억제된 Nag 송신 수를 추가했다.
- BUS-OFF 이력 CSV의 `recovery_dur_ms`와 `recovered`를 진입 직후가 아니라 실제 복구 성공·실패 확정 뒤 기록하도록 수정하고 복구 시작·완료 시각을 상태 API와 동기화했다.
- 시계열 CSV와 통합 로그에 Summon 게이트의 `AP 활성`, `주차`, `소환 중` 상태를 명시적으로 추가해 허용 여부·차단 사유·AP 안정시간과 한 행에서 비교할 수 있도록 보강.
- 개별 A/B 시계열 CSV를 스키마 4로 올리고 RXB0/RXB1별 수신량·오버런, 실제 선회수 배치/프레임 수, RAM 큐 최대 사용량·드롭, 250us/500us/1ms/2ms 초과 루프 공백과 마지막 오버런 당시 처리 단계를 추가했다.
- Web UI 진단 화면과 전체 로그에 RX 버퍼 편중, 수신 큐 포화, 긴 루프 공백, 오버런 발생 단계를 바로 비교할 수 있는 항목을 추가했다.

### 문서

- T2-CAN과 T-Display-S3를 중첩하지 않는 독립 저장소 구조, Wi-Fi AP/STA 연결, 읽기 전용 API 규칙과 실차 확인 순서를 한글 문서로 추가했다.
- 모든 CSV 로그에 펌웨어 버전과 고유 빌드 ID를 기록하고, 통합 로그의 기존 펌웨어 메타 정보와 같은 빌드를 식별할 수 있게 했다.
- 펌웨어 테스트 후 OTA·실차 테스트용 `.bin` 복사본은 `버전_YY-MM-DD_짧은변경요약.bin` 형식으로 자동 생성하도록 README와 작업 지침을 갱신.
- 2026-08-01 실차 OTA에서 차량 통신에러가 발생하지 않은 TX 정지 순서를 README, OTA 검증 절차, 작업 지침에 회귀 기준으로 기록.

## [1.3.7] - 2026-07-31

### 변경

- HW3 bit 19/46 송신 허용 조건을 Web UI/API/NVS의 ON/OFF 실험 옵션으로 추가. ON은 `Parked || Summoning || AP 상태 3~6 안정 1초`, OFF는 주행 상태와 관계없이 수신한 ID `0x3FD` mux 1마다 수정 송신을 시도.
- bit 19는 ECE R79 제한 해제, bit 46은 HW3 Summon 제한 해제로 확정하고 같은 mux 1 프레임에서 함께 적용.
- 최신 검증 원본에서 제거된 Nag MODE 3과 런타임 ID 297 경로를 펌웨어·Web UI·로그에서 삭제하고, 과거 NVS MODE 3 값은 MODE 2 기본값으로 치환.

### 안전성

- OTA 직전 CAN-A를 MCP2515 Listen-Only로, CAN-B를 TWAI 정지 상태로 전환해 물리 TX를 차단.
- OTA 시작·수신·기록·검증 실패 시 CAN TX를 재개하지 않고 재부팅 전까지 fail-closed 유지.
- OTA 첫 부팅의 기능 OFF 안전값을 CAN 드라이버 초기화 전에 적용하고, MCP2515 초기화 뒤 CAN 처리 시작 전 불필요한 2초 공백을 제거해 RX 오버런 가능성을 축소.

### 진단

- A채널 송신 결과를 큐 등록, TX 버퍼 사용 중, 하드 오류, 실제 완료, 중재 손실, 중단으로 분리.
- TX 버퍼 사용 중은 통신 하드 오류와 TX Guard 트리거에서 제외하고, 하드 오류는 1회부터 Guard가 개입하도록 변경.
- 시계열 CSV를 스키마 3으로 갱신하고 송신 허용 조건·AP 안정시간·게이트 사유 및 A채널 상세 송신 결과를 추가.
- 시계열 CSV 포맷 문자열의 불필요한 두 필드로 인해 토크 이후 열이 밀릴 수 있던 문제를 수정하고 헤더와 데이터 열 수를 일치시킴.
- 독립 CSV와 통합 로그의 헤더·데이터 열 수를 릴리스 전에 자동 대조하는 검사 스크립트 추가.

## [1.3.6] - 2026-07-30

### 변경

- 최신 `nag-killer` 원본에서 이전 Mode C가 제거된 점과 실차 결과를 반영해 MODE 2 + AP 전용을 새 기본·권장값으로 지정하고 MODE 3은 레거시 비권장으로 유지.
- MODE 1/2에 검증 원본 선제 주입 범위와 AP 상태 3~6 전용 범위를 선택하는 Web UI/API/NVS 토글 추가.
- 실제 EPAS hands-on 값이 0이 아니면 모든 모드가 수정 송신하지 않도록 공통 안전 조건을 통일.
- `USER_MARK`를 AP 경고 전용이 아닌 일반 로그 분석 구간 마커로 정의하고 `USER_MARK_START/END` 한 쌍당 완료 횟수를 1 증가.
- USER_MARK 완료 횟수, 진행 상태와 원문은 로그 초기화와 새 기록 시작에도 유지하고 보드 재부팅 때만 초기화.
- 메인 화면에 EU Unlock/Summon, TSLLC, Nag Killer 스위치를 배치하고 Nag 주입 범위를 함께 표시.
- `Summon-Unlock`과 `nag-killer` 최신 원본 주소·확인 커밋을 프로젝트 컨텍스트 노트에 고정.

### 추가

- Summon/TSLLC/Nag의 기능 스위치, 주입 준비, 실제 송신 증분을 전체 로그·시계열 CSV·이벤트 CSV·자가 진단 CSV에 공통 기록.
- A채널 TX Fail의 최근 1초 증가량·세션 피크·보호 임계값을 상태 API, Web UI, CANMOD 로그와 자가 진단에 추가.

### 수정

- iPhone Safari에서 9~11px 글꼴이 혼재하고 화면이 과도하게 압축되던 Web UI를 12~16px 공통 타이포그래피, 44px 터치 영역, Safe Area, Safari 글자 크기 보정 및 모바일 한 열 카드 배치로 재구성.
- MCP2515 수신 오버런 정리 시 `CANINTF` 전체를 지우던 라이브러리 경로를 사용하지 않고 `RX0OVR/RX1OVR`와 `ERRIF`만 지워, 새로 도착한 `RX0IF/RX1IF`가 유실될 수 있는 문제를 수정.
- One-shot의 TEC/MERRF 없는 단발 TX 실패는 이력만 남기고, 1초 안에 2회 이상 실패할 때만 TX Fail 사유의 15초 Guard를 시작하도록 과민 차단을 완화. EFLG·TEC 이상은 기존처럼 즉시 보호.
- Web UI의 누적 TX Fail이 한 번 증가한 뒤 영구 경고로 보이던 표시를 최근 1초 실패량이 Guard 임계값에 도달했을 때만 경고하도록 수정.

### 확인

- `Summon-Unlock` 2026-07-29 최신 원본은 대시보드 파일만 추가됐으며 `Parked || Summoning`, mux 1 bit 19, HW3 bit 46 포팅 로직에는 변경 없음.

## [1.3.5] - 2026-07-24

### 변경

- 새 NVS, NVS 초기화, OTA 안전 초기화에서 Nag Killer를 OFF로 저장하고 MODE 3을 기본 선택.
- 기존 `[기본]/A~D` 실험 프로파일을 폐기하고 검증된 MODE 1/2/3 선택으로 Web UI와 API를 개편.
- 메인 화면을 A채널 Summon/TSLLC와 B채널 Nag Killer 상태 카드 중심으로 재배치.
- CAN 자가 진단을 A/B 동시 검사와 HW3 DAS 921/923 경로 기준으로 개편.
- 진단 화면을 A/B 요약 카드 우선으로 바꾸고 채널별 상세 카운터와 BUS-OFF 이력은 기본 접힘으로 배치.
- Web 실시간 로그는 중요 항목을 기본 표시하고 사용자가 전체 로그로 전환할 수 있도록 변경.
- Serial은 부팅·OTA·초기화 실패·BUS-OFF/복구·TEC/REC 임계값 전환만 출력하도록 정리.

### 수정

- 개별 진단 CSV를 단일 헤더의 UTF-8 CSV v2로 개편하고 모든 행에 실제 시각과 업타임을 함께 기록.
- 이벤트 CSV에 A/B/SYSTEM 채널, 심각도, 최초·최종 시각, 반복 횟수, 플래그 해석을 추가하고 반복 이벤트를 30초 단위로 집계.
- 자가 진단 미실행 시 헤더만 저장되던 문제를 `NOT_RUN` 안내 행으로 수정.
- 수동 시계열 기록을 정지한 뒤에도 샘플이 계속 추가되던 문제를 수정하고, 기본 자동 최근 20분/수동 구간 고정 동작을 분리.
- Core 1의 주기 USB 시리얼 출력을 제거하고 B 수신 burst 중 A MCP2515를 재서비스해 A RX 오버런 가능성을 축소.
- A 폴링 공백 최근·최대·2ms 초과 횟수를 API, Web UI, 시계열 CSV, 이벤트 CSV, 자가 진단에 추가.
- 동작 경로가 없던 `Enable Log` UI, `enable_print` 상태 필드와 API를 제거.
- Web 실시간 로그의 업타임 시각을 브라우저 실제 시각 기준으로 표시.
- OTA 시작 시 A/B 수정 송신 호출을 원자적으로 차단하고, 진행 중 호출 종료 확인 후 B TX 큐를 비우도록 보강.
- 모든 Nag 모드의 최종 송신 토크를 -1.80~+1.80 Nm로 제한.
- MODE 3에서만 DAS 921/923과 조향각 297의 마지막 수신 후 1초 이내 조건, AP 3~6, validity=1, 조향각 ±5° 게이트를 적용.
- `twai_transmit()`이 송신 요청을 성공으로 반환한 경우에만 echo·주입 성공 카운터와 마지막 송신 시각을 갱신.
- 성공적으로 송신한 마지막 echo 전체 프레임이 수신된 경우 재처리를 차단.
- A채널 `driver_ok`가 핸들러 존재 여부가 아니라 실제 MCP2515 초기화 결과를 사용하도록 수정.
- A채널 일반 `WARN`을 RX 오버런·오류 경고·에러 패시브·BUS-OFF로 구분하고 EFLG 발생·해제 이벤트를 기록.
- A/B 시계열 CSV, CAN 자가 진단 CSV, 재수신부터 첫 Summon TX까지의 지연 계측을 추가.
- 삭제된 화면 요소를 참조하던 Web UI 코드와 동작하지 않는 Single Shot/BUS-OFF stop 실험 경로를 제거.

## [1.3.4] - 2026-07-22

### 추가

- 검증된 `summon_unlock.ino` 동작을 기준으로 한 HW3 조건부 Summon Unlock과 전용 Web UI 제어·진단.
- Parked, Summoning, ACA, SPR, CAN 수신 카운터, CAN-A TX 성공·실패의 Summon 게이트 진단.
- 지원 상태와 범위 밖 `ota_pending` 값을 검사하는 OTA 부팅 정책 테스트.

### 변경

- CAN-A 역할을 Summon Unlock과 TSLLC로, CAN-B 역할을 Nag Killer로 명확히 분리.
- 일반 Web UI 편집 원본을 `web/web_ui.html`로 두고 펌웨어 헤더와 동기화.

### 수정

- 차량 영향 NVS 설정을 CAN 드라이버와 태스크 시작 전에 모두 로드.
- OTA 첫 부팅, rollback, 복구 경로에서 기능 OFF/stock 안전값을 다시 기록하고 B채널 TX 큐를 정리.
- NVS, OTA metadata, fallback 파티션, boot 파티션 오류 시 CAN을 시작하지 않는 복구 UI로 fail-closed 처리.

## [1.3.2] - 2026-05-13

### 수정

- OTA 업로드 뒤 cache-busting 헤더를 사용한 자동 새로고침.
- OTA metadata 표시 형식을 `YY-MM-DD HH:MM:SS`로 축약.

## [1.2.0] - 2026-05-07

### 추가

- Core 1의 `nagKillerTask` 우선순위 10에서 CAN-A MCP2515와 CAN-B TWAI를 통합 폴링.
- OTA cache-disable 구간의 RX FIFO 보호를 위한 TWAI ISR IRAM 플래그.
- CAN-B용 FSM 기반 적응형 Nag Killer Mode B.
- 첫 부팅 NVS sentinel 초기화, BUS-OFF 이벤트 로그, 5초 시계열 수집과 CSV 다운로드.
- CAN 스택을 시작하지 않는 OTA 복구모드와 OTA watchdog.
- 런타임 시리얼 로그 토글과 Web UI 체크박스 동기화 보완.

### 변경

- `loop()`를 즉시 `vTaskDelete(NULL)`하는 구조로 단순화.
- MCP2515 기본 SPI를 10 MHz로 설정하고 Web UI에서 8 MHz fallback 선택 지원.
- CAN 처리 태스크의 Core·우선순위 설명 정리.

### 수정

- Web UI의 A SPI, one-shot, A TX Guard 체크박스가 새로고침 후 서버 상태와 불일치하던 문제.

## [1.1.0] - 2026-04-06

### 추가

- M5Stack AtomS3 Mini CAN Base 지원과 Summon 관련 확장 오토파일럿 기능.

### 수정

- HW3Handler의 오래된 speed-profile 매핑과 NagHandler 토크 프레임 처리.
- 신규 기능에 맞춘 Web UI.

## [1.0.0] - 2026-04-05

### 추가

- HW3/HW4 FSD 관련 CAN 기능, 오토스티어 Nag 억제, ISA 속도 경고음 억제, 긴급 차량 감지, 속도 프로필, Smart Summon, Web UI, OTA.
- MCP2515, SAME51, TWAI CAN 드라이버 추상화와 여러 ESP32·Feather·M5Stack 보드 지원 기반.
- CAN 전송 DLC 검증, bit 범위 검사, TX cooldown·복구, native NagHandler 테스트.

### 수정

- Nag 토크 값과 CAN counter/checksum 처리.
- FSDEnabled 변수 가림과 TWAI TX timeout.

### 변경

- 빌드 플래그를 `BYPASS_TLSSC_REQUIREMENT`로 정리하고 펌웨어 설정을 `sketch_config.h`로 통합.
