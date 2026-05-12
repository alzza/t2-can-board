# Context Notes - OTA upload CAN TX stop

확인.
- A채널 수정 송신은 `aChannelTxRuntime`이 false면 `shouldSkipATx()`에서 막힌다.
- B채널 Nag echo 송신은 `nagKillerRuntime`이 false면 `NagHandler::handleMessage()`에서 막힌다.
- `applyOtaSafeFeatureRuntimeDefaults()`와 `writeOtaSafeFeatureSettings()`는 이미 OTA 첫 부팅 안전 초기화에서 차량 기능을 OFF/stock으로 내린다.
- ESP-IDF v4.4 TWAI 문서는 OTA/flash write 중 ISR IRAM 설정이 필요할 수 있다고 설명한다. 현재 Arduino ESP32 2.0.17 S3 sdkconfig에서는 `CONFIG_TWAI_ISR_IN_IRAM`이 기본 비활성이라 무조건 IRAM ISR을 켜면 안 된다.
- ESP-IDF v4.4 `twai_stop()`은 TX/RX를 멈추고 TX queue를 clear하지만, 진행 중 프레임을 즉시 중단해 다른 노드가 에러로 해석할 수 있다고 경고한다.

결정.
- OTA upload 시작 시 driver stop/uninstall 대신 애플리케이션 송신 플래그를 먼저 OFF로 내린다.
- OFF 값을 NVS에도 저장해 upload 실패/재부팅 후에도 사용자가 다시 켜기 전까지 송신이 살아나지 않게 한다.
- 짧게 `delay(5)`로 CAN task가 새 runtime을 볼 시간을 준 뒤 `Update.begin()`과 flash write를 진행한다.

수정 결과.
- `prepareOtaUploadCanQuiet()`를 추가했다.
- OTA upload 요청에서 content type 확인 후 `Update.begin()` 전에 `prepareOtaUploadCanQuiet()`를 호출한다.
- helper는 `writeOtaSafeFeatureSettings()`로 A/B 차량 기능 런타임과 NVS를 OFF/stock으로 내리고, `twai_clear_transmit_queue()`로 B채널 대기 TX queue를 비운다.
- helper 실패 시 OTA upload를 500으로 중단한다.
- `include/version.h`와 `VERSION`을 `1.3.1`로 올렸다.

검증.
- VS Code diagnostics: `include/web/web_server.h`, `include/version.h` 오류 없음.
- `pio run -e lilygo_t2can` 통과. Build info `FW131-2605130053-lilygot2can-87001e2-488774e1234aD`, RAM 106,728 bytes, Flash 983,473 bytes.
- `git diff --check -- include/web/web_server.h include/version.h VERSION docs/sessions/2026-05-13-ota-upload-tx-stop docs/sessions/chat_log_2026-05-13.md` 통과. `DIFF_CHECK_STATUS=0`.

실차 확인.
- 사용자가 1.3.1 OTA upload 전 CAN TX OFF/stock 적용 후 차량 에러증상이 사라졌다고 보고했다.