# 2026-08-01 채팅 로그

## 2026-08-01 실차 로그 OTA·ECE R79 검증

사용자 요청.
- 이번 OTA 업데이트 중 차량 통신에러가 발생하지 않았으므로 이 규칙을 지침서에 적용해 달라고 요청했다.
- 오토파일럿 중 ECE R79 적용으로 차선 변경이 좋아진 것 같은지 로그로 검증해 달라고 요청했다.
- 로그 중 에러나 불안정 요소가 있으면 보고해 달라고 요청했다.

확인.
- 현재 OTA 구현은 업로드 전 새 A/B 수정 송신을 원자적으로 막고, 기능 NVS를 OFF/stock으로 저장한 뒤, 이미 시작된 송신 종료를 최대 250ms 기다린다.
- 이후 CAN-A MCP2515를 Listen-Only, CAN-B TWAI를 Stopped로 전환하고, OTA 실패 시 재부팅 전까지 CAN TX를 재개하지 않는다.
- 2026-08-01 실차 OTA에서는 이 절차 적용 후 차량 통신에러가 발생하지 않았다.
- 이벤트 로그의 19:13~19:14 구간에서 `AP_ACTIVE=1`, `SUMMON_GATE=1`, `SUMMON_TX=1`, `TSLLC_TX=1` 조합이 반복 확인됐다.
- 현재 코드에서 Summon TX는 ID `0x3FD` mux 1의 HW3 bit19=0, bit46=1을 함께 적용하므로, AP 중 `SUMMON_TX=1`은 ECE R79 해제 송신이 있었다는 뜻이다.
- B채널은 BUS-OFF, TEC, REC, TX Fail이 모두 0이다.
- A채널은 BUS-OFF, TEC, REC가 모두 0이라 차량 통신에러급 오류는 보이지 않는다.
- A채널에는 RX_OVERRUN과 TX Guard가 반복된다. 시계열 기준 `a_rx_overrun`은 300에서 680까지 증가했고, A TX hard error는 40에서 59까지 증가했다.

수정.
- `.github/copilot-instructions.md`에 OTA 실차 안전 규칙을 추가했다.
- `README.md`의 OTA 안전 절차에 2026-08-01 실차 성공 사례와 유지해야 할 순서를 추가했다.
- `docs/OTA_UPDATE_RECOVERY_TEST.md`에 실차 성공 사례, OTA 파일명 규칙, OTA용 bin 복사본 생성 절차를 반영했다.
- `CHANGELOG.md`에 문서 갱신 내용을 기록했다.

다음 액션.
- 다음 실차 로그에서는 AP 구간이 시계열 CSV에 포함되도록 캡처 시작 시점을 앞당긴다.
- A채널 RX 오버런 빈도와 TX Guard 반복 원인을 별도 작업으로 줄인다.
