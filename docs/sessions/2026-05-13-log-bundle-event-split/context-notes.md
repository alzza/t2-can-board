# Context Notes

- 2026-05-13 전체 저장 문제는 iPhone Safari 선택 UI 자체보다 `/api/logs-bundle` 응답 크기와 긴 chunk 스트리밍 부담 쪽으로 재분석했다.
- 20분 시계열은 2026-05-11에 들어갔고 `samples=240` 로그가 정상 저장·분석된 기록이 있다.
- 2026-05-12 관찰기 이벤트 링버퍼 추가 시 RAM이 약 12.3 KiB 증가했고, 통합 로그에도 관찰기 이벤트 CSV 섹션이 추가됐다.
- `/api/events.csv`와 `/api/signal-observer-log-dl`는 이미 등록된 개별 파일 endpoint다.
- 사용성상 두 이벤트 파일을 따로 누르는 것보다 `/api/events-bundle` 하나로 묶는 편이 낫다.
- 이번 변경은 전체 저장에서 이벤트 row를 제거하고, 이벤트 묶음 파일을 관찰기 카드에서 별도 저장하게 하는 외과적 변경으로 제한한다.
