# 점검 목록

- [x] 복원 기준 커밋 `28d158b54abc5d1ee97d3b3dce5ff17110d2fbd0` 확인
- [x] 대상 하드웨어가 HW3임을 확인
- [x] 검증된 INO 상태 머신 전체를 읽고 대응 관계 작성
- [x] CAN 280/390/921/1016/1021 관찰과 5초 CAN 280 감시 타이머 추가
- [x] CAN 1021 mux 1 송신을 `Parked || Summoning` 조건으로 제한
- [x] bit 19 = 0, HW3 bit 46 = 1 적용 및 bit 47 유지
- [x] CAN 659 AutoLC 수신/송신·API·UI·테스트 경로 제거
- [x] 기존 Web UI/API에 Summon 상태·카운터 표시
- [x] 집중 native 회귀 테스트 추가
- [x] printf 검사·native 테스트·`pio run -e lilygo_t2can` 실행
- [x] 완료 내용을 세션에 기록
