# Nag Killer 안전 업그레이드 계획

## 목표

- 새 NVS, NVS 초기화, OTA 안전 초기화에서 Nag Killer를 OFF로 두고 프로파일은 `[기본]`으로 선택한다.
- `[기본]`을 포함한 모든 프로파일의 기존 타이밍과 패턴은 유지하면서 최종 송신 토크를 ±1.80 Nm로 제한한다.
- 최신 DAS 상태와 조향각이 모두 확인된 경우에만 주입하고, 실제 TWAI 송신 성공만 성공 카운터로 기록하며, 자체 echo 재수신은 다시 처리하지 않는다.

## 설계 비교

### 선택안. 기존 구조에 최종 안전 경계 추가

- `NagHandler` 송신 직전에 공통 토크 clamp와 자체 echo 방어를 둔다.
- 기존 `CanDriver::sendCheck()` 확장 지점을 TWAI 드라이버에서 구현해 실제 송신 결과를 받는다.
- 921/923과 297의 기존 수신 시각을 기준으로 최근 수신 후 1초 이내인지 확인한다.
- 변경 파일과 실차 영향이 작고, 모든 프로파일에 같은 안전 정책을 보장한다.

### 비선택안. 외부 프로젝트의 드라이버와 Nag 모드를 통째로 교체

- 전송 API와 상태기계를 전면 교체해야 하므로 현재 OTA, Web UI, BUS-OFF 복구, 프로파일 동작의 회귀 위험이 크다.
- 검증된 현재 구조를 유지하려는 요구와 맞지 않아 선택하지 않는다.

## 단계별 실행 계획

1. `test/test_native_nag/test_nag_handler.cpp`, `include/drivers/mock_driver.h`에 안전 요구 재현 테스트를 추가한다.
   - 검증 명령은 `pio test -e native_nag`이다.
   - 기존 코드에서 새 테스트가 실패하면 재현 성공이다.
2. `include/can_helpers.h`, `include/drivers/twai_driver.h`, `include/handlers.h`를 최소 수정한다.
   - 성공 조건은 기본값 OFF, 기본 프로파일 유지, stale 입력 차단, ±1.80 Nm clamp, 송신 성공 계수, 자체 echo 차단이다.
3. 관련 native 테스트와 HW3 환경을 빌드한다.
   - 검증 명령은 `pio test -e native_nag`, `pio test -e native`, `pio run -e lilygo_t2can`이다.
   - 모든 명령이 성공해야 한다.
4. 변경 결과와 OTA 후 확인 사항을 한국어 문서에 기록한다.

## 범위 제한

- 프로파일 A~D와 `[기본]`의 타이밍, burst/pause, 방향 유지 정책은 변경하지 않는다.
- CAN-A Summon/TSLLC 로직과 Web UI 디자인은 이번 변경 범위에 포함하지 않는다.
- 기존 NVS에 사용자가 저장한 Nag 프로파일은 일반 재부팅에서 보존한다. 새 NVS·NVS 초기화·OTA 안전 초기화만 `[기본]`으로 설정한다.
