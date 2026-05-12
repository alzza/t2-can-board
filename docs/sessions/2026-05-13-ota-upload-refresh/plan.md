# Plan - OTA upload refresh and 1.3.2

목표.
- 정상 OTA 탭에서 업로드 성공 후 페이지가 재부팅 상태에 멈추지 않게 한다.
- 업로드 성공 응답을 받으면 보드 재부팅 시간을 기다린 뒤 Web UI를 새로고침한다.
- 1.3.1 실차 확인 결과와 1.3.2 변경 내용을 세션 로그에 남긴다.
- 펌웨어 버전을 1.3.2로 올린다.

검증.
- `uploadOta()` success path가 reload를 예약하는지 코드로 확인한다.
- `pio run -e lilygo_t2can` 빌드 성공을 확인한다.
- `git diff --check`로 수정 파일 공백 오류를 확인한다.
