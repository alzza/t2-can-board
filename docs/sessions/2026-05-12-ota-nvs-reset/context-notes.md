# Context Notes - OTA NVS Reset Button

확인.

- 앱 설정은 `canmod` NVS namespace에 저장된다.
- 같은 namespace에 OTA 상태 키 `ota_pending`, `ota_fallback`, `ota_expect_pt`, 최초 초기화 키 `nvs_init_ok`도 있다.
- 부팅 시 `nvs_init_ok`가 없고 `ota_pending==0`이면 `canmod` namespace를 한 번 더 지운 뒤 `nvs_init_ok=1`을 기록한다.
- WiFi/AP 비밀번호는 현재 NVS가 아니라 `include/web/web_server.h`의 `AP_PASS` 상수로 고정되어 있다.

결정.

- 전체 NVS partition을 지우지 않고 앱이 사용하는 `canmod` namespace 전체를 지운다.
- 삭제 후 즉시 재부팅한다. 현재 런타임 변수는 재부팅 전까지 남기보다 재부팅으로 기본값을 다시 로드하는 편이 명확하다.
- OTA pending 상태에서 누르면 OTA rollback metadata도 지워질 수 있으므로 UI 확인창 문구에 이를 명시한다.
- AP 비밀번호는 NVS reset 대상이 아니므로 유지된다. UI 확인창에도 `WiFi/AP 비밀번호는 유지됩니다`를 명시한다.
- API 이름은 `/api/nvs-reset`으로 둔다.

검증 결과.

- VS Code diagnostics: `include/web/web_server.h`, `include/web/web_ui.h`, `scripts/mock_webui_server.mjs` 오류 없음.
- mock Web UI에서 헤더가 `TeslaCAN`, `HW3`, `MOCK-LOCAL`로 보이고 OTA 탭에 `모든 NVS 초기화` 버튼이 표시됨을 확인했다.
- mock `/api/nvs-reset` 응답은 `{"ok":true,"erased":true,"restarting":true,"mock":true}`다.
- `node --check scripts/mock_webui_server.mjs` 통과.
- `pio run -e lilygo_t2can` 통과. RAM 106,616 bytes, Flash 971,497 bytes.
- `git diff --check -- include/web/web_server.h include/web/web_ui.h scripts/mock_webui_server.mjs docs/sessions/2026-05-12-ota-nvs-reset` 통과.