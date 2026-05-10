# 2026-05-10 1016 ALC 주입 계획

## 목표
- ID 1016 `UI_driverAssistControl`에서 `UI_ulcStalkConfirm` bit1을 false로 유지한다.
- ID 1016 `UI_alcOffHighwayEnable` bit56을 true로 유지한다.
- 이미 원하는 값이면 재송신하지 않는다.
- 통합 로그와 Web UI에서 1016 두 신호의 주입 상태를 각각 확인할 수 있게 한다.

## 근거
- `ModelY_CH.dbc.txt` 기준 ID 1016은 `UI_driverAssistControl`이다.
- bit1은 `UI_ulcStalkConfirm`, bit56은 `UI_alcOffHighwayEnable`이다.
- `ModelY_PARTY.dbc`의 ID 1016은 다른 버스 맥락으로 보이는 `VCBATT2_cameraCleaningLogging`이다.

## 성공 기준
- native HW3 테스트에서 1016 변경 시 1회 전송, 무변경 시 0회 전송을 검증한다.
- Web status JSON에 1016 수신과 두 신호별 주입/스킵 카운터가 노출된다.
- Web UI 제어 탭에 `UI_ulcStalkConfirm`, `UI_alcOffHighwayEnable` 스위치가 각각 표시된다.
- `pio run -e lilygo_t2can`가 성공한다.

## 추가 목표
- 제어 탭에서 `UI_ulcStalkConfirm` bit1과 `UI_alcOffHighwayEnable` bit56을 따로 ON/OFF 할 수 있게 한다.
- 각 신호가 이미 필요한 bit 값이라 해당 신호를 수정하지 않은 횟수를 스킵 카운터로 남긴다.
- 통합 로그, logs bundle, Web UI, mock 서버에서 두 신호의 주입/스킵 카운터를 함께 확인한다.
- 마지막 실차 로그 기준 Smart Torque 딜레이와 토크 조합 해석을 기록한다.
