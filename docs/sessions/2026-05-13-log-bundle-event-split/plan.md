# Plan

목표:
- `/api/logs-bundle`에서 관찰기 이벤트 CSV 본문과 밀리초 이벤트 CSV 본문을 분리해 전체 저장 응답을 가볍게 한다.
- 관찰기 이벤트와 밀리초 이벤트는 새 `/api/events-bundle` 묶음 파일로 함께 저장한다.
- 기존 `/api/signal-observer-log-dl`, `/api/events.csv`는 호환용 개별 endpoint로 유지한다.
- Web UI 신호 관찰기 카드에서 이벤트 묶음 파일을 직접 저장할 수 있게 한다.

성공 기준:
- 통합 로그에는 관찰기 이벤트와 밀리초 이벤트의 count/capacity/이벤트 묶음 경로만 남는다.
- 관찰기 카드에 이벤트 묶음 저장 버튼이 있다.
- Native Web UI 회귀 테스트가 해당 분리를 검증한다.
- `pio test -e native -f test_native_web_ui_recovery`와 `pio run -e lilygo_t2can`가 통과한다.
