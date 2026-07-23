---
title: HW3 Nag Killer
sidebar_position: 3
---

# HW3 Nag Killer

현재 LILYGO T2-CAN HW3 빌드의 Nag Killer는 CAN-B에서 EPAS 상태 프레임 ID 880을 관찰하고, Web UI에서 선택한 MODE 1/2/3 규칙에 따라 수정 echo를 전송합니다.

:::caution
이 기능은 운전자 감시를 대신하지 않습니다. 항상 핸들을 잡고 도로에 집중하십시오. 차량 OTA 뒤에는 정차 상태에서 CAN 수신과 BUS-OFF를 먼저 확인해야 합니다.
:::

## 초기 상태와 모드

- 새 NVS와 NVS 초기화에서는 Nag Killer가 OFF로 시작합니다.
- OTA 안전 초기화에서도 Nag를 OFF로 저장합니다.
- 새 NVS, NVS 초기화, OTA 안전 초기화의 선택 모드는 MODE 3(기본)입니다.
- 일반 재부팅에서는 사용자가 저장한 MODE 1/2/3 선택을 유지합니다.
- 이전 `[기본]/A~D` 실험 프로파일과 NVS 값은 폐기되었습니다.

## 활성화 전 확인

1. 차량을 정차하고 Web UI에 접속합니다.
2. CAN-B ID 880, 921/923, 297 수신 상태를 확인합니다.
3. B채널 `BUS-OFF=0`이고 TEC/REC, TX Fail이 증가하지 않는지 확인합니다.
4. Nag Killer를 ON으로 설정합니다.
5. echo 수와 마지막 판정, DAS/AP 상태, 조향각 age를 함께 확인합니다.

## 모드별 실제 동작

| 모드 | 토크/타이밍 | 조건 |
|---|---|---|
| MODE 1 | +1.80 Nm 고정 | ID 880 DLC 8, 실제 `handsOn=0` |
| MODE 2 | `+1.80, +1.50, -1.50, -1.80 Nm` 200ms 순환, 1초 burst + 1.5초 pause | ID 880 DLC 8, 실제 `handsOn=0` |
| MODE 3(기본) | state 2: 2초 후 0.50~1.80 Nm random walk, state 3: 1초 후 ±1.80 Nm 삼각파 | 921/923·297 age ≤1초, AP 3~6, validity=1, 조향각 ±5°, DAS state 2/3 |

조건이 맞으면 원본의 관련 상위 비트를 보존하면서 토크와 hands-on 필드를 적용하고, counter를 1 증가시킨 뒤 checksum을 다시 계산합니다. MODE 1/2는 검증 원본과 같이 DAS/조향의 최근 수신 유효시간을 확인하지 않고, MODE 3만 과거 컨텍스트 재사용을 차단합니다.

## 공통 안전 경계

- 최종 토크는 모드와 무관하게 raw `0x74E~0x8B6`, 즉 -1.80~+1.80 Nm로 제한됩니다.
- `twai_transmit()`이 송신 요청을 성공으로 반환한 경우에만 echo·주입 카운터와 마지막 송신 시각이 갱신됩니다. 버스 ACK와 오류 상태는 별도의 TEC/REC, TX Fail, BUS-OFF 진단으로 확인해야 합니다.
- 성공적으로 보낸 마지막 echo와 ID, DLC, 전체 데이터가 같은 수신 프레임은 다시 처리하지 않습니다.
- MODE 1/2는 실제 `handsOnLevel=0`일 때만 송신합니다. MODE 3은 검증 원본과 같이 0 또는 1을 허용합니다.

## 채널 구분

Nag Killer는 CAN-B 전용입니다. CAN-A의 조건부 Summon Unlock, EU Unlock, TSLLC와는 별도 경로이며 Nag 모드 변경이 CAN-A 기능을 바꾸지 않습니다.
