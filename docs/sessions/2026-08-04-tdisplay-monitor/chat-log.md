# 채팅 로그 - T-Display-S3 모니터 연동

## 확정된 방향

- `/Users/akanus/T-Display-S3`를 독립 PlatformIO·Git 프로젝트로 계속 사용한다.
- T-Display-S3를 `t2-can-board` 하위로 복사하거나 Git 저장소를 중첩하지 않는다.
- T2-CAN과 T-Display-S3는 Wi-Fi AP/STA로 자동 연결하고 T-Display-S3는 읽기 전용으로 동작한다.
- 기존 T-Display-S3 UI·버튼·밝기 구조를 재사용하고 ESP-NOW 수신부를 HTTP 수신부로 교체한다.

## 구현 결과

- T2-CAN에 스키마 1의 `GET /api/monitor`를 추가했다.
- T-Display-S3는 1초 폴링, 2초 재연결, 3.2초 `NO SIGNAL` 정책으로 변경했다.
- 화면은 `OVERVIEW / A CHANNEL / B CHANNEL / FEATURES / SYSTEM` 5페이지로 구성했다.
- T2-CAN native 테스트 104개와 양쪽 실제 펌웨어 빌드가 성공했다.
- 두 장치의 실기 자동 연결과 화면 복구는 차량 보드·디스플레이 업로드 후 확인해야 한다.
