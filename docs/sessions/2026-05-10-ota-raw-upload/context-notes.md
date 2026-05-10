# 2026-05-10 OTA raw upload 컨텍스트 노트

- 원인 후보: UI는 `FormData` multipart, 서버 `otaHandler()`는 raw firmware body를 `Update.write()`에 그대로 기록.
- Arduino `Update.write()`는 첫 바이트가 `0xE9`가 아니면 `UPDATE_ERROR_MAGIC_BYTE`로 abort.
- 현재 `firmware.bin` 첫 바이트는 `0xE9`, multipart 첫 바이트는 boundary의 `-`.
- 기존 차량에 올라간 구 UI는 수정되지 않으므로 최초 1회는 브라우저 버튼 대신 `curl --data-binary @firmware.bin` raw POST가 필요할 수 있다.
- 새 펌웨어부터는 웹 UI 버튼 업로드가 raw 방식으로 동작해야 한다.