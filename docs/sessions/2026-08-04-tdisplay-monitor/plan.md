# 계획 - T-Display-S3 읽기 전용 모니터 연동

- T2-CAN은 기존 AP·CAN 처리를 유지하고 경량 `GET /api/monitor` 응답만 추가한다.
- T-Display-S3는 `TeslaCAN` AP에 자동 연결하고 1초 주기로 모니터 API를 읽는다.
- T-Display-S3에서 차량 기능 제어, CAN 송신, POST 요청을 제공하지 않는다.
- API 스키마 버전을 검증하고 3초 이상 응답이 없으면 `NO SIGNAL`을 표시한다.
- 양쪽 펌웨어를 독립 빌드·검증하고 기존 미커밋 변경은 보존한다.
