---
sidebar_position: 5
---

# HW3 조건부 Summon Unlock

LILYGO T2-CAN HW3 빌드는 Web UI에서 조건부 Summon Unlock을 제어합니다. 이 기능은 검증된 `summon_unlock.ino`의 프레임 판정 방식을 이식했으며, Summon 명령을 새로 생성하거나 상시 송신하지 않습니다.

차량이 CAN-A에 보낸 ID `0x3FD` mux 1 프레임을 관찰하고, 모든 조건이 충족될 때만 수정해 재송신합니다.

## 실제 전송 조건

| 조건 | 내용 |
|---|---|
| 수신 프레임 | ID `0x3FD`, mux 1, DLC 8바이트 |
| 기능 설정 | Web UI의 Conditional Summon Unlock이 ON |
| 송신 허용 | CAN-A TX 마스터 ON, TX Guard 비활성 |
| 상태 게이트 | `Parked || Summoning` |

조건이 하나라도 충족되지 않으면 수정 프레임은 전송하지 않습니다. 기능은 ON이지만 게이트가 닫혀 있으면 `Blocked` 카운터만 증가합니다.

## 게이트 판정

- **Parked**는 CAN 280의 `DI_gear`를 우선 사용합니다. CAN 390의 `DIF_gear`는 CAN 280이 처음부터 없거나 5초 이상 없을 때만 보조로 사용합니다.
- **Summoning**은 CAN 280의 ACA `DI_autonomyControlActive`와 CAN 1016의 SPR `UI_selfParkRequest`가 모두 관측된 상태입니다.
- ACA가 해제되면 SPR 래치를 지워 Summoning 게이트를 닫습니다.
- CAN 280이 5초 이상 수신되지 않으면 INO와 동일하게 Parked로 간주합니다.
- CAN 921 AP 상태는 대시보드 진단용일 뿐 게이트 조건이 아닙니다.

차량 대기 후 호출 지연은 Web UI의 `재수신→첫 TX`에서 확인할 수 있습니다. 이 값은 2초 이상 A채널 수신이 끊겼다가 다시 시작된 시점부터, 검증된 게이트가 열리고 첫 Summon 수정 프레임 송신이 성공한 시점까지입니다. 차량이 CAN 전송을 재개하기 전의 시간은 보드에서 측정할 수 없습니다.

간헐적인 A채널 경고가 보이면 진단 화면에서 현재 EFLG 원인과 RX-OVR, TEC/REC를 확인하고 자가 진단·시계열·밀리초 이벤트 CSV를 저장하십시오. 경고를 숨기거나 Summon 게이트를 완화하지 않습니다.

## EU Unlock과 HW3 Summon 비트

EU Unlock은 독립 기능이 아닙니다. Summon 수정 프레임이 전송될 때 함께 적용되는 bit 19 변경을 뜻합니다.

| CAN ID | mux | 비트 | 값 | 의미 |
|---|---|---|---|---|
| `0x3FD` | 1 | 19 | 0 | `UI_applyEceR79` 해제. EU Unlock 동작 |
| `0x3FD` | 1 | 46 | 1 | HW3 Summon Unlock 비트 |

기본 빌드는 HW3이므로 bit 46을 사용합니다. INO의 HW4 경로는 bit 47을 사용하지만 이 T2-CAN HW3 빌드의 동작이 아닙니다.

## 사용 순서

1. 차량을 정차하고 `TeslaCAN` Wi-Fi에 연결한 뒤 `http://192.168.4.1`을 엽니다.
2. CAN-A RX, TEC, REC, BUS-OFF를 확인합니다.
3. **Controls**에서 **Conditional Summon Unlock (HW3)**을 ON으로 설정합니다.
4. CAN-A 카드에서 **A TX master**를 ON으로 설정합니다.
5. Summon 카드의 `Enabled`, `Active`, `Gate OPEN`, `TX OK / Fail`, 280/390/921/1016 수신 카운터를 확인합니다.

OTA 후 첫 부팅은 모든 차량 영향 기능과 A TX가 OFF/stock 상태입니다. 수신 상태를 확인하고 60초 OTA 확인을 완료한 뒤 기능을 하나씩 켜십시오.

## 즉시 정지

- Conditional Summon Unlock을 OFF하면 다음 mux 1 프레임부터 수정 송신을 중단합니다.
- A TX master를 OFF하면 Summon과 TSLLC의 CAN-A 수정 송신을 함께 중단합니다.
- TX Fail, TEC 상승 또는 BUS-OFF가 보이면 A TX master를 먼저 OFF하고 원인을 확인합니다.

:::caution
Tesla 차량 펌웨어와 CAN 동작은 예고 없이 바뀔 수 있습니다. 개인 실험용 정차 상태에서만 사용하고, 차량 OTA마다 CAN 수신과 BUS-OFF 상태를 다시 확인하십시오. 이 펌웨어는 Tesla 서비스 권한이나 구독 상태를 변경하지 않습니다.
:::
