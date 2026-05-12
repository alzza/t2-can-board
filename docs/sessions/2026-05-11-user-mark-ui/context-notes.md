# USER_MARK UI 컨텍스트 노트

## 기존 동작 확인

- 기존 `/api/user-marker?type=ap_warning`는 `detail=1` 단일 마커만 찍었다.
- 기존 버튼은 메인 헤더가 아니라 CAN 자가 진단 패널 안에 있었다.
- 기존 통합 로그에는 `사용자마커: count=... detail=1`과 이벤트 `USER_MARK` row가 남았지만, START/END 구간 의미는 없었다.

## 변경 결정

- `type=ap_warning`은 하위 호환을 유지하되 내부적으로 active 상태를 토글한다.
- 첫 클릭은 `AP_WARNING_START`, 다음 클릭은 `AP_WARNING_END`로 기록한다.
- 명시 호출용으로 `type=ap_warning_start`, `type=ap_warning_end`, `type=start`, `type=end`도 허용한다.
- 로그 초기화나 새 기록 시작 시 active 상태는 false로 되돌려 다음 USER_MARK가 START가 되게 한다.
- `user_marker_log_count`는 전체 누적 카운트가 아니라 현재 로그 기준 카운트다.

## 로그 해석

- 통합 로그 스냅샷 `사용자마커` 줄은 `count`, `log_count`, `active`, `detail`, `detailText`를 보여준다.
- 이벤트 CSV에서는 `typeName=USER_MARK`, `detail=1/2`, `detailText=marker=AP_WARNING_START/END`로 보인다.
- 5초 시계열에서는 기존처럼 `userMark`와 `dUserMark`가 마커 클릭 횟수로 증가한다.