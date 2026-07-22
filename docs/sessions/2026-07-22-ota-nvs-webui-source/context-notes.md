# 컨텍스트 노트 - OTA/NVS 안전성 재검토와 Web UI 원본 분리

## 확인된 사실

- 현재 OTA handler는 `Update.begin()` 전에 `prepareOtaUploadCanQuiet()`를 호출한다.
- 이 helper는 차량 기능과 `a_ch_tx`를 런타임/NVS 모두 OFF로 저장하며 B채널 TWAI TX 큐를 비운다.
- 안전값 NVS 저장이 실패하면 OTA 업로드를 HTTP 500으로 중단한다.
- 2026-05-14 코드와 현재 코드의 OTA 상태 머신은 동일하다. 현재 변경은 안전 저장 대상의 `enh_autopilot`을 `summon_unlock`로 교체한 것이다.
- 차량에 동작 중인 구 펌웨어는 신규 `summon_unlock` 키를 알지 못하지만 `a_ch_tx=false`, `nag_killer=false`, `tsllc=false`는 저장할 수 있다.
- 신규 펌웨어의 Summon 기본 설정은 ON이지만 실제 주입은 `a_ch_tx`가 ON이어야 하므로, 정상 OTA 업로드를 거치면 첫 부팅 실제 A채널 주입은 차단된다.
- 2026-05-13 문서는 OTA 첫 부팅에서 `writeOtaSafeFeatureSettings()`를 다시 호출한다고 기록했지만 현재 코드와 확인 가능한 커밋에는 해당 호출이 없다. 문서와 코드가 불일치한다.

## 남은 판단

- `NagHandler::nagKillerActive`와 `nagKillerRuntime`의 빌드 기본값은 모두 ON이다.
- setup은 A채널 NVS만 먼저 읽고 CAN 태스크를 생성한 뒤, `webServerInit()`에서 `nag_killer` NVS를 읽는다.
- 따라서 정상 OTA가 구 펌웨어에서 `nag_killer=false`를 저장해도 새 부팅의 CAN 태스크가 NVS 로드보다 먼저 실행되는 짧은 구간에는 B채널 Nag TX가 가능하다.
- 이 레이스는 OTA 첫 부팅뿐 아니라 Nag를 OFF로 저장한 일반 재부팅에도 존재한다.
- 현재 상태는 “업로드 중 TX 차단”은 해결됐지만 “재부팅 직후 TX 차단”은 완전히 해결되지 않은 것으로 판정한다.

## 권장 보완안

1. 모든 차량 기능 NVS를 CAN 드라이버와 태스크 생성 전에 로드한다.
2. `ota_pending==1`이면 현재 펌웨어가 아는 안전 키(`summon_unlock` 포함)를 CAN 초기화 전에 다시 OFF/stock으로 저장한다.
3. 안전 저장 실패 시 CAN 드라이버/태스크를 시작하지 않고 복구 Web UI만 띄우는 fail-closed 동작을 검토한다.
4. OTA metadata NVS 저장/commit 실패를 확인하지 않고 재부팅하는 현재 경로도 별도 보완 후보로 둔다. 이는 TX 오류보다는 자동 롤백 신뢰성 문제다.

사용자가 의심 지점은 먼저 질문하라고 했으므로 위 OTA/CAN 동작 변경은 승인 전 보류한다.

## 승인된 부팅 상태표

| 입력 상태 | 처리 | CAN 시작 |
|---|---|---|
| `pending=0` | 저장 설정 선로드 | 선로드 성공 때만 허용 |
| `pending=1` | 현재 안전값 OFF/stock + `pending=2`를 한 commit으로 저장 | 저장·선로드 성공 때만 허용 |
| `pending=2` | fallback 파티션 설정 후 `pending=3`, 즉시 재부팅 | 금지 |
| `pending=3` | rollback 펌웨어 확인 창을 위해 `pending=4` | 선로드 성공 때만 허용 |
| `pending=4` | 확인 전 재부팅으로 판단해 `pending=5` 복구모드 | 금지 |
| `pending=5` | 복구모드 유지 | 금지 |
| `pending>5` | 손상/미지원 상태로 판단 | 금지 |
| `pending=1/2`, 실행 파티션 불일치 | USB 플래시로 판단해 현재 안전값 저장 + OTA metadata clear | 저장·clear·선로드 성공 때만 허용 |
| NVS init/open/read/write/commit 실패 | 안전 런타임 적용, 차단 사유 기록, 복구 Web UI | 금지 |
| fallback 누락/파티션 없음/boot 설정 실패 | 차단 사유 기록, 복구 Web UI | 금지 |

누락 키는 fresh NVS 또는 구 펌웨어 호환 상황이므로 정의된 기본값을 사용한다. 단, OTA 첫 부팅은 누락 여부와 무관하게 먼저 현재 안전값을 기록하므로 신규 `summon_unlock` 기본 ON이 TX를 재개하지 않는다.

## 구현 결과

- `include/ota_boot_policy.h`에 `pending=0~5`, USB 파티션 불일치, 범위 밖 값을 순수 정책으로 분리했다.
- `setup()`은 NVS init/sentinel, OTA 상태 전이, 퇴역 키 purge, 차량 영향 설정 선로드가 모두 성공한 뒤에만 MCP2515/TWAI와 통합 CAN 태스크를 생성한다.
- 선로드 대상은 Summon, Nag, TSLLC, ISA/긴급차량 기능, A TX, A SPI, one-shot, TX guard, Nag profile, BUS-OFF cooldown이다.
- 읽기는 지역 변수에 전부 성공한 뒤 런타임에 한 번에 반영하므로 부분 로드 상태로 CAN이 시작되지 않는다.
- `pending=1`과 USB 파티션 불일치는 현재 펌웨어 기준 안전값을 다시 기록한다. 구 펌웨어에 없던 `summon_unlock`도 OFF로 생성된다.
- OTA 업로드, 자동/수동 rollback, 복구모드 진입은 런타임/NVS OFF, B TX queue clear, 상태/metadata commit 성공을 모두 확인한다.
- rollback 전에 사용자가 기능을 다시 켠 경우도 재차 안전값을 기록하므로 ON 설정이 fallback 펌웨어로 넘어가지 않는다.
- OTA metadata 저장 실패는 현재 실행 파티션을 boot 대상으로 복원하고 재부팅하지 않는다.
- NVS 자동 erase 복구와 Web NVS reset은 다음 재부팅에서도 안전하도록 기능 OFF/stock과 `nvs_init_ok`를 저장한다.
- fail-closed 시 A/B CAN 드라이버를 만들지 않고 복구 Web UI만 시작한다. API/UI의 `can_boot_block_reason`에서 원인을 확인할 수 있다.

## 검증 결과

- OTA 상태 정책 7/7 통과. 범위 밖 `pending=6~255` 전체를 검사했다.
- Web UI/복구 UI 집중 테스트와 정책 테스트 15/15 통과.
- 전체 native 91/91 통과.
- B채널 Nag 전용 43/43 통과.
- HW3 차량용 빌드 성공. RAM 106776바이트(32.6%), Flash 981881바이트(49.9%).
- 최종 바이너리 크기 982240바이트, SHA-256 `a7f32ec0f919092eab2e527201fd63b25bc0a7095b0821376e1bea4abe16cac0`.
- Web UI 동기화, printf 포맷 검사, `git diff --check` 통과.
- `checking-printf-formats/SKILL.md`는 저장소에 없어 제공 검사 스크립트로 대체했다.

## 실차 OTA 순서

1. 주행하지 않는 상태에서 TeslaCAN Web UI에 접속한다.
2. OTA 업로드를 시작하면 구 펌웨어가 먼저 A TX, Summon/EAP, TSLLC, Nag를 OFF로 저장하고 B TX queue를 비운다.
3. 새 펌웨어 첫 부팅은 다시 `summon_unlock`, TSLLC, Nag, A TX를 OFF로 기록한 뒤 CAN 모니터링만 시작한다.
4. Web UI에서 A/B 수신과 BUS-OFF 0을 확인하고 60초 안에 새 펌웨어 확인을 누른다.
5. 확인 완료 후 필요한 기능을 Nag, A TX, TSLLC/Summon 순으로 하나씩 켜며 TX Fail/TEC/BUS-OFF를 확인한다.

## 남은 실차 리스크

- fallback 파티션의 5월 14일 구 펌웨어 자체 코드는 변경할 수 없다. rollback 전에 NVS 안전값을 저장하지만 구 펌웨어 내부의 짧은 B 설정 로드 순서는 그대로다.
- NVS 물리 쓰기가 지속적으로 실패하면 안전값을 영구 저장할 수 없으므로 복구 UI 상태에서 재부팅을 반복하지 말고 정상 바이너리를 다시 OTA하거나 보드를 점검해야 한다.
- native/빌드는 상태 전이와 컴파일을 검증하지만 실제 차량 버스의 전원·리셋 타이밍은 첫 OTA에서 확인해야 한다.

## Web UI 원본 분리

- 편집 원본: `web/web_ui.html`.
- 펌웨어 임베디드 결과: `include/web/web_ui.h`의 `WEB_UI_HTML`.
- 동기화: `python3 scripts/sync_web_ui.py --sync`.
- 확인: `python3 scripts/sync_web_ui.py --check`.
- mock 서버는 HTML 원본을 직접 읽는다. 복구모드 UI는 기존 헤더에서 유지한다.

## Web UI 원본 분리 당시 검증

- Web UI 동기화 검사 통과.
- Web UI native 회귀 테스트 8/8 통과.
- HW3 차량용 펌웨어 빌드 성공: RAM 106680바이트(32.6%), Flash 978753바이트(49.8%).
- 생성 바이너리 SHA-256: `ff55469bab41585e290af722e80c929b6eb105415ee9585c38548dae67e8f149`.
- printf 포맷 검사와 `git diff --check` 통과.
- mock 서버 JS 문법 검사는 로컬 Node의 `libllhttp.9.3.dylib` 손상으로 실행하지 못했다. 이번 변경은 파일 읽기 경로와 raw-literal 추출 제거에 한정된다.
