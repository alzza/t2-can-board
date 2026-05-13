# Context Notes

- 사용자 보고: 20분 시계열 저장은 한 번 되었고, 이후 전체 저장 클릭 시 다운로드 중 보드가 재부팅.
- 기존 대응(5/13 05:44): 다운로드 전 polling 5초 pause였으나 현재 코드에는 30초 고정 재개 타이머가 있음.
- 현재 가설: 대용량 logs-bundle 전송이 30초를 넘기면 polling/status/nag가 중간 재개되어 HTTP 처리 경합이 커지고 재부팅으로 이어짐.
- 보강 방향: JS pause 연장 + 서버측 status/nag 방어 가드 이중 적용.
