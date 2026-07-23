# Nag Mode 1/2/3 정밀 포팅 계획

## 목표

- ev-open-can-tools의 Nag Mode 1/2/3 선택과 각 상태기계를 HW3 CAN-B ID 880 경로에 포팅한다.
- 현재 프로젝트의 NVS, OTA 안전 초기화, Web API, Web UI, TWAI 진단과 결합한다.
- 원본과 달라지는 안전 정책은 사용자 확인 없이 적용하지 않는다.

## 확인된 원본 동작

- Mode 1은 ID 880의 실제 hands-on이 0일 때 고정 +1.80 Nm, hands-on 1, counter +1, checksum 재계산 echo를 시도한다.
- Mode 2는 +1.80, +1.50, -1.50, -1.80 Nm를 200ms 단계로 순환한다. 1초 burst 뒤 1.5초 pause를 반복한다.
- Mode 3은 921과 297이 각각 1000ms 이내이고 AP state 3~6, 조향 validity 1, 조향각 -5~+5도일 때만 동작한다.
- Mode 3의 DAS hands-on state 2는 진입 2000ms 뒤 0.50~1.80 Nm random walk를 조향 반대 방향으로 적용한다.
- Mode 3의 DAS hands-on state 3은 진입 1000ms 뒤 -1.80~+1.80 Nm 삼각파를 1000ms 주기로 적용한다.
- 원본 Mode 1/2에는 921·297 최근 수신 유효시간 게이트가 없다.

## 현재 프로젝트와 충돌하는 부분

- 현재 `nag_mode`는 스마트 모드 하나로 강제되고 UI는 `[기본]/A~D` 프로파일을 선택한다.
- 현재 `[기본]/A~D`는 원본 Mode 3과 타이밍·상태·토크 패턴이 다르다.
- 직전 안전 보완은 921·297의 최근 수신 후 1000ms 이내 조건을 현재 모든 프로파일에 공통 적용했다.
- Mode 1/2를 원본 그대로 포팅하면 이 최근 수신 유효시간 조건 없이 ID 880 echo를 수행한다.

## 확정된 구현 방침

1. 기존 `[기본]/A~D` 실험 프로파일을 폐기하고 검증된 MODE 1/2/3만 사용한다.
2. MODE 1/2는 원본과 같이 921·297 최근 수신 유효시간 게이트 없이 동작한다.
3. MODE 3은 원본 조건을 적용하되, 현 차량 호환을 위해 DAS 소스 921/923을 모두 허용한다.
4. 새 NVS·NVS 초기화·OTA 안전 초기화는 Nag OFF + MODE 3을 저장한다.

## 검증 기준

- 원본 Mode 1 고정 echo, Mode 2 네 토크 순환·burst/pause, Mode 3 최근 수신 유효시간·validity·state 2/3 테스트가 통과해야 한다.
- 잘못된 mode NVS/API 값은 안전 기본값으로 clamp되어야 한다.
- Web UI와 API가 동일한 선택 모드와 설명을 표시해야 한다.
- `pio test -e native_nag`, `pio test -e native`, `pio run -e lilygo_t2can`, UI 동기화 검사가 통과해야 한다.
