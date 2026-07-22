# 1.3.4 릴리스 문서·버전 갱신 컨텍스트 노트

## 근거

- 대상 차량은 HW3이며, A채널 MCP2515는 Summon Unlock/TSLLC 전용이고 B채널 TWAI는 Nag Killer 전용이다.
- Summon Unlock은 `0x3FD` mux 1에서 기능 ON, A TX 마스터 ON, `Parked || Summoning`을 모두 만족할 때만 수정 프레임을 보낸다.
- HW3은 bit 19를 0, bit 46을 1로 적용한다. AP 상태는 모니터링 필드일 뿐 게이트 조건이 아니다.
- OTA 새 펌웨어 확인 창은 60초다. 첫 부팅은 차량 기능과 A TX를 OFF/stock으로 기록한 뒤 CAN 모니터링을 시작한다.

## 버전 결정

- 기존 `VERSION`과 `FIRMWARE_VERSION`은 `1.3.3`이지만 헤더 patch와 fallback build ID는 `1.3.2`에 머물러 있었다.
- 오늘의 사용자 대면 기능·안전 동작 변경을 반영해 다음 patch 릴리스인 `1.3.4`로 통일한다.

## 구현 및 검증 결과

- README에 HW3 Summon 제어 순서, 게이트 조건, bit 19/46, A TX 마스터 정지 방법, OTA 60초 확인 절차를 기록했다.
- docs-site의 Smart Summon 문서는 HW3 Conditional Summon Unlock으로 교체했고, 이전 EAP 문서는 퇴역 호환 안내로 전환했다.
- `python3 scripts/check_release_metadata.py`, `python3 scripts/sync_web_ui.py --check`, `python3 scripts/check_printf_formats.py`, `git diff --check`가 통과했다.
- `pio test -e native` 91/91, `pio test -e native_nag` 43/43, `pio run -e lilygo_t2can`이 통과했다.
- 최종 HW3 바이너리 SHA-256은 `1867f03e89d08493eac1cba5a5a5562436bcad286d463a0da770e203c4f19a76`이다.
- docs-site `npm run build`는 소스 문제가 아니라 로컬 Node가 `libllhttp.9.3.dylib`를 찾지 못해 실행되지 않았다.
