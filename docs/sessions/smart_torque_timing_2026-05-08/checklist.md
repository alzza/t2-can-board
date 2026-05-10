# Smart Torque 타이밍 로깅 보강 체크리스트

- [x] 현재 시계열 로그 필드 확인.
- [x] Mode B 상태머신과 진단 필드 확인.
- [x] ESP-IDF v4.4.8 TWAI 문서 확인.
- [x] Mode B 타이밍 상수와 진단 필드 추가.
- [x] Mode B 상태/phase/첫 echo 이벤트 추가.
- [x] 통합 로그 [4] 시계열 컬럼 추가.
- [x] 통합 로그 [5] 이벤트 컬럼 동기화.
- [x] `/api/nag-stats`와 사용자 마커 로그에 Mode B 타이밍 맥락 추가.
- [x] `docs/log_reference.md`에 새 컬럼과 이벤트 해석 추가.
- [x] 변경 파일 에디터 진단 확인.
- [x] 빌드/테스트 검증.
- [x] 세션 로그에 변경과 검증 기록.
- [x] 통합 로그 기준으로 CSV 저장 표현 정정 (주석, 문서, 세션 로그).

검증 결과.
- `git diff --check -- include/can_helpers.h include/event_log.h include/handlers.h include/timeseries.h include/web/web_server.h docs/log_reference.md docs/sessions/smart_torque_timing_2026-05-08/plan.md docs/sessions/smart_torque_timing_2026-05-08/checklist.md docs/sessions/smart_torque_timing_2026-05-08/context-notes.md`: 출력 없음.
- `pio run -e lilygo_t2can`: SUCCESS, `pio_build_status=0`.
- `pio test -e native_nag`: 32개 테스트 전부 통과, `pio_native_nag_status=0`.
- VS Code Problems: 변경 코드/문서 파일 오류 없음.