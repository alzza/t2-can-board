---
sidebar_position: 99
---

# 변경 이력

Tesla Open CAN Mod의 주요 변경 사항을 기록합니다. 형식은 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)을 따르며 버전은 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)을 사용합니다.

## [Unreleased]

## [1.3.5] - 2026-07-24

### 변경

- 새 NVS, NVS 초기화, OTA 안전 초기화에서 Nag Killer는 OFF, 선택은 MODE 3(기본)으로 시작.
- `[기본]/A~D` 실험 프로파일을 폐기하고 검증된 MODE 1/2/3 UI/API로 교체.
- 메인 화면을 A채널 Summon/TSLLC와 B채널 Nag Killer 상태 카드 중심으로 재배치.
- CAN 자가 진단을 A/B 동시 검사와 HW3 DAS 921/923 경로 기준으로 개편.
- 진단 화면은 A/B 요약을 먼저 보여주고 채널별 상세 카운터·BUS-OFF 이력은 기본 접힘으로 변경.
- Web 로그는 중요 항목을 기본 표시하며 필요할 때 전체 로그로 전환 가능.
- Serial은 부팅·OTA·초기화 실패·BUS-OFF/복구·TEC/REC 임계값 전환만 출력.

### 수정

- 개별 진단 CSV를 실제 시각·업타임·채널 접두사를 갖춘 단일 헤더 UTF-8 CSV v2로 개편.
- 이벤트에 채널·심각도·최초/최종 시각·반복 횟수·플래그 해석을 추가하고 반복 alert를 30초 단위로 집계.
- 자가 진단 미실행 파일에 `NOT_RUN` 안내 행을 기록하고 수동 기록 정지 시 선택 구간을 고정.
- Core 1 주기 USB 시리얼 출력을 제거하고 B 수신 burst 중 A MCP2515를 재서비스해 RX 오버런 가능성을 축소.
- A 폴링 공백 최근·최대·2ms 초과 계측을 Web UI, API, CSV, 자가 진단에 추가.
- 동작하지 않던 `Enable Log`, `enable_print` 상태/API를 제거하고 실제 동작하는 Web 로그 필터로 교체.
- Web 실시간 로그 시각을 브라우저 실제 시각 기준으로 보정.
- OTA 시작 시 A/B 수정 송신 호출을 원자적으로 차단하고, 진행 중 호출 종료 확인 후 B TX 큐를 비우도록 보강.
- 모든 Nag 모드에 -1.80~+1.80 Nm 최종 토크 상한 적용.
- MODE 3에만 921/923·297 마지막 수신 후 1초 이내 조건, AP 3~6, validity=1, 조향각 ±5° 게이트 적용.
- `twai_transmit()` 송신 요청 성공만 성공 카운터에 반영하고 자체 echo 재처리 차단.
- A채널 일반 `WARN`을 RX 오버런·오류 경고·에러 패시브·BUS-OFF로 구분.
- A/B 시계열·CAN 자가 진단·밀리초 이벤트 CSV와 재수신→첫 Summon TX 지연 계측 추가.
- 실제 MCP2515 초기화 결과를 A채널 드라이버 상태에 반영하고 삭제된 실험 플래그·UI 참조 제거.

## [1.3.4] - 2026-07-22

### 추가

- Parked 또는 Summoning 상태 게이트와 Web UI 진단을 갖춘 HW3 조건부 Summon Unlock.
- OTA 부팅 정책 테스트와 CAN을 시작하지 않는 복구 진단.

### 변경

- CAN-A는 Summon Unlock·TSLLC, CAN-B는 Nag Killer를 담당.
- 일반 Web UI 원본을 `web/web_ui.html`로 유지하고 펌웨어에 동기화.

### 수정

- 차량 영향 NVS 값을 CAN 시작 전에 로드.
- OTA와 rollback 오류 시 CAN을 비활성화한 복구 UI로 fail-closed 처리.


### 추가

- M5Stack AtomS3 Mini CAN Base 지원과 Summon 관련 확장 오토파일럿 기능.

### 수정

- HW3Handler의 오래된 speed-profile 매핑, NagHandler 토크 프레임, Web UI 기능 처리.

## [1.0.0] - 2026-04-05

### 추가

- HW3/HW4 FSD 관련 CAN 기능, Nag 억제, ISA 경고음 억제, 긴급 차량 감지, 속도 프로필, Smart Summon, Web UI, OTA.
- MCP2515, SAME51, TWAI 드라이버와 여러 보드 지원 기반.

### 수정

- Nag 토크 값, CAN counter/checksum, FSDEnabled 변수 가림, TWAI TX timeout.

### 변경

- 빌드 플래그와 펌웨어 설정 구조 정리.
