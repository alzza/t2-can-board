# 2026-05-12 실험 기능 세션 전용화 컨텍스트 노트

확인한 사실.
- `UI_autoLaneChangeEnable` 0x293 송신 경로는 `uiAutoLaneChangeEnableRuntime`이 켜진 경우에만 들어간다.
- 기존 코드는 `auto_lc`, `ulc_offhwy`, `ulc_speed`, `ulc_blind`를 NVS에서 읽어 부팅 시 복원했다.
- 그래서 사용자가 해당 운행에서 UI를 건드리지 않아도 이전 세션 NVS 값이 남아 있으면 실험 주입이 켜질 수 있었다.
- 0x293 `UI_chassisControl`은 counter/checksum이 있는 프레임이므로 자동 복원 persistence와 특히 궁합이 나쁘다.

결정.
- 검증 전 실험 탭 항목은 세션 전용으로 둔다.
- 제어 탭 승격 전까지는 재부팅 후 기본값 복귀가 기본 정책이다.
- 0x293은 runtime OFF면 수신 카운트만 하고 송신하지 않는다.
- runtime ON이고 raw 값 변경이 필요할 때만 payload를 바꾸고 counter/checksum을 갱신한다.

수정 결과.
- `loadAExperimentSettings()`는 `UI_ulcOffHighway`, `UI_autoLaneChangeEnable`, `UI_ulcSpeedConfig`, `UI_ulcBlindSpotConfig`를 더 이상 NVS에서 읽지 않고 기본값으로 초기화한다.
- 위 4개 실험 API는 런타임만 바꾸고 NVS에 저장하지 않는다. 이때 필요한 A TX dependency도 세션 안에서만 켠다.
- 긴급 복원은 위 4개 실험 항목을 백업값에서 복원하지 않고 기본값으로 둔다.
- `UI_alcOffHighwayEnable`은 기존 제어 탭 기능이라 persistence를 유지했다.

검증 결과.
- VS Code diagnostics: 수정 파일 오류 없음.
- `pio test -e native -f test_native_helpers -f test_native_hw3_autolc` 통과. 31 test cases, 31 succeeded.
- `pio run -e lilygo_t2can` 통과. SUCCESS, RAM 93,288 bytes, Flash 955,169 bytes.