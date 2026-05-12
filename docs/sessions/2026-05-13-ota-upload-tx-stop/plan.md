# Plan - OTA upload CAN TX stop

목표.
- OTA 업로드 요청을 받으면 flash write 전에 차량 기능 런타임과 NVS 값을 OFF/stock으로 내린다.
- TWAI driver 자체는 강제로 `twai_stop()`하지 않는다. ESP-IDF v4.4 문서상 `twai_stop()`은 진행 중 프레임을 즉시 중단할 수 있어 업로드 직전 차량 버스에는 더 위험할 수 있다.
- 펌웨어 버전을 1.3.1로 올린다.

검증.
- OTA handler에서 `Update.begin()` 전에 TX 안전 초기화가 실행되는지 코드로 확인한다.
- `pio run -e lilygo_t2can` 빌드 성공을 확인한다.
- `git diff --check`로 수정 파일 공백 오류를 확인한다.