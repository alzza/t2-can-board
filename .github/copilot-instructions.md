# Copilot Instructions

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them — don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it — don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.

---

## Project-Specific Guidelines

### Stack
- **MCU**: ESP32-S3 (LILYGO T2-CAN), Arduino-ESP32 2.0.17 = ESP-IDF v4.4.x
- **RTOS**: FreeRTOS, **Core 1 = CAN A/B 통합 폴링 / Core 0 = WiFi·HTTP·보조 태스크** 분리
- **A 채널**: MCP2515(SPI, GPIO10/11/12/13, CS=GPIO13, RST=GPIO9, INT=GPIO8 미사용) → `appLoop<MCP2515Driver>()`이 **`nagKillerTask` 본문 시작에서** 매 iter 호출됨 + `HW3Handler` (하드코딩: `using SelectedHandler = HW3Handler`). SPI 8MHz 기본 (10MHz 가능).
- **B 채널**: 내장 TWAI (TX=GPIO7, RX=GPIO6, 500kbps) → `nagKillerTask` (**Core 1**, prio 10) + `NagHandler` (ID 880/921). TWAI ISR `intr_flags = ESP_INTR_FLAG_IRAM` 요청 (sdkconfig `CONFIG_TWAI_ISR_IN_IRAM=y` 시 OTA cache-disable 구간에서도 RX FIFO 보호).
- **`loop()` (Arduino loopTask, Core 1, prio 1)**: A/B 폴링 없음. `vTaskDelay(100ms)`로 IDLE 양보만 수행.
- **보조 태스크** (모두 Core 0):
  - `canAlertTask` (prio 1, 20ms): `TWAIDriver::pollAlerts()` → eventLog 기록
  - `statusLogTask` (prio 1): 5초 상태 요약 로그 (`T2CAN_STATUS_LOG_TASK=0`으로 disable)
  - `timeseriesTask`: 5초 간격 시계열 수집 (최근 30분, CSV 다운로드)
- **웹서버**: `esp_http_server` (Core 0, stack=16384) + cJSON, 45개 API 엔드포인트
- **WiFi**: AP-only (`TeslaCAN`, `kApChannel`) — STA 비활성으로 TWAI ACK 안정화
- **빌드**: PlatformIO, env `lilygo_t2can` (보드) + `native_nag`/`native`/`native_log_buffer` (네이티브 테스트)

### Key Files
- `src/main.cpp` — `setup()`/`loop()`, `otaBootCheck()`, `nagKillerTask`, `canAlertTask`, 웹서버 시작
- `include/app.h` — `appSetup<Driver>()` / `appLoop<Driver>()` 템플릿, `SelectedHandler = HW3Handler`
- `include/handlers.h` — `HW3Handler` (A ch, ID 1021, EAP/TSLLC 주입), `NagHandler` (B ch, ID 880 echo)
- `include/can_helpers.h` — 런타임 토글 (`Shared<bool>`), `BChannelDiagnostics`, `AChannelDiagnostics`, `BusOffEventLog`, `NagConfig`, checksum/bit/mux 헬퍼
- `include/t2can_pins.h` — T2-CAN 핀맵 (TWAI TX/RX, MCP2515 SPI/CS/RST)
- `include/web/web_server.h` — 45개 HTTP 핸들러, NVS 영속화, OTA 상태 머신, `gOtaRecoveryModeActive`
- `include/web/web_ui.h` — `WEB_UI_HTML` (정상 SPA) + `WEB_RECOVERY_UI_HTML` (OTA 복구모드 전용 UI)
- `include/drivers/twai_driver.h` — TWAI 드라이버 (BUS-OFF 복구: hard re-install, 쿨다운 런타임 설정)
- `include/drivers/mcp2515_driver.h` — MCP2515 SPI 드라이버 (one-shot, EFLG/TEC/REC 폴링)
- `include/event_log.h` — 이벤트 로그 링 버퍼 (TWAI alert, BUS-OFF 등)
- `include/timeseries.h` — 5초 간격 시계열 (B채널 Hz, 에러 카운터, 결정 분포)

### OTA 상태 머신 (NVS `ota_pending` 키)
```
pending=0  정상 동작
pending=1  새 FW 기록 완료 → 재부팅 대기
pending=2  새 FW 부팅 중 → 3분 내 /api/ota-confirm 필요 (웹 배너 표시)
pending=3  롤백 설정 완료 → 재부팅 대기
pending=4  이전 FW 복구 부팅 중 → 1분 내 /api/ota-recovery-confirm 필요
pending=5  복구모드 (gOtaRecoveryModeActive=true, CAN 비활성, 웹 서버만 동작)
```
- `otaBootCheck()`: `setup()` 최초 실행 시 pending 상태 읽어 전이 처리
- `otaWatchdogTask`: 1초 폴링, 타임아웃 시 자동 롤백 또는 복구모드 전환
- 복구모드: CAN 초기화 건너뜀, `webServerInit(nullptr)`, `WEB_RECOVERY_UI_HTML` 서빙

### Web API 엔드포인트 (45개)
```
GET  /                          대시보드 HTML (정상 또는 복구모드 UI)
GET  /api/status                통합 상태 JSON (3s polling, OTA 필드 포함)
GET  /api/nag-config            NagConfig JSON
GET  /api/nag-stats             B채널 실시간 통계
POST /api/nag-mode              Nag 모드 변경
POST /api/nag-update            NagConfig 업데이트
POST /api/nag-reset             NagConfig 초기화
POST /api/enhanced-autopilot    EAP 토글
POST /api/tsllc                 TSLLC 토글 (스톱사인/초록불)
POST /api/nag-killer            Nag Killer 토글
POST /api/isa-speed-chime       ISA 속도 차임 토글
POST /api/emergency-vehicle-detection  긴급차량 감지 토글
POST /api/a-ch-tx               A채널 1021 송신 마스터 토글
POST /api/a-spi-8mhz            A MCP2515 SPI 8MHz 토글
POST /api/a-oneshot             A MCP2515 one-shot 모드 토글
POST /api/a-tx-guard            A채널 TX guard 토글
POST /api/twai-ss-tx            TWAI single-shot TX 토글
POST /api/twai-busoff-stop      TWAI BUS-OFF 시 송신 중단 토글
POST /api/emergency-disable     A채널 비상 비활성화
POST /api/emergency-restore     A채널 비상 복구
GET  /api/busoff-log            BUS-OFF 이벤트 로그 JSON
GET  /api/busoff-log-dl         BUS-OFF 이벤트 로그 CSV
DELETE /api/busoff-log          BUS-OFF 이벤트 로그 초기화
POST /api/busoff-cooldown       BUS-OFF 복구 쿨다운 런타임 변경
POST /api/busoff-mode           BUS-OFF 소프트 모드 토글
POST /api/can-diag/start        CAN 자가진단 태스크 시작
GET  /api/can-diag/log          CAN 자가진단 로그 폴링
GET  /api/timeseries-csv        시계열 CSV 다운로드
GET  /api/timeseries-status     시계열 상태
POST /api/timeseries-reset      시계열 초기화
POST /api/timeseries-rec        시계열 레코딩 토글
GET  /api/events-csv            이벤트 로그 CSV
GET  /api/logs-bundle           통합 로그 번들 (런타임+BUS-OFF+채널 스냅샷)
POST /api/time                  wall-clock 동기화
POST /api/user-marker           수동 AP 경고 마커
POST /api/ota                   OTA 펌웨어 업로드
POST /api/reboot                재부팅
POST /api/ota-confirm           새 FW 확정 (pending 2→0)
POST /api/ota-rollback          새 FW 롤백 (pending 2→3)
POST /api/ota-recovery-confirm  복구 확정 (pending 4→0)
POST /api/ota-enter-recovery    복구모드 강제 진입 (pending 4→5)
POST /api/enable-print          런타임 시리얼 로그 토글
POST /api/set-theme             UI 테마 저장
GET  /generate_204              캡티브 포털 리다이렉트
GET  /hotspot-detect.html       캡티브 포털 리다이렉트
```

### NVS 키 (namespace `"canmod"`, 키 길이 ≤15)
```
isa_speed_chime   emerg_veh_det   enh_autopilot   nag_killer   tsllc
a_ch_tx           a_spi_mhz       a_oneshot        a_tx_guard
nag_mode          nag_id          nag_tc           nag_tb2      nag_tb3   nag_ho
theme
ota_pending       ota_fallback
busoff_cooldown   busoff_soft_mode
```

### Conventions
- **채널 경계**: A/B 카운터/버퍼/처리 루프를 절대 혼용 금지
- **B 채널 소프트 필터**: `nagKillerTask` 인라인에서 ID 880/921 외 조기 반환 (`rxLimit=30`)
- **핫패스 Serial 금지**: `appLoop`, `nagKillerTask`, handlers 내 직접 `Serial.print` 금지 → `logRing.push()` + `runtimeSerialPrintln()` 사용. 기본값 `T2CAN_RUNTIME_SERIAL_LOGS=0`
- **statusLogTask 분리**: 5초 상태 요약은 `statusLogTask`(Core0, prio 1)에서 처리 → `nagKillerTask`(prio 10) 핫패스 보호. `T2CAN_STATUS_LOG_TASK=0` 빌드 플래그로 fallback
- **NVS 키 규칙**: 길이 15자 이하, `static_assert`로 빌드 타임 검증, 기존 키 변경 금지
- **체크섬**: `computeTeslaChecksum()` 경로 우선 사용 (sum + 0x73 & 0xFF)
- **OTA 복구모드**: `gOtaRecoveryModeActive=true`이면 CAN 초기화 전체 건너뜀, `loop()`에서 즉시 반환
- **HW4Handler/LegacyHandler**: 현재 주석 처리됨. `SelectedHandler = HW3Handler` 고정. 변경 전 확인 필수

### Build & Test
```bash
# 빌드
pio run -e lilygo_t2can

# 업로드
pio run -e lilygo_t2can -t upload

# B채널 핵심 테스트 (31개)
pio test -e native_nag

# 전체 네이티브 테스트
pio test -e native_nag -e native -e native_log_buffer
```


### ESP32 CAN(TWAI) 레퍼런스 (필수 참조)

> **⚠️ MANDATORY**: ESP32 CAN/TWAI 관련 코드를 작성·수정·검토할 때는
> 반드시 아래 공식 문서를 먼저 확인해야 한다.
> 현재 환경: Arduino ESP32 2.0.17 = **ESP-IDF v4.4.x** (v3.3/v5.x 문서와 다름)

**필수 레퍼런스**:
- **[ESP-IDF v4.4.8 ESP32-S3 Programming Guide (Index)](https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32s3/index.html)**
  → TWAI 작업 전 버전 기준, 드라이버 동작 전제, API 탐색 시작점으로 먼저 확인
- **[ESP-IDF v4.4 TWAI API (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32s3/api-reference/peripherals/twai.html)**
   
**확인 필수 항목**:
1. State Machine 준수: `Stopped → Running → Bus-Off → Recovering → Stopped`
2. `twai_message_t.ss` 플래그 — Single Shot TX (충돌 시 재전송 금지, TEC 억제)
3. `twai_message_t.extd` 플래그 — 11bit vs 29bit ID 구분
4. BUS-OFF 복구 절차: `twai_initiate_recovery()` 후 Recovering → Stopped → `twai_start()`
5. Alert 시스템: `TWAI_ALERT_BUS_OFF`, `TWAI_ALERT_ERR_PASS`, `TWAI_ALERT_TX_FAILED` 등

**금지 사항**:
- v3.3 `can_` 구 API 참조 금지 (deprecated, 현재 환경에서 존재하지 않음)
- 임의 ESP32 예제 복붙 금지 (버전 불일치로 `msg.ss` 누락, state 오류 발생 이력 있음)
 
### TWAI 안전 체크리스트 (수정 전 항상 확인)

> 이 항목들은 2026-04-18 사고(우선순위 실수 변경)에서 도출된 교훈이다.

```bash
# NagTask 우선순위 → 항상 10
grep "NagTask.*nullptr" src/main.cpp

# TX 백오프 로직 → 없어야 함
grep "setTxBackoff" src/main.cpp

# ESP-NOW 채널 강제 변경 → 없어야 함
grep "esp_wifi_set_channel" src/main.cpp

# 빌드
pio run -e lilygo_t2can
```
### Available Skills & Context Utilization
질문에 아래와 관련된 도메인이 포함되어 있다면, 반드시 해당 경로의 `SKILL.md` 파일을 먼저 읽고 그 지침에 따라 답변하세요.

- **Coding Principles:** `.github/skills/karpathy-guidelines/SKILL.md` (오버엔지니어링 금지, 최소한의 외과적 변경 원칙 준수)
- 
### Usage Rule
1. 사용자가 CAN 통신이나 특정 안정화 로직을 물으면, 위 Skills 디렉토리의 컨텍스트를 최우선으로 반영한다.
2. 코드를 수정할 때는 'karpathy-guidelines'를 참조하여 기존 코드를 최대한 보존하며 필요한 부분만 수정한다.
3. 사용자가 명시적으로 `A채널 동작완벽함!` 또는 `B채널 동작완벽함`이라고 선언하기 전까지, 진단/가이드/테스트 우선순위는 A채널(주행/정차 통신 테스트)을 1순위로 유지한다.