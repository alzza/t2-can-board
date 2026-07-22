# 컨텍스트 노트

- 차량 하드웨어: HW3
- 기준: 2026-05-14 커밋 `28d158b54abc5d1ee97d3b3dce5ff17110d2fbd0`
- 참조: 사용자가 제공한 검증 완료 `summon_unlock.ino`
- BLE 코드는 이식하지 않고 기존 Wi-Fi Web UI를 제어면으로 유지한다.
- INO의 CAN 해석을 그대로 보존한다. CAN 390의 데이터 바이트 2는 대체 경로이고 CAN 921은 진단 전용이다.
- 부팅/감시시간 초과 동작은 parked=true로 유지한다.
- 실험용 CAN 659 `UI_autoLaneChangeEnable`은 제거 대상이다.
