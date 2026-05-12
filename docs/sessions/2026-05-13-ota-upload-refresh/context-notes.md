# Context Notes - OTA upload refresh and 1.3.2

확인.
- 정상 Web UI `uploadOta()`는 `/api/ota` 성공 시 `업로드 완료. 재부팅 중...`만 표시하고 reload를 예약하지 않는다.
- `poll()`은 `_otaUploadInProgress`가 true면 즉시 return한다. 성공 후 onloadend에서 false로 돌아오지만, 재부팅 중 fetch 실패 상태만 보여줄 뿐 새 펌웨어 페이지 reload는 강제하지 않는다.
- 두 번째 `uploadOta()`는 `WEB_RECOVERY_UI_HTML` 내부의 OTA 복구모드 전용 UI다. 정상 OTA 탭 문제와 별개다.

결정.
- 정상 OTA 탭 success path에만 새로고침 예약을 추가한다.
- 버튼은 성공 후 계속 비활성 상태로 두어 중복 업로드를 막는다.
- 복구모드 UI는 이번 요청 범위에서 건드리지 않는다.

수정 결과.
- `scheduleOtaReload()`를 추가했다.
- 정상 OTA upload 성공 시 9초 후 `/api/status`를 2초 간격으로 확인하고, 응답이 살아나면 `location.reload()`를 호출한다.
- 성공 후 `_otaUploadInProgress`를 유지해 기존 `poll()`이 재부팅 대기 문구를 덮어쓰지 않게 했다.
- 실패/timeout/abort 경로는 기존처럼 버튼을 다시 활성화하고 poll을 재개한다.
- `include/version.h`와 `VERSION`을 `1.3.2`로 올렸다.

검증.
- VS Code diagnostics: `include/web/web_ui.h`, `include/version.h` 오류 없음.
- `pio run -e lilygo_t2can` 통과. Build info `FW132-2605130106-lilygot2can-87001e2-3281f603e852D`, RAM 106,728 bytes, Flash 984,129 bytes.
- `git diff --check -- include/web/web_ui.h include/version.h VERSION docs/sessions/2026-05-13-ota-upload-refresh docs/sessions/2026-05-13-ota-upload-tx-stop docs/sessions/chat_log_2026-05-13.md` 통과. `DIFF_CHECK_STATUS=0`.
