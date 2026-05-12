# USER_MARK UI와 START/END 토글 계획

목표:
- 메인 헤더의 `전체 저장` 옆에 USER_MARK 버튼과 로그당 기록 횟수를 표시한다.
- 기존 `/api/user-marker?type=ap_warning` 동작을 클릭마다 단일 마커만 찍는 방식에서 `AP_WARNING_START` / `AP_WARNING_END` 토글로 확장한다.
- 통합 로그, 이벤트 CSV, status JSON에서 현재 USER_MARK active 상태와 로그당 카운트를 확인할 수 있게 한다.

검증:
- VS Code diagnostics 오류 없음.
- `pio test -e native_nag` 통과.
- `pio run -e lilygo_t2can` 통과.
