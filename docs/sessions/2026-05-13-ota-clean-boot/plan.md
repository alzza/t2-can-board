# Plan - OTA clean boot feature reset

목표.
- OTA 업로드 후 새 펌웨어 첫 부팅에서 이전 NVS 기능 설정을 무시한다.
- 차량 통신에 영향을 줄 수 있는 스위치는 모두 OFF 또는 stock 값으로 시작한다.
- OTA rollback 상태 키는 보존한다.

검증.
- `ota_pending==1` 첫 부팅 경로에서 기능 reset helper가 호출되는지 코드로 확인한다.
- `pio run -e lilygo_t2can` 빌드 성공을 확인한다.
- `git diff --check`로 패치 공백 오류를 확인한다.
