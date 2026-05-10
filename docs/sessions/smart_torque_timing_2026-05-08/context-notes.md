# Smart Torque 타이밍 로깅 보강 컨텍스트 노트

확인한 사실.
- 현재 통합 로그 [4] 시계열 섹션은 `DAS`, `Mode`, skip 사유, echo delta는 갖고 있지만 `AP`, `Phase`, `297`, 조향각, Mode B 전용 주입 카운터, 토크, 프레임 age, 상태 지속 시간을 담지 않는다.
- `/api/nag-stats`에는 이미 `dasApState`, `steerAngleDeg`, `frames297`, `modeBPhase`, `modeBInjects`, `modeBLastNm`이 있다.
- 통합 로그 스냅샷에는 `B나그판정` 한 줄로 age와 `AP/Phase`가 보이지만, 5초 시계열에는 빠져 있다.
- ESP-IDF v4.4.8 TWAI 문서 기준 TEC/REC, arbitration lost, bus error, tx failed, rx missed는 타이밍 변경의 안전성 판단에 계속 필요하다.

판단.
- 최적 타이밍은 `DAS state 변화 → Phase 변화 → 첫 echo → 사용자 마커`의 시간 관계로 판단해야 한다.
- 5초 시계열만으로는 300~500ms 단위의 최적값을 직접 계산하기 어렵다.
- 이벤트 로그에는 전이와 첫 echo만 남기고, 반복 echo는 누적 카운터와 delta로 본다.

추가할 핵심 필드.
- 시계열: `f297`, `apState`, `modeBPhase`, `steerDeg`, `realTorqueNm`, `modeBInject`, `modeBLastNm`, `age880Ms`, `ageDasMs`, `age297Ms`, `ageEchoMs`, `modeBStateAgeMs`, `modeBPhaseAgeMs`, `modeBFirstEchoDelayMs`, `d297`, `dModeBInject`.
- 이벤트: `MODEB_STATE`, `MODEB_PHASE`, `MODEB_FIRST_ECHO`.