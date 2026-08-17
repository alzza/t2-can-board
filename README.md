# Tesla Open CAN Mod

LILYGO T2-CAN ESP32-S3에서 동작하는 Tesla HW3용 듀얼 CAN 펌웨어입니다.

- CAN-A: EAP/ECE R79·Summon Unlock, TSLLC, 수신 진단
- CAN-B: Nag Killer
- Wi-Fi: Web UI, OTA, 로그 다운로드

## 주의

- 차량 CAN 통신은 안전에 영향을 줄 수 있습니다.
- 차량 OTA 전에는 송신 기능을 끄고, 수신 상태와 BUS-OFF 여부를 먼저 확인하십시오.
- 이 프로젝트는 개인 실험 및 교육용입니다.

## 기본 사용

1. `pio run -e lilygo_t2can`
2. `pio run -e lilygo_t2can -t upload`
3. `TeslaCAN` AP에 연결
4. `http://192.168.4.1` 접속

## 보드 역할

| 채널 | 역할 |
|---|---|
| CAN-A | EAP/ECE R79·Summon Unlock, TSLLC |
| CAN-B | Nag Killer |
| Wi-Fi | 상태 확인, 제어, OTA |

## HW3 기능 구분

| Web UI 토글 | CAN-A 동작 |
|---|---|
| EAP / EU Unlock / Summon | ID `0x3FD` mux 1에서 ECE R79 bit19 해제와 HW3 Summon bit46 활성화를 함께 적용 |
| TSLLC | ID `0x3FD` mux 0에서 bit38/39 적용 |

- 실제 Summoning은 `ACA + 확인된 SPR`로 판단합니다.
- Summoning 중에는 같은 ID의 송신 경쟁을 줄이기 위해 TSLLC를 잠시 보류합니다.
- TSLLC는 A CAN 시작 15초·유효 프레임 1,001개 이후, AP 상태 3~6의 최근 신호가 1초 이상 안정된 경우에만 동작합니다. 원본 bit38/39가 이미 1이면 송신하지 않습니다.
- 비정상 재부팅 후 TSLLC 설정은 보존되지만, Web UI에서 한 번 재승인해야 다시 송신합니다.
- A RX overrun은 첫 발생 시 자동 복구 확인 동안 A TX를 보류하고, 60초 내 재발하면 해당 부팅의 A TX를 잠급니다.
- API와 NVS는 기존 `/api/summon-unlock`, `/api/tsllc`, `summon_unlock`, `tsllc`를 그대로 사용합니다.

## 기록 규칙

- 펌웨어 산출물 이름은 `버전_YY-MM-DD_짧은변경요약.bin` 형식을 사용합니다.
- README, CHANGELOG, 공개 문서는 한글로 작성합니다.
- 세션용 기록은 저장소에 올리지 않습니다.
