# D안 sign hold 구현 계획

목표.
- C안을 실차 비교 기준으로 보존한다.
- D안을 새 Smart Torque 프로파일로 추가한다.
- D안은 C안의 state1/state2/strong timing과 토크 범위를 그대로 쓰고, state2 mild와 strong의 토크 부호 결정에만 sign hysteresis/hold를 적용한다.

근거.
- 최신 20분 로그에서 C안 strong echo ratio는 높았으므로 실패 원인을 strong 미주입으로 단정하지 않는다.
- 현재 코드는 `_mbAngleDeg > 0.0f` 즉시 기준으로 방향을 바꾼다.
- 직선 저조향각에서 조향각 부호가 흔들리면 합성 토크가 사람이 1~2초 한 방향으로 잡는 패턴처럼 보이지 않을 수 있다.

구현 범위.
- `include/can_helpers.h`에 D안 프로파일 상수와 설정을 추가한다.
- `include/handlers.h`에 D안 전용 방향 hold 상태와 helper를 추가한다.
- `include/web/web_ui.h`의 프로파일 선택 UI와 fallback 설명에 D안을 추가한다.
- 이벤트/진단 설명의 프로파일 번호 설명을 D안까지 갱신한다.
- native NAG 테스트에 D안 설정, D안 sign hold, C안 즉시 전환 보존 테스트를 추가한다.

성공 기준.
- D안에서 `steerDeg`가 0도 근처 또는 반대 부호로 순간 전환되어도 hold 시간 안에는 주입 토크 부호가 유지된다.
- D안에서 hold 시간이 지난 뒤 충분히 반대 방향 조향각이 확인되면 토크 부호가 전환된다.
- C안과 기존 기본/A/B 동작은 기존 즉시 방향 결정 로직을 유지한다.
- `pio test -e native_nag`가 통과한다.
- `pio run -e lilygo_t2can`가 통과한다.