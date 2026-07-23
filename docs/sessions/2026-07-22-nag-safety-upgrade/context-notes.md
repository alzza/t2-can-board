# Nag Killer 안전 업그레이드 컨텍스트 노트

## 확정 사항

- 차량은 HW3이다.
- CAN-B는 Nag Killer 전용이다.
- 새 NVS, NVS 초기화, OTA 안전 초기화의 프로파일은 `[기본]`이다.
- 새 NVS의 Nag Killer 런타임 기본값은 OFF다.
- 기존 `[기본]` 프로파일의 500/700/400ms 타이밍과 mild 0.50~1.50 Nm 설정은 유지한다.
- 안전 보완은 프로파일 선택과 무관하게 모든 프로파일에 공통 적용한다.

## 코드 근거

- `nagCfgDefaultsSmart()`와 OTA 안전 설정은 이미 `kNagSmartProfileDefault`를 저장한다.
- `kNagKillerDefaultEnabled`는 NAG_KILLER 빌드에서 현재 true라 새 NVS 기본 ON이 된다.
- `_mbSendEcho()`는 현재 실제 드라이버 전송 결과를 확인하지 않고 성공 카운터를 올린다.
- 921/923과 297 수신 시각은 기록하지만 880 처리에서 최근 수신 유효시간을 검사하지 않는다.
- 강한 상태는 raw 1838~2258까지 만들 수 있어 공통 ±1.80 Nm 상한을 넘는다.

## 구현 결정

- 기존 `CanDriver::sendCheck()`를 유지하고 `TWAIDriver`만 실제 결과를 반환하도록 구현한다.
- 최근 수신 유효시간 기준은 외부 검증 프로젝트와 같은 1000ms로 고정한다.
- 별도 프로파일이나 사용자 설정을 추가하지 않고 송신 경계에서 공통 안전 정책을 적용한다.
- 자체 echo는 성공적으로 보낸 마지막 프레임과 ID, DLC, 전체 8바이트가 같은 경우에만 차단한다.

## 구현 결과

- NAG_KILLER 빌드의 `kNagKillerDefaultEnabled`를 false로 변경했다.
- `TWAIDriver::sendCheck()`가 `twai_transmit()`의 송신 요청 결과를 반환하고 실패 시 기존 suppressed 카운터를 증가시킨다. 버스 ACK 결과는 기존 TEC/REC·TX Fail·BUS-OFF 진단으로 별도 판단한다.
- `NagHandler`가 유효한 921/923과 297을 각각 관측했는지 확인하고 마지막 수신 age가 1000ms를 넘으면 차단한다.
- 모든 프로파일의 최종 송신 경계에서 raw 토크를 `0x74E~0x8B6`로 제한한다.
- 성공 송신 뒤에만 echo 카운터와 마지막 echo를 기록한다.
- 토크 진단값을 실제 DBC 변환식 `raw * 0.01 - 20.5`로 통일했다.

## 검증 결과

- 수정 전 새 안전 테스트 6건이 모두 실패해 문제를 재현했다.
- `pio test -e native_nag`에서 49/49 통과했다.
- `pio test -e native`에서 91/91 통과했다.
- `pio run -e lilygo_t2can`이 성공했다.
- HW3 빌드 결과는 RAM 106808바이트(32.6%), Flash 982125바이트(50.0%)다.
