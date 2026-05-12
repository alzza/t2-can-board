# Context Notes - Signal Observer Capture Controls

확인:
- firmware observer core에는 `signalObserverRuntime`이 이미 있어 수신 관찰 on/off가 가능하다.
- 기존 reset API는 카운터만 초기화하고 runtime 상태를 바꾸지 않는다.
- Web UI에는 `맨 위`, `맨 아래` 스크롤 버튼이 있으며, 사용자는 이를 `시작`, `정지` 캡처 버튼으로 바꾸길 원한다.

결정:
- `시작`은 새 실험 구간 의미가 되도록 카운터를 reset한 뒤 runtime ON으로 둔다.
- `정지`는 카운터를 그대로 보존하고 runtime OFF로 둔다.
- 누적 카운터는 firmware에서 그대로 유지하고, 정지 중에는 더 이상 증가하지 않게 한다.
- A/B 필터와 JSON 설정 스키마는 변경하지 않는다.
