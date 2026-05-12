# 2026-05-12 CRC/checksum 정밀 검토 체크리스트

- [x] 관찰기와 로그 저장 경로가 CAN 송신을 하지 않는지 확인한다.
- [x] 0x293과 0x3F8의 checksum/counter 신호를 T-CAN/DBC로 확인한다.
- [x] 0x293 AutoLC 주입에 counter/checksum 보정을 추가한다.
- [x] helper와 HW3 handler 테스트를 추가한다.
- [x] native 테스트를 실행한다.
- [x] `pio run -e lilygo_t2can` 빌드를 실행한다.
- [x] 날짜별 chat log에 결과를 기록한다.
