# 2026-07-31 컨텍스트 노트

## 확정 사항

- 대상 차량은 Tesla HW3이다.
- CAN-A는 Summon Unlock/EU Unlock/TSLLC, CAN-B는 Nag Killer 전용이다.
- bit 19는 ECE R79 제한 해제, HW3 bit 46은 Summon 제한 해제로 확정한다.
- 송신 허용 조건은 사용자가 ON/OFF로 비교 실험한다.
- 조건 ON은 `Parked || Summoning || AP 상태 3~6 안정 1초`이며 AP 주행 중 bit 19/46을 함께 적용한다.
- ON 기본값은 `Parked || Summoning || AP 상태 3~6 안정 1초`다.
- OFF는 상태 게이트만 해제하며 Summon 기능, A TX 마스터, TX Guard는 유지한다.
- HW3 mux 1 수정 비트는 bit 19=0, bit 46=1이다.
- Nag MODE 3과 실험용 ID 297 런타임 경로는 삭제한다.
- Nag 기본값은 MODE 2와 오토파일럿 전용이다.

## OTA 안전 원칙

- 사용자가 OTA 전에 A TX와 Nag Killer를 먼저 끄는 절차는 계속 권장한다.
- 펌웨어도 OTA 시작 직전에 기능 OFF를 NVS에 저장한다.
- 진행 중 송신 호출을 배출한 뒤 A MCP2515는 Listen-Only, B TWAI는 정지한다.
- OTA가 실패해도 TX를 자동 복원하지 않으며 재부팅 전까지 차단한다.
- 새 펌웨어 첫 부팅은 차량 기능 OFF 값을 CAN 초기화 전에 적용한다.

## 로그 판정 원칙

- A TX 큐 등록은 실제 버스 전송 완료와 동일하지 않다.
- Busy는 MCP2515 TX 버퍼가 사용 중인 상태이며 하드 오류로 보지 않는다.
- 실제 완료, 중재 손실, 중단, 컨트롤러 오류를 별도 카운터로 비교한다.
- 송신 허용 조건, AP 상태·안정시간, 게이트 사유를 상태 API와 시계열 CSV에 남긴다.
