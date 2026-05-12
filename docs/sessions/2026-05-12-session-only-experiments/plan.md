# 2026-05-12 실험 기능 세션 전용화 계획

목표.
- 검증 전 실험 탭 기능은 재부팅 후 항상 기본값으로 돌아가게 한다.
- 과거 NVS 값이 남아 있어도 0x293/0x3F8 실험 주입이 자동 복원되지 않게 한다.
- 0x293은 payload를 실제로 바꿀 때만 counter/checksum을 갱신하고 송신한다.

범위.
- `UI_ulcOffHighway`, `UI_autoLaneChangeEnable`, `UI_ulcSpeedConfig`, `UI_ulcBlindSpotConfig`를 세션 전용으로 바꾼다.
- 기존 제어 탭에도 있는 `UI_alcOffHighwayEnable`은 이번 범위에서 persistence를 유지한다.
- 신호 관찰기와 로그 저장은 계속 수신/HTTP 전용으로 둔다.

성공 기준.
- 부팅 시 위 4개 실험 항목은 NVS를 읽지 않고 기본값으로 초기화된다.
- 위 4개 API 변경은 런타임에만 반영되고 NVS에 저장되지 않는다.
- 긴급 복원으로 위 4개 실험 항목이 다시 켜지지 않는다.
- 0x293 AutoLC runtime이 꺼져 있으면 0x293 수신 시 송신하지 않는다.
- `pio test -e native -f test_native_helpers -f test_native_hw3_autolc`가 통과한다.
- `pio run -e lilygo_t2can`가 통과한다.