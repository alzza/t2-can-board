# 2026-08-01 실차 로그 OTA·ECE R79 검증 체크리스트

- [x] `can_timeseries_ab 6.csv` 스키마 3과 시간 범위 확인
- [x] `can_events 6.csv` 이벤트 종류와 경고·오류 집계
- [x] `canmod_20260801_193620.txt` 최종 자가 진단 확인
- [x] AP 활성 구간의 `SUMMON_GATE=1`, `SUMMON_TX=1`, `TSLLC_TX=1` 이벤트 확인
- [x] B채널 BUS-OFF, TEC/REC, TX Fail 0 확인
- [x] A채널 BUS-OFF, TEC/REC 0 확인
- [x] A채널 RX 오버런과 TX Guard 반복을 불안정 요소로 기록
- [x] OTA 무통신에러 성공 규칙을 README, OTA 절차, 작업 지침에 반영
- [ ] 다음 실차 로그에서 AP 구간 시계열 CSV가 잘리지 않도록 캡처 시작 시점 확인
- [x] A채널 RX 오버런 빈도 감소 방안 검토 및 2026-08-02 소프트웨어 안정화 1단계에 반영
