# Web UI redesign checklist

- [x] 현재 `web_ui.h`의 섹션, 타일, JS 업데이트 경로 확인
- [x] 메인 타일 중복 제거와 신규 3개 타일 배치
- [x] 제어 섹션을 큰 카드형 토글 구조로 정리
- [x] OTA 섹션을 탭/화면으로 노출하고 상태 문구 확인
- [x] mock 서버 API 부족분 확인 및 보강
- [x] 정적 검증 실행
- [x] mock 서버 실행 후 브라우저 smoke 검증
- [x] 세션 로그 기록

검증 결과.
- `get_errors` 기준 `include/web/web_ui.h`, `include/web/web_server.h`, `scripts/mock_webui_server.mjs` 오류 없음.
- `node --check scripts/mock_webui_server.mjs` 통과.
- `git diff --check -- include/web/web_ui.h include/web/web_server.h scripts/mock_webui_server.mjs docs/sessions/2026-05-10-webui-redesign/plan.md docs/sessions/2026-05-10-webui-redesign/checklist.md docs/sessions/2026-05-10-webui-redesign/context-notes.md docs/sessions/chat_log_2026-05-10.md` 통과.
- `pio run -e lilygo_t2can` 성공. `PIO_STATUS=0`, `1 succeeded in 00:00:38.092`.
- mock 브라우저에서 `normal`, `bus_off`, `no_frames` 메인 A/B 상태 배지 색상과 타일 Hz 표시를 확인함.
