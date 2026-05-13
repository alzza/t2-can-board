# Plan

목표
- 전체 저장 다운로드 중 보드 재부팅 원인을 코드/로그 근거로 좁히고, 추측성 대수술 없이 최소 변경으로 완화한다.

설계
- 클라이언트: 다운로드 중 background polling 재개 타이머를 endpoint별로 충분히 늘려 대용량 logs-bundle 전송 중 병행 요청을 줄인다.
- 서버: logs-bundle 전송 중 `/api/status`, `/api/nag-stats`를 가볍게 503 처리해 겹침 요청을 방어한다.

검증 기준
1. `pio test -e native -f test_native_web_ui_recovery` 통과
2. `pio run -e lilygo_t2can` 빌드 성공
3. `git diff --check -- include/web/web_ui.h include/web/web_server.h test/test_native_web_ui_recovery/test_web_ui_recovery.cpp docs/sessions/chat_log_2026-05-13.md` 통과
