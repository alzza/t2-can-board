# 2026-05-12 CRC/checksum 정밀 검토 컨텍스트 노트

확인한 사실.
- `signalObserverObserveFrame()`은 수신 프레임의 raw 값과 카운터만 갱신하며 CAN 송신을 호출하지 않는다.
- `/api/signal-observer/capture`는 `signalObserverRuntime`과 통계만 변경한다.
- `/api/logs-bundle`은 HTTP 응답 chunk를 작성할 뿐 CAN 송신 경로를 호출하지 않는다.
- T-CAN/DBC 기준 0x293 `UI_chassisControl`에는 `UI_chassisControlCounter : 52|4@1+`와 `UI_chassisControlChecksum : 56|8@1+`가 있다.
- T-CAN/DBC 기준 0x3F8 `UI_driverAssistControl`의 현재 실험 신호에는 checksum/counter 필드가 없다.
- 현재 `HW3Handler`의 0x293 AutoLC 주입은 payload byte3만 변경하고 counter/checksum을 갱신하지 않은 채 `sendCheck()`를 호출한다.

판단.
- 차량 에러 메시지가 관찰기 저장 직후처럼 보인 것은 저장 동작 자체보다 실험 토글이 켜진 상태에서 0x293 변조 프레임이 유효하지 않은 checksum/counter로 송신된 상관관계일 가능성이 높다.
- 보정은 0x293 AutoLC 경로에 한정하는 것이 현재 근거에 맞는 최소 수정이다.

수정 결과.
- 0x293 `UI_chassisControl` AutoLC 주입 직전에 byte 6 upper nibble counter를 +1 modulo 16으로 갱신하고 byte 7 checksum을 재계산한다.
- `dlc < 8` 프레임은 0x293 AutoLC 주입 대상에서 제외한다.
- 0x3F8 `UI_driverAssistControl` 실험 신호는 현재 T-CAN/DBC 근거상 checksum/counter 보정을 추가하지 않는다.

검증 결과.
- `pio test -e native -f test_native_helpers -f test_native_hw3_autolc` 통과. 30 test cases, 30 succeeded.
- `pio run -e lilygo_t2can` 통과. SUCCESS, RAM 93,288 bytes, Flash 955,465 bytes.
- 이전에 `test_native_hw3` 전체를 함께 실행했을 때 실패한 1021/787 테스트는 이번 0x293 보정과 무관한 기존 legacy expectation이라 전용 테스트로 분리했다.
