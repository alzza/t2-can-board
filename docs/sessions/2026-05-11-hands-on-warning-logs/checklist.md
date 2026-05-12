# DAS hands-on warning 로그 보강 체크리스트

- [x] 기존 timeseries와 logs-bundle 구조 확인.
- [x] T-CAN 기준 `DAS_autopilotHandsOnState` bit layout과 enum 확인.
- [x] enum label, group, warning level helper 추가.
- [x] timeseries CSV와 통합 로그 [4] 컬럼 확장.
- [x] 이벤트 CSV와 통합 로그 [5] detailText 확장.
- [x] JSON status/nag-stats 현재 상태 필드 확장.
- [x] native helper 테스트 추가.
- [x] `pio test -e native_nag` 실행.
- [x] `pio run -e lilygo_t2can` 실행.
- [x] 세션 로그에 구현과 검증 결과 기록.