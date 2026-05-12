# Plan - Signal Observer Capture Controls

1. 캡처 API를 추가한다.
   - 검증: `POST /api/signal-observer/capture`로 `enabled=true/false`가 반영되고, start는 카운터를 새로 시작한다.
2. 실험 탭 버튼을 교체한다.
   - 검증: 기존 `맨 위`, `맨 아래` 버튼 대신 `시작`, `정지` 버튼이 표시된다.
3. mock 서버를 펌웨어 API와 맞춘다.
   - 검증: mock에서 start/stop 상태와 카운터 freeze가 확인된다.
4. 빌드와 문법 검증을 실행한다.
   - 검증: `node --check`, `pio run -e lilygo_t2can` 통과.
