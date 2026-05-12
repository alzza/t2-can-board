# Plan - OTA firmware metadata display

목표.
- OTA 업로드 대상 파티션 label 저장 시점을 보정한다.
- OTA 상태 JSON에 현재/이전 펌웨어 상세 메타와 업로드 시각을 노출한다.
- OTA 탭과 확인/복구 배너에서 버전, 빌드 ID, 빌드 시각, 업로드 시각을 표시한다.

검증.
- `pio run -e lilygo_t2can` 빌드 성공을 확인한다.
- `git diff --check`로 수정 파일 공백 오류를 확인한다.
