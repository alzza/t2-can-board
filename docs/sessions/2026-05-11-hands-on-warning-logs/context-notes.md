# DAS hands-on warning 로그 보강 컨텍스트 노트

## 확인한 신호 근거

- T-CAN script 결과 `DAS_autopilotHandsOnState`는 ModelY CH의 `0x399 DAS_status`에 있다.
- Decimal ID는 `921`이며, 현재 펌웨어는 `921`과 `923`을 DAS_status 후보로 모두 수신한다.
- Layout은 `startBit=42`, `length=4`, `byteOrder=little`, extract는 `(data[5] >> 2) & 0x0F`다.
- Enum은 `0 NOT_REQD`, `1 REQD_DETECTED`, `2 REQD_NOT_DETECTED`, `3 REQD_VISUAL`, `4 REQD_CHIME_1`, `5 REQD_CHIME_2`, `6 REQD_SLOWING`, `7 REQD_STRUCK_OUT`, `8 SUSPENDED`, `9 REQD_ESCALATED_CHIME_1`, `10 REQD_ESCALATED_CHIME_2`, `15 SNA`다.

## 구현 판단

- 동작 변경은 하지 않는다. 이번 작업은 log-only 진단 보강이다.
- 5초 시계열은 경고 상태와 조향/토크/decision을 같은 row에서 비교하는 용도로 쓴다.
- ms 이벤트 로그는 state transition timing을 보는 용도로 쓴다.
- 이벤트 구조체는 키우지 않고 기존 `detail`을 해석한 `detailText`만 출력한다.
- D안 판별은 기존 `smartProfile=4`와 새 warning label/level 조합으로 충분히 가능하게 한다.

## 구현 결과

- `dasStateName`, `dasStateGroup`, `dasWarnLevel`, `dasWarning`을 5초 시계열 CSV와 통합 로그 [4]에 추가했다.
- `/api/events.csv`와 통합 로그 [5]는 기존 numeric `detail` 옆에 `detailText`를 추가해 NAG_MODE, MODEB_STATE, MODEB_PHASE, FIRST_ECHO, USER_MARK를 바로 읽을 수 있게 했다.
- `/api/status`의 B채널 JSON과 `/api/nag-stats`에는 현재 DAS hands-on state name/group/warning level/warning flag를 추가했다.
- USER_MARK와 통합 로그 스냅샷의 B채널/B나그판정 줄에도 DAS hands-on 상태 이름과 warning level을 함께 남긴다.

## 검증

- VS Code diagnostics: 수정 파일 오류 없음.
- `pio test -e native_nag`: 41개 테스트 모두 통과.
- `pio run -e lilygo_t2can`: SUCCESS, RAM 92,144 bytes, Flash 927,185 bytes.