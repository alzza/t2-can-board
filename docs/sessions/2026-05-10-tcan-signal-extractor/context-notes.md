# T-CAN 신호 상세 추출 도구 컨텍스트 노트

## 확인한 데이터 구조

- T-CAN Explorer는 SPA이며 `https://tcan.latency.is/data/manifest.json`을 먼저 로드한다.
- Frame 상세 데이터는 `/data/frames/<sanitized-frame-key>.json`에 있다.
- Sanitizing 규칙은 app bundle 기준 `key.replace(/[^A-Za-z0-9_]+/g, "_")`다.
- 예시로 `0x399__DAS_status`는 `/data/frames/0x399__DAS_status.json`처럼 요청된다.

## 이번 요청 신호의 주요 frame 후보

- `DAS_autopilotState`, `DAS_autopilotHandsOnState`는 ModelY CH 조건에서 `0x399__DAS_status`, decimal `921`에 있다.
- `EPAS3P_torsionBarTorque`, `EPAS3P_handsOnLevel`은 ModelY CH 조건에서 `0x52__EPAS3P_sysStatus`, decimal `82`에 있다.
- `SCCM_steeringAngle`은 ModelY VEH/CH 조건에서 `0x129__SCCM_steeringAngleSensor`, decimal `297`에 있다.

## 구현 판단

- 외부 패키지 없이 Python 표준 라이브러리만 사용한다.
- 자주 쓸 도구이므로 기본 URL, platform, bus, 출력 format을 CLI 옵션으로 둔다.
- T-CAN의 데이터 값은 원문 JSON을 기준으로 하고, derived DBC-style line과 physical formula는 읽기 편의를 위한 보조 출력으로만 둔다.
- 사람이 바로 확인할 비트 레이아웃은 byte별 bit7..bit0 격자로 출력한다.
- 격자 셀은 `raw[n]`으로 표기하며 `raw[0]`은 물리값 계산에 들어가는 raw 값의 LSB다.
- Markdown table만으로는 원문에서 눈에 덜 띄므로 `Visual bit map` 고정폭 코드블록을 먼저 출력하고, 표는 보조로 둔다.
- `test/test_tcan_signal_detail/test_tcan_signal_detail.py`는 little-endian, Motorola big-endian, Markdown table header를 검증한다.
- `--format html`은 standalone HTML 문서를 생성한다. 기본값은 계속 Markdown이며, JSON 포맷은 자동화/구조화 검증용으로 유지한다.
- HTML bit map은 byte/bit 격자에 `raw[n]`과 payload absolute bit를 같이 표시해 Markdown보다 눈으로 확인하기 쉽게 만든다.
- `--html` 단축 옵션도 지원해 `--format html`과 같은 동작을 한다.
- HTML 보강 단계에서는 bit table 좌측 byte 컬럼을 sticky 처리하고 used/unused/byte legend를 문서 상단에 둔다.
- HTML 본문은 signal 카드 나열 대신 frame 기준으로 묶고, 같은 frame 안에 포함된 signal chip과 signal별 상세 카드를 함께 배치한다.
- 후속 보강으로 legend 자체를 sticky 처리해 긴 문서 스크롤에서도 상태 의미를 유지한다.
- frame card 상단에 frame bit overlap map을 추가해 선택 신호 간 payload bit 중첩 여부를 색으로 빠르게 식별한다.

## 생성 결과

- 추가 스크립트는 `scripts/tcan_signal_detail.py`다.
- 생성 문서는 `docs/tcan_signal_reference_ModelY_VEH_CH.md`다.
- HTML 예시 문서는 `docs/tcan_signal_reference_ModelY_VEH_CH.html`로 생성한다.
- HTML은 signal별 anchor 링크, frame/layout card, styled bit-map table, enum table, pseudocode block을 포함하는 standalone 문서다.
- 최신 HTML은 frame anchor nav, frame card, signal chip 목록, sticky byte column, legend를 포함한다.
- overlap map의 겹침 셀은 경고 계열 색으로 표시하며, 셀 title로 해당 bit를 공유하는 signal 이름을 노출한다.
- JSON 검증 결과 `missingSignals=[]`, `matchCount=5`다.
- 요청 신호 5개는 모두 ModelY + VEH/CH filter에서 매칭됐다.
- `EPAS3P_torsionBarTorque`와 `EPAS3P_handsOnLevel`은 T-CAN ModelY CH 기준 `0x52` frame에 있다.
- 현재 프로젝트의 party-side `0x370` decimal `880` variant와는 bus/view 조건이 다르므로 문서 해석 때 bus context를 같이 봐야 한다.

## 자동 참고 규칙

- CAN signal 이름, bit layout, endian, scale, offset, enum, frame ID가 필요한 코드 작업에서는 `scripts/tcan_signal_detail.py`를 먼저 사용한다.
- 기본 필터는 ModelY + VEH/CH지만 bus/view 차이로 frame ID가 달라질 수 있으므로 필요한 bus를 명시하고 matching source를 기록한다.