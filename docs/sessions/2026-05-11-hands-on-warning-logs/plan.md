# DAS hands-on warning 로그 보강 계획

작성 시각: 2026-05-11 19:24 KST.

## 목표

통합 로그만 보고 `DAS_autopilotHandsOnState` 경고 단계, 조향각, 실제 토크, NAG handler 판정, D안 profile 여부를 함께 해석할 수 있게 만든다.

## 범위

1. `DAS_autopilotHandsOnState` raw enum을 사람이 읽는 이름과 경고 level로 변환하는 helper를 추가한다.
2. `/api/timeseries.csv`와 `/api/logs-bundle`의 20분 시계열 CSV에 `dasStateName`, `dasStateGroup`, `dasWarnLevel`, `dasWarning`을 추가한다.
3. 기존 ms 단위 이벤트 로그의 `MODEB_STATE`, `MODEB_PHASE`, `MODEB_FIRST_ECHO` detail을 사람이 읽는 `detailText`로 함께 출력한다.
4. `/api/nag-stats`, `/api/status` 계열 JSON에 현재 DAS hands-on label과 warning level을 같이 넣는다.
5. NAG 주입 동작, AP gate, D안 sign hold 로직은 변경하지 않는다.

## 성공 기준

- `DAS_autopilotHandsOnState` 0,1,2,3,4,5,6,7,8,9,10,15,0xFF가 명확한 이름과 warning level로 출력된다.
- 통합 로그 [4] 시계열 행에서 같은 행 안에 profile, AP state, DAS warning, steering angle, real torque, injected torque, decision이 함께 보인다.
- 통합 로그 [5] 이벤트 행에서 hands-on state 전환이 `detailText`로 해석된다.
- `pio test -e native_nag`가 통과한다.
- `pio run -e lilygo_t2can`가 통과한다.