# Summon Unlock UI 병합 계획

## 목표

- 검증된 `summon_unlock.ino`의 제어 API와 상태 필드를 현재 Web UI 구조에 맞게 병합한다.
- Summon Unlock을 기존 EAP 명칭·변수·API·NVS에서 완전히 분리한다.
- A채널은 Summon Unlock/TSLLC 전용, B채널은 Nag Killer 전용이라는 역할을 UI와 진단에 명확히 표시한다.

## 확정된 설계

- 원본 `INDEX_HTML`이 없으므로 INO의 `/api/enable`, `/api/disable`, `/api/stats` 의미와 상태 필드를 기준으로 현재 UI를 설계한다.
- 런타임 변수는 `summonUnlockRuntime`, API는 `/api/summon-unlock`, NVS 키는 `summon_unlock`을 사용한다.
- 기존 `enh_autopilot` NVS 값은 이전하지 않고 폐기 대상으로 처리한다. 새 Summon Unlock 값이 없으면 INO와 동일하게 ON으로 시작한다.
- Summon 게이트는 `Parked || Summoning`이며 AP 상태는 모니터링 전용으로 유지한다.
- A채널 TX 마스터는 진단 화면의 안전장치로 계속 노출한다.

## 단계별 실행

1. EAP 식별자와 API/NVS 연결을 Summon Unlock 전용 이름으로 교체한다.
   - 대상 파일: `include/can_helpers.h`, `include/handlers.h`, `include/web/web_server.h`, `platformio.ini`, 관련 테스트.
   - 성공 조건: 기존 EAP 런타임/API/NVS 이름이 남지 않고 Summon 게이트 테스트가 통과한다.
2. 메인·제어·진단 UI를 INO 상태 필드 기준으로 정리한다.
   - 대상 파일: `include/web/web_ui.h`, `scripts/mock_webui_server.mjs`, Web UI 회귀 테스트.
   - 성공 조건: 활성 상태, Gate, AP, Parked, Summoning, ACA, SPR, 5개 수신 카운터, Mux1/TX/차단 상태가 올바른 영역에 표시된다.
3. 좁은 테스트부터 전체 관련 검증과 보드 빌드를 실행한다.
   - 검증: Summon/HW3/Web UI/native/Nag 테스트, printf 검사, `git diff --check`, `pio run -e lilygo_t2can`.
   - 성공 조건: 모든 실행 항목이 통과하고 실차에서만 확인 가능한 항목을 별도로 남긴다.

## 비범위

- BLE 기능 추가.
- INO에 없는 새 CAN 조건이나 비트 추가.
- Nag Killer 알고리즘 변경.
- TSLLC 알고리즘 변경.
- 사용자가 요청하지 않은 인접 코드 리팩터링.
