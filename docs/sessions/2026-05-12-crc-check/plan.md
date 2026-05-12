# 2026-05-12 CRC/checksum 정밀 검토 계획

목표.
- 실험 탭 관찰기와 로그 저장이 CAN 송신을 유발하는지 코드로 확인한다.
- A채널 실험 주입 프레임 중 checksum/counter가 필요한 프레임을 T-CAN/DBC 기준으로 확인한다.
- 확인된 누락만 최소 범위로 보정하고, 관련 테스트와 빌드로 검증한다.

성공 기준.
- 0x293 `UI_chassisControl` 주입 시 counter 52|4가 +1 되고 checksum 56|8이 재계산된다.
- 0x3F8 `UI_driverAssistControl`은 T-CAN/DBC 기준 checksum/counter 필드가 없음을 기록한다.
- `pio test -e native -f test_native_helpers -f test_native_hw3`가 통과한다.
- `pio run -e lilygo_t2can`이 통과한다.
