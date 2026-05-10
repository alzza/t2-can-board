# 스마트토크 프로파일 컨텍스트 노트

## 2026-05-10 07:43 KST

- 사용자는 스텔스 모드 삭제를 승인했고, 테스트 단계이므로 스마트토크 `기본값 / A안 / B안` 3단계 실험은 유지하기를 원했다.
- 웹 UI는 라디오 버튼 선택 시 문구와 설명, 변수 표시가 바뀌는 간결한 구성을 선호한다고 했다.
- 이전 로그 기준 타이밍 문제는 700ms/400ms 적용으로 해결됐고, 남은 이슈는 AP gate 및 주입량 최적화다.
- BUS-ERR는 API 연결 문제가 아니라 TWAI `bus_error_count` 실측값이다. BUS-OFF와 별개이며, `TWAI RUNNING`, `TEC/REC=0`, `TX-FAIL=0`이면 현재 장애로 보지 않는다.
- 구현 시 AP gate를 확대하지 않는다. 주입량 감소 실험은 state1 grace 축소와 burst/pause 중심으로 제한한다.

## 2026-05-10 08:13 KST

- Stealth/Mode A PRNG 주입 경로는 제거하고 Smart Torque 단일 경로로 정리했다. 호환용 `mode` 값은 Smart 고정으로 남겼다.
- Smart profile은 `기본 / A안 / B안` 3개이며 NVS 키는 `nag_prof`, 웹 API는 `/api/nag-profile?p=0|1|2`를 사용한다.
- 로그 번들, `/api/nag-stats`, `/api/nag-config`, 시계열 CSV에 `smartProfile`과 프로파일 label/summary/timing 값을 남기도록 맞췄다.
- 웹 UI는 스텔스/스마트 토글을 제거하고 profile radio 선택 시 설명과 timing 타일이 갱신되도록 변경했다.
- `test_native_nag`는 Smart Torque AP gate와 state2 delay를 반영하도록 갱신했다. 기본 state2 mild 출력 raw 범위는 2098~2198로 검증한다.
- `pio run -e lilygo_t2can`는 성공했다. `pio test -e native_nag`는 32개 모두 통과했다.
- 전체 `pio test -e native`는 기존 stale 테스트 때문에 실패한다. HEAD에도 `LegacyHandler`/`HW4Handler`가 없고, HW3 native 기대값도 현재 HW3-only 핸들러와 불일치한다.

## 2026-05-10 16:24 KST

- 사용자는 delay+torque 조합 1차 후보를 `[제어]` Nag Killer 프로파일 C안으로 추가하길 요청했다.
- 적용 기준은 state2 delay 600ms, state2 mild 상한 1.7Nm, strong delay 400ms와 strong 최대 2.10Nm 유지다.
- 기존 state2 mild 범위는 `handlers.h`에 +0.5~+1.5Nm, -1.5~-0.5Nm로 하드코딩되어 있어 C안만 다르게 적용하려면 프로파일 설정에 mild raw delta를 추가해야 한다.
- TWAI 복구/driver 설정은 이번 범위가 아니다. ESP-IDF v4.4 TWAI 문서는 확인했지만 드라이버 API 변경은 하지 않는다.

## 2026-05-10 16:38 KST

- C안은 profile id 3으로 추가했다. NVS/API clamp는 0~3을 허용한다.
- `NagSmartProfileSettings`에 `state2MildMinRawDelta`와 `state2MildMaxRawDelta`를 추가했다. 기본/A/B는 50~150 raw delta, C안은 50~170 raw delta다.
- handler state2 mild 출력은 프로파일 delta에서 계산한다. C안 양수 방향은 raw 2098~2218, 음수 방향은 raw 1878~1998이다.
- mock Web UI `http://127.0.0.1:8788/?scenario=normal`에서 C안 선택 시 `600ms`, `±0.5-1.7Nm`, API `smartProfile=3`을 확인했다.
- 검증은 `node --check scripts/mock_webui_server.mjs`, `git diff --check`, `pio test -e native_nag`, `pio run -e lilygo_t2can`가 통과했다.

## 2026-05-10 토크 인젝션 가이드 상세화

- 가이드는 923 단독 표기를 쓰고 있었지만 현재 구현은 ID 921/923을 모두 `DAS_status` 후보로 읽는다.
- `ModelY_PARTY.dbc`는 `BO_ 923 DAS_status`, `ModelY_CH.dbc.txt`는 `BO_ 921 DAS_status`로 확인됐다.
- `DAS_autopilotState`는 data[0] 하위 4비트, `DAS_autopilotHandsOnState`는 data[5] bit2..5로 구현되어 있다.
- `EPAS3P_torsionBarTorque` 주입 생성기는 구현 내부 기준으로 raw 2048을 중립으로 쓰며, 로그의 `modeBLastNm`도 `(raw - 2048) * 0.01` 기준이다.
- DBC 물리식은 `(raw * 0.01 - 20.5)`라서 실제 수신 토크 `realTorqueNm`과 주입 생성기 표시 기준을 문서에서 구분해야 한다.
- 현재 프로파일 기본/A/B/C 값은 `include/can_helpers.h`의 `NagSmartProfileSettings`가 단일 기준이다.
- C안은 구현과 mock 검증은 완료됐지만 아직 실차 검증 전 후보로 문서화한다.

## 2026-05-10 프로파일 실험 가치 검토

- `docs/sessions/tesla_log/canmod_20260510_071256.txt` 원본을 직접 재파싱했다.
- 10분 시계열은 120 samples, Mode B samples는 104개다.
- Mode B 합계는 `d880=52003`, `dEcho=14899`, `dModeBInject=14607`, `dSkipAP=25477`, `dSkipHO=209`, `dSkipDAS=0`, `dDrop=0`이다.
- AP-active Mode B 구간만 보면 53 samples, `d880=26502`, `dEcho=14516`, `dModeBInject=14224`, `dSkipAP=749`, `dSkipHO=137`, `dDrop=0`이다.
- zero-injection AP-block 샘플은 48개이고 `d880=24001`, `dModeBInject=0`, `dSkipAP=23975`라서 무주입의 큰 원인은 AP gate다.
- 시계열의 `modeBFirstEchoDelayMs`는 `1ms x10`, `400ms x4`, `700ms x16`, `701ms x1`, `710ms x1`로 재계산됐다.
- 주입 토크 절대값은 전체 median `0.945Nm`, p75 `1.18Nm`, max `2.10Nm`; state2는 median `0.94Nm`, max `1.50Nm`; strong은 median/max `2.10Nm`다.
- CAN health는 `BUS-OFF=0`, `TEC=0`, `REC=0`, `TX-Fail=0`, `EchoDrop=0`로 깨끗했다.
- 기본 프로파일은 실차 비교의 control로 반드시 유지한다. 700/400ms가 동작함은 확인됐지만 state2 상한 1.5Nm에 닿았고 continuous injection이라 최종 최적값이라고 단정하지 않는다.
- A안은 delay를 유지하고 burst/pause와 state1 grace만 바꾸는 duty-reduction 실험이다. 기본/C안 다음에 테스트할 가치가 높다.
- B안은 900/600ms와 매우 sparse한 burst로 가장 보수적이다. 효과 후보보다는 하한/negative-control 성격이 강하므로 우선순위는 낮다.
- C안은 최신 로그가 직접 가리키는 후보라 다음 1순위 실차 테스트 가치가 높다. 단, AP_BLOCK은 해결하지 못하고 기본과 같은 continuous injection 리스크가 있다.
