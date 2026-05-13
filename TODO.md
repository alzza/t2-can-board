해야할일..


- [x] 관찰기 로그는 따로 저장.
- [x] 통합로그는 시계열 20분/이벤트로그 포함하여 기록하기.
- [x] 관찰기 로그기록은 최초 정지 상태에서 시작해서 사용자가 시작버튼을 누르면 그때 작함. 
- [x] nag killer D모드 수정하기. 
      -> 원인분석을 위해 기존 채팅로그 찾아서 분석기 만들어보기.
     주입해야할 타이밍에 주입을 했는데도 인식못하는 현상?인듯함.
     -> 2026-05-13: realHo 단발 pulse 뒤 sign hold 유지 fix (handlers.h), native_nag 43/43 통과. 실차 검증 미완료.

- [x] 관찰기는 A채널만 분석하기. 
      -> tcan_observer.json channel "A+B" → "A"로 변경. B채널 프레임 필터링은 signalObserverObserveFrame의 channelMask 체크가 처리함.
- [x] docs/tcan_observer.json 기본값으로 A채널 단일 신호로 바꾸기. 
      -> 3개 신호 → SCCM_turnIndicatorStalkStatus(frame 585, A-only) 단일 신호로 축소. "A+B"는 서버가 400 반환하므로 실질적으로 동작 불가였음.
- [x] 관찰기 mux mismatch 프레임 무시하도록 수정하기.
      -> 근본 원인: frame 585/659/1016이 CH bus(B채널)에도 동일 ID로 존재 → "A+B"로 양쪽 버스 데이터 수신 → 값 차이로 changeCount++ 반복. channel="A"로 B채널 프레임 필터링하면 해결. 펌웨어 mux 처리 로직(muxLength>0 체크)은 이미 올바름.


