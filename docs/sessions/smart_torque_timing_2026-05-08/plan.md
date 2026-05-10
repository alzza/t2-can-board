# Smart Torque 타이밍 로깅 보강 계획

목표.
- 5초 시계열 로그와 밀리초 이벤트 로그만으로 Mode B 타이밍 후보를 판단할 수 있게 만든다.
- 이번 변경에서는 주입 타이밍 값을 바꾸지 않고, 실차 로그 판단에 필요한 관측값만 추가한다.

성공 기준.
- `/api/logs-bundle` 통합 로그 [4] 시계열 섹션에 Mode B 판단 필드가 추가된다.
- `/api/logs-bundle` 통합 로그 [5] 이벤트 섹션에 Mode B 상태 전이/phase 전이/첫 echo 지연 이벤트가 추가된다.
- 개별 `/api/timeseries.csv`, `/api/events.csv`는 디버그용 보조 엔드포인트일 뿐 실차 회수 경로로 삼지 않는다.
- `pio run -e lilygo_t2can` 빌드가 성공한다.
- 가능하면 `pio test -e native_nag`를 실행하고, 실패 시 실제 에러를 기록한다.

변경 범위.
- `include/can_helpers.h`: Mode B 타이밍 상수와 진단 필드 추가.
- `include/event_log.h`: Mode B 이벤트 타입과 통합 로그 이벤트 설명 추가.
- `include/handlers.h`: Mode B 상태/phase 전이와 첫 echo 지연 기록 추가.
- `include/timeseries.h`: 시계열 샘플 필드와 출력 컬럼 추가.
- `include/web/web_server.h`: 통합 로그의 시계열/이벤트 헤더와 출력 컬럼 동기화.
- 문서/세션 로그: 이번 결정과 검증 결과 기록.

검토 포인트.
- 5초 시계열은 300~500ms 결정을 직접 측정하기엔 거칠다.
- 따라서 시계열은 구간 맥락을, 이벤트 로그는 ms 단위 전이를 맡는다.
- 이벤트는 매 echo마다 찍지 않고 첫 echo와 전이만 찍어 이벤트 링버퍼를 보호한다.