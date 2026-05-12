# Context Notes - OTA firmware metadata display

확인.
- 기존 OTA 탭은 `ota_current_label`, `ota_fallback_label`만 표시한다.
- 현재 펌웨어 상세 메타는 `/api/status` root에 이미 있다.
- 이전 펌웨어 상세 메타는 OTA 업로드 순간 NVS에 저장하지 않으면 새 펌웨어에서 알 수 없다.
- 기존 업로드 handler는 `Update.end(true)` 후 `esp_ota_get_next_update_partition(NULL)`를 호출한다. 이 값은 기대 파티션 label 저장에 불안정할 수 있으므로 업로드 시작 전에 target partition을 잡아야 한다.

결정.
- OTA 업로드 시작 전에 target partition label을 잡고 `ota_expect_pt`에 저장한다.
- upload handler가 현재 실행 중인 펌웨어 메타를 fallback 메타로 저장한다.
- 새 펌웨어 첫 부팅 `pending==1`에서 자기 메타를 new 메타로 저장한다.
- rollback 후 `pending==3`에서는 표시용 이전 파티션을 실패한 새 펌웨어 쪽으로 돌린다.

수정 결과.
- `ota_expect_pt` 저장에 쓰는 target partition label을 `Update.end(true)` 이후가 아니라 업로드 시작 전에 잡는다.
- `pending==1` 첫 부팅에서 expect mismatch가 있으면 USB 플래시로 오판하지 않고 현재 실행 파티션 label로 보정한다.
- OTA upload 시 fallback 펌웨어 메타와 browser upload timestamp를 NVS에 저장한다.
- 새 펌웨어 첫 부팅 시 new 펌웨어 메타를 NVS에 저장한다.
- rollback 후 `pending==3`에서는 `ota_fallback` 표시 label과 fallback meta를 실패한 새 펌웨어 쪽으로 돌린다.
- NVS 메타가 없는 이전 펌웨어도 `esp_ota_get_partition_description()`으로 partition app version/project/date/time을 읽어 표시한다.
- OTA 탭은 현재/이전 파티션, 현재/이전 펌웨어, 빌드 시각, 업로드 시각을 표시한다.

검증.
- VS Code diagnostics: `include/web/web_server.h`, `src/main.cpp`, `include/web/web_ui.h` 오류 없음.
- `pio run -e lilygo_t2can` 통과. SUCCESS, RAM 106,728 bytes, Flash 982,965 bytes.
- `git diff --check -- src/main.cpp include/web/web_server.h include/web/web_ui.h docs/sessions/2026-05-13-ota-firmware-meta docs/sessions/chat_log_2026-05-13.md` 통과. `DIFF_CHECK_STATUS=0`.
