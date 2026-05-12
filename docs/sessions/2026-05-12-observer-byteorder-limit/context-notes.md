# Context Notes - Signal Observer Byte Order And Limits

확인.

- 현재 관찰기 펌웨어는 little-endian raw 추출만 지원한다.
- 현재 업로드 API와 mock 서버는 `byte_order`가 little이 아니면 거부한다.
- 현재 T-CAN observer JSON 생성기도 little이 아니면 실패한다.
- 관찰기 슬롯은 최대 10개지만 A채널 MCP2515 필터 ID 한도는 기존 기본 ID `659`, `1016`, `1021`을 포함해 6개다.
- 따라서 A 또는 A+B 신호가 새로 추가할 수 있는 고유 ID는 기본 ID와 중복을 제외하고 최대 3개다.

결정.

- 전체 신호 슬롯 `10`은 유지한다. B-only 신호는 A필터를 쓰지 않으므로 10개까지 관찰 가능하다.
- A필터 제한은 펌웨어가 이미 최종 거부하지만, 생성기에서도 기본 ID 3개를 포함해 사전 검사한다.
- `byte_order`는 `little`, `big` 문자열로 저장한다. T-CAN의 `byteOrder` 값을 그대로 사용한다.
- 수동 JSON 호환성을 위해 `byte_order`가 없으면 기존처럼 little로 해석한다.

구현 결과.

- `SignalObserverDef`와 이벤트 CSV에 `byte_order`를 추가했다.
- little은 기존 64-bit little payload shift 방식, big은 T-CAN/DBC Motorola bit position 순서로 raw를 조립한다.
- `/api/signal-observer/config`는 `little`, `intel`, `big`, `motorola`를 받고 잘못된 값은 400으로 거부한다.
- T-CAN observer JSON 생성기는 `little`과 `big`을 모두 출력하고, 기본 A ID `0x293`, `0x3F8`, `0x3FD` 포함 6개 제한을 사전 검사한다.
- 생성 JSON metadata에는 `maxSignals`, `maxAFilterIds`, `aFilterIds`, `aFilterUsed`, `aFilterRemaining`을 포함한다.
- README와 `docs/tcan_observer.json` 예시를 새 제한 설명과 metadata에 맞췄다.

검증 결과.

- VS Code diagnostics: 수정한 주요 파일 오류 없음.
- `pio test -e native -f test_native_helpers` 통과. 30 test cases, 30 succeeded.
- `.venv/bin/python -m py_compile scripts/tcan_signal_observer_json.py` 통과.
- big-endian 생성 스모크: `EPAS3P_torsionBarTorque`, `EPAS3P_handsOnLevel`이 `byte_order: big`으로 생성됨.
- A필터 초과 스모크: `--max-a-filter-ids 3`에서 `A-channel filter ID limit exceeded`로 실패함을 확인.
- `node --check scripts/mock_webui_server.mjs` 통과.
- `pio run -e lilygo_t2can` 통과. RAM 106,616 bytes, Flash 969,485 bytes.