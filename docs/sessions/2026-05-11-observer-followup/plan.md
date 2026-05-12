# Plan - Observer Follow-up

1. D안 적용 확인.
   - 검증: 펌웨어 `/api/nag-profile`가 4를 허용하는지 확인하고, mock도 같은 응답을 내게 한다.
2. USER_MARK 카운트 의미 정정.
   - 검증: START에서는 카운트 유지, END에서만 완료 구간 카운트가 1 증가한다.
3. 신호 관찰기 사용성 보강.
   - 검증: 관찰기 카드 안에서 표 위/아래 이동과 통합 로그 저장이 가능하고, 빠른 프레임은 초당 변화량으로 읽을 수 있다.
4. T-CAN 이름 기반 observer JSON 생성기 추가.
   - 검증: 신호 이름만 넣어 JSON을 만들고 기존 `/api/signal-observer/config` 스키마에 맞는다.
5. 빌드와 mock 검증.
   - 검증: `node --check`, Python syntax, `pio run -e lilygo_t2can`를 실행한다.
