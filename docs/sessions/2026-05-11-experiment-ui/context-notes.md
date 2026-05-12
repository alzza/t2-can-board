# 실험 탭 ULC/ALC 비트 제어 컨텍스트 노트

## Plan

1. 기존 A채널 0x3F8 `UI_driverAssistControl` 주입 경로를 확장한다.
2. `실험` 탭을 메인 탭 뒤에 추가하고 실험 신호 제어만 배치한다.
3. `UI_ulcOffHighway`, `UI_autoLaneChangeEnable`, `UI_ulcSpeedConfig`, `UI_ulcBlindSpotConfig`를 새 런타임 설정으로 추가한다.
4. `UI_alcOffHighwayEnable`은 이미 구현된 런타임/NVS/API를 재사용한다.
5. 통합 로그에는 `/api/logs-bundle` 스냅샷과 런타임 로그 둘 다 남도록 한다.

## 확인한 근거

- `UI_driverAssistControl`은 ID 0x3F8(1016), 길이 8, ModelY_CH 기준이다.
- `UI_ulcOffHighway`: bit15, 1bit.
- `UI_alcOffHighwayEnable`: bit56, 1bit, 기존 구현 있음.
- `UI_ulcSpeedConfig`: bit50-51, 2bit, 값 0 DISABLED / 1 MILD / 2 AVERAGE / 3 MAD_MAX.
- `UI_ulcBlindSpotConfig`: bit52-53, 2bit, 값 0 STANDARD / 1 AGGRESSIVE / 2 MAD_MAX.
- `UI_autoLaneChangeEnable`은 ID 0x293 `UI_chassisControl` bit24-25라 0x3F8과 다른 프레임이다.

## 결정

- 요청한 기존 틀 유지 기준으로 새 UI는 별도 `실험` 탭에 두고, 기존 `제어` 탭의 A채널 구조는 유지한다.
- 0x3F8 신호는 기존 `HW3Handler`의 1016 경로에서 한 번에 송신한다.
- 0x293 `UI_autoLaneChangeEnable`은 별도 프레임이므로 A채널 필터와 주입 경로에 ID 659를 추가한다.

## 완료

- `실험` 탭을 `메인/제어/진단/OTA` 뒤에 추가했다.
- `UI_autoLaneChangeEnable`, `UI_ulcOffHighway`, `UI_alcOffHighwayEnable`은 스위치로 제어한다.
- `UI_ulcSpeedConfig`, `UI_ulcBlindSpotConfig`는 순정 유지 + raw 선택 라디오로 제어한다.
- `/api/logs-bundle` 채널 스냅샷과 런타임 로그에 실험 설정/카운터를 남긴다.

## 검증

- `node --check scripts/mock_webui_server.mjs` 통과.
- `pio run -e lilygo_t2can` 성공. RAM 92,192 bytes, Flash 942,833 bytes.
- mock Web UI에서 `실험` 탭 표시, AutoLC/ULCOffHW ON, Speed AVERAGE, Blind AGGRESSIVE 선택 후 `/api/status` 반영 확인.
- mock `/api/logs-bundle`에서 `UI_ulcOffHighway ON=1 AutoLC=1 Speed=AVERAGE Blind=AGGRESSIVE` 라인 확인.
