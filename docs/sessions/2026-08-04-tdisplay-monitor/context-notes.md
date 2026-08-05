# 컨텍스트 노트 - T-Display-S3 모니터 연동

- T2-CAN AP는 `TeslaCAN`, WPA2 비밀번호 `asdf1234`, 채널 1, 주소 `192.168.4.1`을 사용한다.
- 기존 T-Display-S3는 ESP-NOW 채널 1에서 예전 22/52/56/60바이트 페이로드를 받았으며 현재 T2-CAN은 ESP-NOW를 송신하지 않는다.
- `/api/status`는 Web UI·로그·OTA·상세 진단을 함께 만들어 주기적 전용 모니터에는 크다. 고정 버퍼 기반 경량 API를 분리한다.
- T-Display-S3 사용자 변경은 `README.md`, `src/main.cpp`, `src/ui.cpp`, `src/ui.h`에 남아 있으므로 해당 기능을 보존하며 수정한다.
- API 응답은 2048바이트 스택 고정 버퍼를 사용하고 `Cache-Control: no-store`를 지정한다. 요청 처리 중 동적 JSON 생성, NVS·로그 쓰기와 CAN 송신을 하지 않는다.
- T-Display-S3 HTTP 연결 제한시간은 500ms, 응답 제한시간은 700ms이며 마지막 정상 응답 3.2초 뒤 `NO SIGNAL`로 판정한다.
- T-Display-S3의 과거 `wifi_on`, `bt_on` NVS 값은 무시한다. 자동 연결에 필요한 Wi-Fi는 항상 켜고 Bluetooth는 끈다.
