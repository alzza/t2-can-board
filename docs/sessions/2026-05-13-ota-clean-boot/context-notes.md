# Context Notes - OTA clean boot feature reset

확인.
- OTA 업로드 완료 시 기존 펌웨어가 `ota_pending=1`, `ota_fallback`, `ota_expect_pt`를 NVS에 저장한다.
- 새 펌웨어 첫 부팅에서 `otaBootCheck()`가 `pending==1`을 `2`로 전이한다.
- 현재 NVS 최초 초기화 블록은 `ota_pending!=0`이면 erase 없이 설정을 보존한다.
- OTA rollback 키가 같은 namespace에 있으므로 `nvs_erase_all()`을 그대로 쓰면 rollback 상태가 사라진다.

결정.
- OTA 첫 부팅에서는 전체 NVS erase 대신 차량 통신/기능 스위치 키만 OFF 또는 stock으로 덮는다.
- `ota_pending`, `ota_fallback`, `ota_expect_pt`는 보존한다.
- `a_ch_tx`도 false로 저장해 A채널 수정 송신이 사용자 재활성화 전까지 불가능하게 한다.

수정 결과.
- `applyOtaSafeFeatureRuntimeDefaults()`는 CAN task 생성 전 메모리 runtime을 OFF/stock으로 내린다.
- `writeOtaSafeFeatureSettings()`는 OTA 첫 부팅에서 차량 통신 기능 NVS 키를 OFF/stock으로 저장한다.
- `otaBootCheck()`의 `pending==1` 경로가 기능 안전 초기화 후 `pending=2`로 전이한다.
- `nagKillerRuntime`은 빌드 기본 true지만 OTA 첫 부팅에서는 runtime과 NVS 모두 false가 된다.
