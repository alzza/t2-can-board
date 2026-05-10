# Troubleshooting Log

이 파일은 실차 테스트 및 개발 중 발생한 문제와 해결 과정을 기록합니다.
체인지로그에 담기엔 너무 긴 원인 분석 및 배선 이슈를 포함합니다.

---

## 2026-04-22 — B채널 BUS-OFF 반복 발생 (30회/6분)

### 증상
- 웹 대시보드: `BUS-OFF 발생: 30`, `복구(시도/성공/실패): 30/30/0`
- 오류 피크: `RX/TX = 0 / 240`
- 수신(B-RX): 정상 ~100Hz, 에코 전송: 수신과 거의 1:1 비율
- 복구 자체는 성공(11ms)하나 곧 재발

### 원인 분석

#### 1순위 (소프트웨어): `WIFI_AP_STA` 백그라운드 채널 스캔
- `web_server.h`에서 `WiFi.mode(WIFI_AP_STA)`로 초기화 → STA 모드 활성화
- STA 모드는 WiFi 백그라운드 채널 스캔을 수행 (100ms 단위 라디오 전환)
- 이 순간 CPU 인터럽트 레이턴시 급증 → TWAI TX가 ACK 수신 윈도우(수십 µs) 를 놓침
- ACK 에러 → TX 에러 카운터 증가 → 256 도달 → BUS-OFF
- **이전 버전(71c3d70)에서 버스오프가 1~2회로 적었던 이유:** ESP-NOW 추가 전에는 순수 AP 모드만 사용

**수정 (2026-04-22):**
```cpp
// web_server.h: WIFI_AP_STA → WIFI_AP
WiFi.mode(WIFI_AP);  // STA 모드 제거 → 채널 스캔 없음

// main.cpp: initEspNowSender()에서 WiFi.mode() 중복 호출 제거
// esp_wifi_set_ps(WIFI_PS_NONE) 유지 (파워세이브 완전 비활성)
```

> **근거:** ESP-NOW는 STA 접속 없이 AP 모드만으로 브로드캐스트 가능.
> STA 모드는 실제 WiFi 네트워크 접속 시에만 필요.

#### 2순위 (소프트웨어): send() 내 per-call 상태 조회 과잉 엔지니어링
- 이전 수정에서 `send()` 호출마다 `twai_get_status_info()` 실행 (100Hz × syscall)
- 임계값 5개(192, 224, soft/hard 타이머, 큐 워터마크)로 복잡도 증가
- `nagKillerTask`가 이미 1ms마다 상태를 읽고 있으므로 중복

**수정:** task 레벨에서 CAN 규격 기반 임계값 128(error-passive 진입점)에 도달 시
`setTxBackoff(200ms)` 호출. `send()`는 백오프 타이머 체크만 수행.

```cpp
// nagKillerTask (1ms 루프, 기존에도 twai_get_status_info 호출 중)
if (twSt.tx_error_counter >= 128) {
    driverB->setTxBackoff(200);  // 200ms 에코 중단
}
```

> **왜 128인가:** CAN ISO 11898-1 규격상 TX 에러 카운터 128 = error-passive 전환점.
> 192/224는 BUS-OFF(256)에 너무 가까운 임의값이었음.

#### 3순위 (하드웨어): GND 미접지
- T2-CAN 보드를 USB-C로만 전원 공급하고 차량 GND 연결 없음
- CAN TX는 절대 전압 기준으로 다른 노드가 dominant bit 샘플링 → GND 기준 다르면 ACK 불가
- RX=0, TX=240 비대칭 패턴은 GND 미접지의 전형적 증상

**조치:** OBD-II 핀 4 또는 5(GND) → T2-CAN 보드 GND 핀 연결

### 진단 체크리스트

| 확인 항목 | 증상 | 상태 |
|---|---|---|
| `WIFI_AP_STA` → `WIFI_AP` 변경 | STA 채널 스캔 제거 | ✅ 수정됨 |
| TX 백오프 단순화 | per-call syscall 제거 | ✅ 수정됨 |
| OBD GND 연결 | RX=0/TX≫0 비대칭 해소 | ⬜ 하드웨어 조치 필요 |
| 종단저항 합 60Ω 확인 | CAN_H/L 반사 방지 | ⬜ 확인 필요 |

### 기대 결과
- 펌웨어 수정 후: BUS-OFF 빈도 대폭 감소 (STA 스캔 제거)
- GND 연결 후: BUS-OFF 0에 수렴 (ACK 정상 수신)

---

## 2026-04-25 — 남은 잠재 리스크 점검 (Stealth/Original 공존 기준)

### 증상
- 실차 안정도는 개선됐지만, 운영 중 남아있는 소규모 리스크를 선제 점검할 필요가 있음
- 점검 대상:
    - `nagKillerActive`와 `nagKillerRuntime`의 동기화 해석 혼선
    - Stealth 토크 워크 범위의 실차 수용성
    - echo 전 `delayMicroseconds(100)` 이후 `twai_transmit()` 블로킹 지터

### 원인 분석

#### 1) `nagKillerActive` vs `nagKillerRuntime`
- 현재 런타임 ON/OFF는 웹 토글 경로에서 `nagKillerRuntime`이 실질 제어
- 핸들러 내부 `nagKillerActive`는 테스트/로컬 경로를 제외하면 운영 중 값 변경 경로가 사실상 없음
- 결과적으로 동작상 충돌보다는 "중복 게이트로 인한 해석 혼선"이 리스크

#### 2) Stealth `_torqWalk` 범위
- 현재 범위: 기본 2150~2290, 간헐 burst 2350±20
- 환산 토크:
    - 2150 -> +1.0Nm
    - 2230 -> +1.8Nm
    - 2290 -> +2.4Nm
    - 2330~2370 -> +2.8~+3.2Nm
- 즉, 기존 Original 고정점(+1.8Nm) 주변에서 움직이며 burst는 짧은 구간으로 제한됨

#### 3) `delayMicroseconds(100)` + `twai_transmit()` 지터
- 500kbps 기준 1bit=2us, EOF+Intermission 최소 여유는 약 20us
- 100us 지연은 버스 idle 안전 마진으로 타당
- 지터의 주원인은 delay가 아니라 `twai_transmit(..., 2ms)`의 가변 대기
- 다만 100Hz(10ms 주기) 대비 2ms 상한은 관리 가능한 범위이며, 백오프/드롭 카운터로 관측 가능

### 수정 내용
- 코드 수정 없음 (동작 보존)
- 운영 점검 절차를 추가해 리스크를 관측 기반으로 관리

### 운영 점검 절차 (실차 10~20분)

1. 모드별 순차 검증
- Original 10분 -> Stealth 10분 순서로 동일 구간 주행

2. 관측 지표
- `busoff_count`, `twai_tx_err_peak`, `txBackoffDropCount`, `frameHz`
- `torqueNm` 분포가 대체로 +1.0~+3.2Nm 내에서 유지되는지 확인

3. 이상 판단 기준
- `busoff_count`가 분당 5 이상 증가하면 이상
- `txBackoffDropCount`가 지속적으로 단조 증가하면 송신 압박 의심
- `torqueNm`가 범위를 반복적으로 이탈하면 Stealth 토크 워크 파라미터 재조정

### 후속 개선 옵션 (필요 시)

1. 게이트 단순화
- 운영 경로에서 `nagKillerRuntime` 단일 게이트로 정리하고 `nagKillerActive`는 테스트 전용으로 명시

2. Stealth 범위 축소 프로파일
- 장거리 테스트에서 변동이 크면 `_torqWalk` 상단/버스트 폭을 소폭 축소

3. 지터 가시화 강화
- 이미 수집 중인 `lastEchoCallUs`, `txBackoffDropCount`를 기준으로 경고 임계 로그만 추가

### 기대 결과
- 현재 안정성을 유지하면서도, 재발 가능성을 수치 기반으로 조기 감지
- 코드 변경 전에도 운영 판단이 가능해 롤백 없는 보수적 대응 가능

---

## 이슈 추가 템플릿

```
## YYYY-MM-DD — 제목

### 증상

### 원인 분석

### 수정 내용

### 기대 결과
```
