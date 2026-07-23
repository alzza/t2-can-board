# 컨텍스트 노트

## 차량 조건

- 차량은 HW3이다.
- A채널은 Summon Unlock/TSLLC 전용, B채널은 Nag Killer 전용이다.
- Summon Unlock은 검증된 INO의 `Parked || Summoning` 게이트를 유지한다.

## 확인된 문제

- 사용자가 차량에서 본 표시는 `A WARN`으로 확인했다.
- A채널 UI의 `WARN`은 모든 MCP2515 EFLG 비트가 하나로 합쳐져 원인을 알 수 없다.
- RX0OVR/RX1OVR는 드라이버가 즉시 클리어하지만 UI에는 다음 폴링까지 일반 `WARN`으로 표시된다.
- A채널 `driver_ok`는 실제 MCP2515 초기화 결과가 아니라 핸들러 포인터 존재 여부를 사용한다.
- 메인 UI 갱신 코드에 이미 삭제된 DOM ID 참조가 남아 있다.
- 자가 진단은 B채널 중심이며 A채널 초기화·프레임·EFLG·TEC/REC를 종합 검사하지 않는다.
- 자가 진단의 DAS 검사는 921만 사용해 HW3에서 허용한 923 경로를 반영하지 못한다.
- `timeseries.csv`는 B채널 중심이라 A채널 간헐 경고의 원인을 사후 분석할 수 없다.
- 숨겨진 Single Shot/BUS-OFF stop 실험 UI와 동작하지 않는 호환 API가 남아 있다.

## 적용 결과

- A채널 상태는 `OK`, `NO_FRAMES`, `RX_OVERRUN`, `ERROR_WARNING`, `ERROR_PASSIVE`, `BUS_OFF`, 초기화/루프 오류로 구분한다.
- 현재 EFLG 원시값과 각 비트, 발생·해제 이벤트, TEC/REC를 API·Web UI·CSV에 남긴다.
- RX 오버런 비트는 드라이버에서 클리어하되 당시 원시값은 이벤트 CSV와 시계열에 보존한다.
- A/B 상태 판정은 서버의 `health_state`, `health_reason`, `health_level`을 단일 기준으로 사용한다.
- 자가 진단은 A/B 드라이버·태스크·트래픽·TEC/REC·BUS-OFF와 DAS 921/923을 함께 검사한다.
- 차량 CAN 재수신 시작부터 첫 Summon TX 성공까지의 시간을 계측한다. 검증된 게이트를 열기 전 대기시간도 포함된다.
- Single Shot/BUS-OFF stop 호환 플래그와 삭제된 UI DOM 참조를 제거했다.

## 검증 결과

- `pio test -e native`: 93개 성공.
- `pio test -e native_nag`: 23개 성공.
- `pio run -e lilygo_t2can`: HW3 실제 펌웨어 빌드 성공.
- Web UI 원본/임베디드 헤더 동기화, printf 형식 검사, `git diff --check` 통과.
- 인앱 브라우저 연결이 제공되지 않아 실제 렌더링 시각 검수는 수행하지 못했다. HTML 구조·DOM 연결·회귀 테스트로 대체 검증했다.

## 변경하지 않는 항목

- 실제 차량 로그 없이 A채널 경고 임계값을 완화하거나 경고를 숨기지 않는다.
- 검증된 Summon 게이트·HW3 bit46·1021 mux1 조건은 변경하지 않는다.
- A TX Guard 15초 동작과 TEC 기준은 이번 계측 결과를 얻기 전까지 유지한다.

## 차량 재검증 절차

1. 차량 정차 상태에서 Web UI의 **진단** 화면을 연다.
2. `A WARN`이 보이면 즉시 `USER_MARK`를 누른다.
3. A 현재/피크 EFLG, RX-OVR, MERRF, TEC/REC, 프레임/루프 경과시간을 확인한다.
4. **자가 진단 CSV**, **A/B 시계열 CSV**, **전체 로그 저장**을 순서대로 저장한다.
5. 차량 대기 후 Summon 호출 시 **재수신→첫 TX** 값을 함께 기록한다.
6. 이 자료가 확보되기 전에는 A TX Guard 임계값·15초 유지시간·Summon 게이트를 변경하지 않는다.
# OTA 송신 안전 보완

- OTA 전 TX OFF는 계속 유효한 보호 절차다. 차량 CAN 오류의 직접 치료가 아니라 플래시 기록·재부팅 경계에서 수정 프레임이 남는 위험을 줄인다.
- OTA 핸들러는 새 A/B 수정 송신을 차단하고 이미 시작된 `sendCheck()` 호출이 끝날 때까지 최대 250ms 기다린 뒤 B TX 큐를 비운다.
- MCP2515 하드웨어 TX 버퍼의 물리 전송 완료를 이 소프트웨어 확인만으로 증명할 수는 없다. 따라서 정차 상태에서 업로드하고, 첫 부팅은 기능 OFF 상태로 수신·오류만 확인하는 절차를 유지한다.
