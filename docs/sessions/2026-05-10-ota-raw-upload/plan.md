# 2026-05-10 OTA raw upload 수정 계획

목표:
- Web UI OTA 업로드가 서버의 raw firmware 처리 방식과 맞도록 수정한다.
- 정상 UI와 복구모드 UI 모두 같은 방식으로 고친다.
- 차량에서 OTA 업로드, 자동 롤백, 복구모드 진입, 복구 업로드를 확인하는 절차 문서를 만든다.

범위:
- `include/web/web_ui.h`의 OTA 업로드 JS만 최소 수정한다.
- 서버 OTA 상태머신은 이번 변경에서 구조 변경하지 않는다.
- 문서는 실차 검증 절차와 최초 1회 raw curl 업로드 우회 절차를 포함한다.

성공 기준:
- 정상 UI와 복구모드 UI 모두 `application/octet-stream` raw `.bin` 전송으로 변경.
- 업로드 실패/타임아웃 시 버튼이 복구되고 상태 문구가 갱신됨.
- 검증 절차 문서가 차량 현장에서 따라 하기 쉬운 순서로 작성됨.
- `git diff --check`, `pio run -e lilygo_t2can` 통과.