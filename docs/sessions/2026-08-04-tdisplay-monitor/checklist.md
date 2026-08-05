# 체크리스트 - T-Display-S3 모니터 연동

- [x] 두 저장소의 기존 미커밋 변경 확인.
- [x] `GET /api/monitor` 스키마 1 구현.
- [x] T-Display-S3 `TeslaCAN` 자동 연결·재연결 구현.
- [x] 1초 폴링·3.2초 `NO SIGNAL`·스키마 불일치 처리.
- [x] 5페이지 UI를 현재 A/B·기능·게이트·시스템 필드로 갱신.
- [x] T2-CAN native 회귀 테스트 104개 통과·펌웨어 빌드 성공.
- [x] T-Display-S3 JSON 파서·펌웨어 빌드 성공.
- [x] 양쪽 README·CHANGELOG 한글 갱신.
- [x] 날짜 규칙 BIN 복사본·SHA-256 생성.
- [x] 사용자 실기 확인에서 S3 디스플레이 자동 연결과 상태 화면 정상 표시.
- [ ] T2-CAN 재부팅 시 `NO SIGNAL` 표시 후 자동 복구와 무제어 동작을 별도 확인.
