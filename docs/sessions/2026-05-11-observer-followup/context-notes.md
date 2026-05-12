# Context Notes - Observer Follow-up

확인:
- 펌웨어 `/api/nag-profile?p=4` 경로는 `nagSmartProfileClamp()`와 `nagSmartProfileSettings()`를 사용하므로 D안을 허용한다.
- mock 서버는 D안 profile이 없고 clamp가 0..3이라 로컬 UI에서 D안이 적용되지 않는다.
- 기존 USER_MARK count는 버튼 이벤트 수다. 이번 목표는 START→END 완료 구간 수로 바꾸는 것이다.
- 신호 관찰기는 누적 카운터를 정확히 보존하되, 빠른 프레임은 UI에서 delta/s로 읽는 쪽이 진단 의미를 덜 훼손한다.
- A/B 채널 선택은 JSON의 `channel` 필드로 이미 가능하다. A채널은 MCP2515 필터 슬롯 제한 때문에 기존 659/1016/1021 포함 6개 ID 제한을 유지한다.

결정:
- observer firmware core는 유지한다. 카운터를 느리게 만들거나 샘플링으로 누락시키지 않는다.
- JSON 생성기는 PC-side Python 스크립트로 추가한다. 펌웨어에 T-CAN metadata를 싣지 않는다.
- 관찰기 UI는 카드 안에서 필요한 버튼과 delta 열만 추가하고, 동적 편집 UI는 이번 범위에 넣지 않는다.
