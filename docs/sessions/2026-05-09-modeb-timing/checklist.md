# 2026-05-09 Mode B timing checklist

- [x] Mode B state 2 delay를 2000ms에서 700ms로 변경한다.
- [x] Mode B strong delay를 1000ms에서 400ms로 변경한다.
- [x] AP gate `3..6` 유지 사실을 주석과 문서에 남긴다.
- [x] `STEERING_TORQUE_INJECTION_GUIDE.md`의 오래된 2초/1초 설명을 갱신한다.
- [x] `docs/sessions/chat_log_2026-05-09.md`에 실제 변경과 검증 결과를 기록한다.
- [ ] `git diff --check`를 실행한다.
- [ ] `pio run -e lilygo_t2can`를 실행한다.
- [ ] 필요 시 `pio test -e native`를 실행한다.
