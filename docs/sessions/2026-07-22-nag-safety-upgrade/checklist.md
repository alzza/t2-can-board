# Nag Killer 안전 업그레이드 체크리스트

- [x] 사용자 승인 범위와 `[기본]` 프로파일 적용 의미 확인.
- [x] 현재 Nag 상태기계, NVS/OTA 기본값, TWAI 송신 경로 확인.
- [x] 새 NVS에서 Nag 기본 OFF 테스트 추가.
- [x] DAS·조향 신호 최근 수신 유효시간 차단 테스트 추가.
- [x] 모든 프로파일 최종 토크 ±1.80 Nm 테스트 추가.
- [x] TWAI 송신 실패 시 성공 카운터 미증가 테스트 추가.
- [x] 자체 echo 재수신 차단 테스트 추가.
- [x] 최소 코드 수정.
- [x] `pio test -e native_nag` 통과.
- [x] `pio test -e native` 통과.
- [x] `pio run -e lilygo_t2can` 통과.
- [x] 컨텍스트 노트와 한국어 변경 문서 갱신.
