# T-CAN 신호 상세 추출 도구 계획

## 목표

- T-CAN Explorer의 공개 JSON 데이터에서 특정 CAN signal 묶음의 상세 정보를 반복 추출할 수 있게 한다.
- 이번 요청의 대상 신호는 `DAS_autopilotState`, `DAS_autopilotHandsOnState`, `EPAS3P_torsionBarTorque`, `EPAS3P_handsOnLevel`, `SCCM_steeringAngle`다.
- 결과는 기본 Markdown, 시각 확인용 HTML, 재가공 가능한 JSON을 지원한다.

## 접근

1. `https://tcan.latency.is/data/manifest.json`에서 signal이 들어 있는 frame을 찾는다.
2. 각 frame key로 `https://tcan.latency.is/data/frames/<sanitized-key>.json`을 내려받는다.
3. platform과 bus filter를 적용해 사용자가 보는 T-CAN URL 조건과 맞춘다.
4. frame metadata, signal layout, enum map, VAPI alias, source presence, DBC-style `SG_` 줄을 출력한다.
5. `--format html`은 같은 데이터를 standalone HTML로 렌더링하고, byte/bit layout은 색상 있는 표로 보여준다.

## 검증

- 요청한 5개 signal을 대상으로 Markdown 출력이 생성되는지 확인한다.
- JSON 출력도 parse 가능한지 확인한다.
- HTML 출력 파일에 signal card와 bit map table이 생성되는지 확인한다.
- `git diff --check`로 문서와 스크립트 공백 오류를 확인한다.

## 추가 보강

- Markdown 출력에 byte/bit별 `raw[n]` 격자를 추가해 비트 레이아웃을 바로 볼 수 있게 한다.
- CAN signal 구현이나 검토 때 이 도구를 자동 참고하도록 `.github/copilot-instructions.md`에 사용 규칙과 예시를 남긴다.
- 비트 레이아웃을 눈으로 비교할 때는 `--format html` 출력도 함께 생성할 수 있게 한다.