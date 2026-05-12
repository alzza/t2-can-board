# Signal Observer Event Log Context Notes

Plan.

1. 기존 관찰기 요약 상태는 유지하고, 별도 256개 이벤트 링버퍼를 추가한다.
2. `signalObserverObserveFrame()`에서 모든 등록 신호에 대해 `FIRST_SEEN`, `RAW_CHANGE`, `ACTIVE_START`, `ACTIVE_END`를 기록한다.
3. 캡처 시작/정지, 카운터 리셋, 설정 업로드 시 이벤트 로그를 함께 정리한다.
4. `/api/signal-observer-log-dl` CSV 엔드포인트를 추가하고, UI의 관찰기 로그 저장 버튼만 이 엔드포인트로 바꾼다.
5. 통합 로그는 기존 요약을 유지하되 관찰기 이벤트 개수/용량/최근 이벤트를 제한된 링버퍼에서만 출력한다.

결정.

- 방향지시기 예시는 `SCCM_turnIndicatorStalkStatus`였지만, 구현은 신호 이름과 무관하게 등록된 모든 관찰기 신호에 동일하게 적용한다.
- 메모리 한도는 고정 `256 events`로 시작한다. 프레임 전체가 아니라 변화 이벤트만 저장해 RAM과 로그 크기를 제한한다.
- 다운로드 파일명은 펌웨어 wall-clock 동기화가 있으면 `CAN_SNIPPER_YYYYMMDD_HHMMSS.csv`, 없으면 uptime 기반으로 둔다.

구현 결과.

- `include/can_helpers.h`에 256개 `SignalObserverEvent` 링버퍼를 추가했다.
- `FIRST_SEEN`, `RAW_CHANGE`, `ACTIVE_START`, `ACTIVE_END`, `CAPTURE_START`, `CAPTURE_STOP`, `RESET`, `CONFIG_LOADED`를 기록한다.
- `/api/status`의 `signal_observer`에 `event_count`, `event_capacity`, `event_head`, `event_overwritten`를 추가했다.
- `/api/signal-observer-log-dl` CSV 다운로드를 추가했다.
- `/api/logs-bundle`에는 관찰기 요약 뒤에 bounded 이벤트 CSV 섹션을 추가했다.
- UI의 관찰기 저장 버튼은 통합 로그 대신 관찰기 전용 CSV를 받는다.

검증.

- VS Code diagnostics: 수정한 주요 파일 오류 없음.
- `pio run -e lilygo_t2can` 성공.
- 빌드 후 RAM: 105,592 / 327,680 bytes, 32.2%.
- 이전 기준 93,288 bytes 대비 +12,304 bytes. 256개 이벤트 링버퍼 추가분과 일치한다.