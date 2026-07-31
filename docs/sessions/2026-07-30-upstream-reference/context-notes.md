# 외부 검증 원본 기준 컨텍스트 노트

## 고정 참조 저장소

- Nag Killer: https://github.com/06066060606060/nag-killer
- Nag Killer 기준 파일: https://github.com/06066060606060/nag-killer/blob/main/Nag-killer.ino
- Summon Unlock: https://github.com/06066060606060/Summon-Unlock
- Summon Unlock 기준 파일: https://github.com/06066060606060/Summon-Unlock/blob/main/summon_unlock.ino

앞으로 Nag Killer, Summon Unlock, EU Unlock 관련 CAN ID·비트·주기·조건을
변경하거나 실차 로그를 판정할 때 위 두 저장소의 최신 `main`을 먼저 비교한다.
Web UI·BLE·Wi-Fi 구현 차이와 CAN 알고리즘 차이를 분리해서 검토하고, 검증 원본에
없는 송신 조건이나 비트 변경은 사용자 확인 없이 추가하지 않는다.

## 2026-07-30 비교 기준

- Nag Killer 최신 확인 커밋: `4c7011ad4a567450a6b44cb0d1346efd34f69e5d`
- Summon Unlock 최신 확인 커밋: `9d5ffe9eff922084872dee009632ea1ad644100f`
- Summon Unlock 최신 변경은 대시보드 HTML 추가이며 CAN 로직은 프로젝트에
  보관된 `summon_unlock.ino`와 동일하다.
- Summon 게이트는 `Parked || Summoning`이다. CAN 921 AP 상태는 진단 표시용이며
  게이트 조건이 아니다.
- mux 1의 bit 19 해제가 EU Unlock이고, 차량 HW3 빌드에서는 bit 46을 적용한다.
- Nag Killer 최신 원본은 Mode A, Mode B, Custom만 제공하며, 이전 Mode C
  상태기계는 최신 원본에서 제거되었다. 현재 프로젝트의 Mode 3은 레거시
  호환 경로로 취급하고 신규 권장 경로로 단정하지 않는다.
- 현재 프로젝트의 새 기본값은 실차에서 동작이 확인된 Mode 2와 AP 전용
  주입 범위다. 기존 NVS의 유효한 선택값은 강제로 변경하지 않는다.

## 2026-07-31 사용자 승인 변경

- ECE R79 조건은 비교 실험을 위해 NVS/Web UI 토글로 분리한다.
- 조건 제한 ON은 `Parked || Summoning || AP 상태 3~6 안정 1초`, OFF는
  수신한 mux 1마다 송신을 시도한다. HW3 bit 19=0, bit 46=1은 바꾸지 않는다.
- 최신 검증 원본과 실차 결과에 따라 Mode 3과 런타임 ID 297 경로를 삭제한다.
  과거 NVS Mode 3 값은 부팅 시 Mode 2로 치환한다.
- OTA 시작 전 A MCP2515는 Listen-Only, B TWAI는 정지 상태로 전환하며,
  실패 후에도 재부팅 전까지 물리 TX 차단을 유지한다.

## 2026-07-29 실차 로그 판정과 조치

- MODE 3 주입 카운터는 `3503 → 11944`로 증가했으므로 송신 자체가 멈춘 것은
  아니지만, USER_MARK 경고 구간에서 경고 억제에는 실패했다. 최신 원본도
  이전 Mode C를 제거했으므로 MODE 3을 임의 보정하지 않고 레거시 비권장으로
  유지한다.
- Summon/EU Unlock 송신 성공은 로그에서 확인되었다. 원본 조건대로 일반주행·AP
  중에는 작동하지 않고 `Parked || Summoning`일 때만 ID 1021을 수정한다.
- A채널은 BUS-OFF 0, TEC/REC/MERRF 0이었지만 TX Fail 37회 증가, RX 오버런
  301회 증가, TX Guard 활성 107/240 표본(44.6%)이었다.
- A 폴링 최대 공백은 약 2.34ms라 RX 오버런을 태스크 지연만으로 설명할 수
  없었다. 라이브러리 `clearRXnOVR()`가 CANINTF 전체를 지워 새 RX 인터럽트까지
  없앨 수 있는 경로를 발견해 오버런 플래그와 ERRIF만 지우도록 수정했다.
- TEC/MERRF 없는 단발 TX Fail마다 15초 Guard를 시작하던 동작은 one-shot의
  정상적인 단발 중재 손실까지 장애로 확대할 수 있었다. EFLG·TEC 보호는 즉시
  유지하고, TX Fail 사유는 1초 안에 2회 이상일 때만 Guard를 시작한다.
