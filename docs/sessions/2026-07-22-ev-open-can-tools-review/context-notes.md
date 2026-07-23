# ev-open-can-tools 비교 분석 컨텍스트 노트

## 입력

- 분석 대상은 사용자가 제공한 `ev-open-can-tools-dev (2).zip`이다.
- 임시 분석 경로는 `/private/tmp/ev-open-can-tools.BkBKrn/ev-open-can-tools-dev`다.
- 외부 프로젝트는 ESP-IDF 기반이며 현재 프로젝트는 Arduino/PlatformIO 기반이다. 따라서 구성 전체 이식이 아니라 안전 정책·알고리즘·테스트 단위 비교를 기본으로 한다.

## 현재 전제

- 차량은 HW3이다.
- CAN-A는 Summon Unlock/TSLLC, CAN-B는 Nag Killer 전용이다.
- Summon 포팅은 검증된 INO의 `Parked || Summoning` 게이트와 HW3 bit 46을 유지해야 한다.
- 차량 오류 방지를 위해 CAN 안정화와 A채널 진단이 우선이다.

## 비교 결과

### 즉시 보완 후보

- 현재 Nag Killer의 강한 상태는 raw 1838~2258, 즉 ±2.10 Nm까지 생성한다. 그러나 공통 상한은 `kNagTorqueRawMin/Max` 0x74E~0x8B6, 즉 ±1.80 Nm다. `_mbSendEcho()` 직전의 최종 하드 clamp와 강한 상태 상한 통일이 필요하다.
- 현재 B채널 `CanDriver::send()`은 void라 `_mbSendEcho()`가 실제 TWAI 전송 결과와 무관하게 echo·framesSent·주입 카운터를 증가시킨다. 외부 도구처럼 bool 전송 결과를 반환해 성공·실패·억제를 분리할 수 있다.
- 현재 Nag은 921/923·297의 마지막 수신 시각을 기록하지만 `_handleModeB()`에서 최근 수신 유효시간 차단에 사용하지 않는다. 880은 계속 오고 DAS 상태가 끊긴 경우 과거 AP·hands-on 값으로 주입을 이어갈 수 있다.
- 외부 Nag은 마지막 자체 echo 전체 비교를 사용한다. 현재는 real hands-on 값만 보고 재진입을 막으므로 동일 프레임 재수신 방어를 추가 검토할 수 있다.

### 현재 구현이 더 강한 부분

- 현재 A채널은 EFLG, TEC/REC, MERRF, RX overflow, TX Guard, BUS-OFF 재초기화와 재시작 fallback을 이미 제공한다. 외부 MCP2515의 5회 TX 실패 복구를 그대로 이식할 필요는 없다.
- 현재 B채널은 TWAI soft recovery, cooldown, hard reinstall fallback, TEC/REC·중재 손실·버스 오류·큐 누락, 이벤트·시계열·로그 번들을 제공한다.
- 현재 Nag은 프로파일 A~D, 실시간 DAS 상태 전이, 조향 방향, burst/pause, 상태별 진단을 이미 갖춘다. 외부 Mode A/B/C 전체 포팅은 기능 퇴행 또는 중복이다.
- 현재 OTA/NVS는 차량 기능 안전값 재기록과 CAN-disabled fail-closed 복구 UI를 갖춘다.

### 직접 이식하지 않을 항목

- 외부 범용 JSON 플러그인 엔진은 임의 CAN 송신 표면을 넓힌다. 사용자의 실험 기능 제거 방침과 맞지 않는다.
- 외부 Summon-only 정책은 500ms 최근 수신 유효시간, 속도 ID 599, AP 비활성, 기어 일치까지 요구한다. 검증된 INO의 `Parked || Summoning`과 280 5초 무수신 시 Parked fallback을 바꾸므로 직접 교체하면 현재 차량의 동작을 깨뜨릴 수 있다.
- ESP-IDF 전체 이식은 Arduino/PlatformIO 기반 현재 프로젝트의 OTA·Web UI·드라이버 구조를 대규모로 교체하므로 개선 후보가 아니다.

### 단계 제안

1. Nag 안전성 hotfix. 최종 ±1.80 Nm clamp, 송신 결과 계수, DAS/조향 최근 수신 유효시간 차단, 자체 echo 방어를 테스트부터 추가한다.
2. Nag 관측성. UI와 로그에 `READY/BLOCKED` 사유, 각 입력 프레임 age, send attempt/success/suppressed를 분리한다.
3. 실차 확인 뒤에만 프로파일 타이밍·토크·기본 ON/OFF 정책을 조정한다.

## 검증 범위

- 이번 단계는 외부 ZIP과 현재 소스를 읽어 비교한 분석만 수행했다. 펌웨어 코드·Web UI·빌드 설정은 수정하지 않았다.
