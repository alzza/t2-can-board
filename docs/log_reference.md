# CanMod 통합 로그 완전 해설서

> 이 문서는 `/api/logs-bundle` 로 다운로드되는 통합 로그 파일의 모든 필드와 판정 코드를 초보자도 이해할 수 있도록 예시와 함께 설명한다.
>
> 로그 파일 예시: `canmod_20260508_194303.txt`

현재 통합 로그는 시계열 전체를 큰 임시 버퍼에 복사하지 않고 링버퍼에서 한 행씩
복사해 HTTP 응답으로 전송한다. 따라서 저장 버튼을 누른 뒤 Safari가 파일을 만드는
동안 잠시 기다려야 할 수 있지만, 과거처럼 약 79.7KB의 추가 힙을 한 번에 요구하지
않는다. Web UI 상태 갱신은 통합 로그 저장 시 최대 15초 후 자동 재개된다.

---

## 목차

1. [로그 파일 전체 구조](#1-로그-파일-전체-구조)
2. [섹션 1 — 헤더 정보](#2-섹션-1--헤더-정보)
3. [섹션 1 — 런타임 로그 줄별 해설](#3-섹션-1--런타임-로그-줄별-해설)
4. [섹션 2 — BUS-OFF 이벤트 로그](#4-섹션-2--bus-off-이벤트-로그)
5. [섹션 3 — 채널 상태 스냅샷](#5-섹션-3--채널-상태-스냅샷)
6. [섹션 4 — 시계열 로그 섹션 완전 해설](#6-섹션-4--시계열-로그-섹션-완전-해설)
7. [섹션 5 — 밀리초 이벤트 로그](#7-섹션-5--밀리초-이벤트-로그)
8. [판정 코드 완전 해설](#8-판정-코드-완전-해설)
9. [로그 분석 실전 가이드](#9-로그-분석-실전-가이드)

---

## 1. 로그 파일 전체 구조

```
=== CanMod 통합 로그 ===          ← 파일 헤더 (기기 정보)
Generated: ...
...

=== [1] 런타임 로그 ===           ← 5초마다 기록되는 실시간 상태 로그
...

=== [2] BUS-OFF 이벤트 로그 ===  ← BUS-OFF가 발생했을 때만 채워짐
...

=== [3] 채널 상태 스냅샷 ===      ← 로그 생성 시점의 최신 상태
...

=== [4] 10분 시계열 로그 ===      ← 통합 로그 안의 표 형식 섹션, 시간에 따른 변화 추적
...

=== [5] 밀리초 이벤트 로그 ===    ← 중요 이벤트의 정확한 발생 시각
...
```

---

## 2. 섹션 1 — 헤더 정보

```
=== CanMod 통합 로그 ===
Generated: 2026-05-08 19:43:03.780
Firmware: 1.2.0
Build: FW120-2605081918
Env: lilygo_t2can
BuiltAt: 2026-05-08T19:18:22+09:00
Git: can-bus-stabilization/bb4e5c9 dirty=1 source=a3f7e5ed5b0b
Uptime: 1460435 ms (00:24:20)
BUS-OFF 쿨다운: 1000 ms
```

| 필드 | 예시값 | 의미 |
|------|--------|------|
| `Generated` | 2026-05-08 19:43:03.780 | 로그 파일이 생성된 실제 시각 |
| `Firmware` | 1.2.0 | 현재 실행 중인 펌웨어 버전 |
| `Build` | FW120-2605081918 | 빌드 ID (버전코드 + 날짜/시간). `FW120` = v1.2.0, `2605081918` = 2026-05-08 19:18 빌드 |
| `Env` | lilygo_t2can | PlatformIO 빌드 환경명. 항상 `lilygo_t2can`이어야 정상 |
| `BuiltAt` | 2026-05-08T19:18:22+09:00 | 펌웨어가 컴파일된 시각 (KST) |
| `Git` | can-bus-stabilization/bb4e5c9 dirty=1 | 현재 Git 브랜치/커밋. `dirty=1`이면 커밋되지 않은 변경이 있는 빌드 |
| `Uptime` | 1460435 ms (00:24:20) | 기기 부팅 후 경과 시간. 짧으면 최근에 재부팅된 것 |
| `BUS-OFF 쿨다운` | 1000 ms | BUS-OFF 복구 후 재시도 전 대기 시간 |

> **`dirty=1` 빌드란?**  
> 펌웨어를 USB로 업로드할 때 커밋하지 않은 수정이 있으면 `dirty=1`로 표시된다.  
> 재현 불가능한 빌드이므로, 장기 테스트용 펌웨어는 `dirty=0`으로 관리하는 것이 좋다.

---

## 3. 섹션 1 — 런타임 로그 줄별 해설

5초마다 다음 6종류의 로그가 순서대로 출력된다.

---

### 🟢⚡ [A-CH] 주입 완료 줄

```
🟢⚡ [A-CH] TSLLC 주입 완료: 정지/출발 제어 활성
🔵⚡ [A-CH] 작동 OK: EAP 규제 완화 주입 완료
```

주입 이벤트가 발생할 때마다 출력된다. 5초 집계와는 별도로 실시간으로 찍힌다.

| 아이콘 | 의미 |
|--------|------|
| 🟢⚡ | TSLLC(정지/출발 자동 제어) 주입 성공 |
| 🔵⚡ | EAP(Enhanced Autopilot 규제 완화) 주입 성공 |

---

### 🟢 [A-CH] 5초 집계 줄

```
🟢 [A-CH] RX:8621 | 1021:8621 | mod:2873 | TX:OK=5662/Fail=85 | TEC=0/peak=0 | REC=0 | MERRF=1 | RX-OVR=1 | EFLG=0x00
```

**A채널(MCP2515 SPI)** 의 5초 누적 상태 요약이다.  
A채널은 차량 원본 CAN 버스에서 프레임을 읽고, 특정 ID 프레임을 변조하여 재전송하는 "주입 채널"이다.

| 필드 | 예시 | 의미 | 정상 기준 |
|------|------|------|-----------|
| `RX` | 8621 | 총 수신 프레임 수 (부팅 이후 누적) | 계속 증가 |
| `1021` | 8621 | ID 0x1021 수신 누적 수. EAP/TSLLC 주입의 트리거가 되는 프레임 | RX와 동일하면 이 버스에는 1021만 있는 것 |
| `mod` | 2873 | 실제로 데이터를 수정해 재전송한 횟수. MuxID==1인 1021 프레임만 수정된다 | 1021의 약 1/3 (MuxID는 3가지 중 하나) |
| `TX:OK` | 5662 | 전송 성공 횟수 (누적) | 계속 증가 |
| `TX:Fail` | 85 | 전송 실패 횟수 (누적). MCP2515가 버스 경쟁에서 지거나 CRC 오류 발생 시 증가 | 낮을수록 좋음. 100 이하는 장시간 운행에서 허용 범위 |
| `TEC` | 0 | 송신 에러 카운터. 0=정상, 96 이상=오류 경고, 128 이상=에러 패시브, 256에서 BUS-OFF 진입 | **반드시 0** |
| `TEC/peak` | 0 | TEC 최고 도달값 (부팅 후 최대) | 0에 가까울수록 좋음 |
| `REC` | 0 | 수신 에러 카운터 | **반드시 0** |
| `MERRF` | 1 | MCP2515 메시지 에러 인터럽트 누적 횟수 | 1~2 정도는 무시 가능 |
| `RX-OVR` | 1 | 수신 버퍼 오버플로 횟수. ESP32 태스크가 너무 바빠서 MCP 버퍼를 제때 비우지 못할 때 발생 | 1~2 정도는 허용. 10↑이면 처리 지연 의심 |
| `EFLG` | 0x00 | MCP2515 에러 플래그 레지스터 값. 각 비트가 특정 에러를 나타냄 | **0x00** |

> **EFLG 비트 의미 (0x00이 아닐 때):**
> - `0x01`: EWARN — TEC 또는 REC가 96 이상 (에러 경고)
> - `0x02`: RXWAR — 수신 에러 카운터 96 이상
> - `0x04`: TXWAR — 송신 에러 카운터 96 이상
> - `0x08`: RXEP — 수신 에러 패시브 (REC ≥ 128)
> - `0x10`: TXEP — 송신 에러 패시브 (TEC ≥ 128)
> - `0x20`: TXBO — 버스 오프 (TEC ≥ 256)
> - `0x40`: RX1OVR — 수신 버퍼 1 오버플로
> - `0x80`: RX0OVR — 수신 버퍼 0 오버플로

---

### 🔵 [B-CH] 5초 집계 줄

```
🔵 [B-CH] RX:3855420|Filt:277414|Echo:17772|TxFail:0|TEC:0/REC:0|880:130771|921:0|923:3087|297:143556|DAS:1@923|Mode:A|TWAI:O
```

**B채널(TWAI 내장 CAN)** 의 5초 누적 상태 요약이다.  
B채널은 EPAS(전동 파워스티어링) CAN 버스를 감시하면서, 조건이 맞을 때 에코 프레임을 주입하여 "핸즈온" 신호를 조작하는 "Nag Killer 채널"이다.

| 필드 | 예시 | 의미 | 정상 기준 |
|------|------|------|-----------|
| `RX` | 3855420 | B채널 총 수신 프레임 수 (누적). B버스는 트래픽이 매우 많다 | 계속 증가 |
| `Filt` | 277414 | 소프트웨어 필터를 통과한 감시 대상 프레임 수 (ID 880/921/923/297만 통과) | 계속 증가 |
| `Echo` | 17772 | B채널에서 발사한 에코(변조 주입) 패킷 수 (누적) | Nag Killer 동작 시 증가 |
| `TxFail` | 0 | B채널 TX 최종 실패 수 | **반드시 0** |
| `TEC` | 0 | TWAI 송신 에러 카운터 | **반드시 0** |
| `REC` | 0 | TWAI 수신 에러 카운터 | **반드시 0** |
| `880` | 130771 | **ID 880 (EPAS 핸들 상태)** 수신 누적. **Nag 에코의 직접 트리거** | 차량 주행 중 ~100Hz로 증가 |
| `921` | 0 | **ID 921 (DAS_status, CH 버스용)** 수신 누적. Model Y PARTY 버스에서는 보통 0 | 0이어도 923에서 대체 가능 |
| `923` | 3087 | **ID 923 (DAS_status, PARTY 버스용)** 수신 누적. AP 상태를 알려주는 핵심 ID | ~2Hz, 5초에 10~11개 증가 |
| `297` | 143556 | **ID 297 (SCCM 조향각 센서)** 수신 누적. 조향각과 조향각 속도 포함 | ~100Hz, 5초에 ~500개 증가 |
| `DAS` | 1@923 | 현재 DAS Autopilot 상태값과 소스 ID. `1@923` = 923 프레임에서 읽은 상태값 1 | 아래 DAS 상태표 참고 |
| `Mode` | A | 현재 Nag 동작 모드. A=A채널 기반 EAP 주입, B=B채널 에코 주입 활성 | 주행/AP 조건 따라 변동 |
| `TWAI` | O | TWAI 드라이버 상태 코드 | O=OK, R=Recovering, X=BUS-OFF |

> **DAS 상태값 의미:**
> | 값 | 의미 | Nag 발생 가능성 |
> |----|------|----------------|
> | 0 | AP 비활성 | Nag 없음 |
> | 1 | AP 활성 (정상 주행 중) | Nag 없음 (정상 상태) |
> | 2 | AP 활성 + **핸들 잡기 요청 (Nag!)** | **에코 주입 대상** |
> | 3~7 | AP 다양한 경고 단계 | 에코 주입 대상 |
> | 9, 10 | AP 특수 모드 | 에코 주입 대상 |

---

### 🥷 [B-NAG] Nag Killer 5초 집계

```
🥷 [B-NAG] 5s 880:+0 921:+0 923:+10 297:+500 Echo:+0 Drop:+0 | Mode=A AP=1 Phase=0 HO=0 DAS=0x01@923 Verdict=NO_880
```

Nag Killer 로직이 지난 5초 동안 무엇을 감지했고 어떻게 판정했는지 요약이다.

| 필드 | 예시 | 의미 | 분석 팁 |
|------|------|------|---------|
| `5s 880:+N` | +0 | 지난 5초간 ID 880 수신 증가량 | **+0이면 880이 안 들어오는 것. 에코 불가** |
| `5s 921:+N` | +0 | 지난 5초간 ID 921 수신 증가량 | Model Y에서는 +0이 정상 |
| `5s 923:+N` | +10 | 지난 5초간 ID 923 수신 증가량 | 5초에 10~11개가 정상 (~2Hz) |
| `5s 297:+N` | +500 | 지난 5초간 조향각 수신 증가량 | 100Hz → 5초 = **약 500개가 정상** |
| `Echo:+N` | +0 | 지난 5초간 에코 발사 횟수 | 에코 조건 충족 시 880과 비슷하게 증가 |
| `Drop:+N` | +0 | 타이밍 초과로 에코를 포기한 횟수 | 0이 이상적. 증가하면 처리 지연 |
| `Mode` | A | 현재 Nag 동작 모드 (A 또는 B) | |
| `AP` | 1 | 현재 DAS Autopilot 상태값 | 2 이상이어야 에코 대상 |
| `Phase` | 0 | Nag 에코 시퀀스 단계 (Mode B 전용) | 0=대기, 1~5=에코 진행 중 |
| `HO` | 0 | HandsOn 레벨 (0=손 안 잡음) | 0이어야 에코 가능 |
| `DAS` | 0x01@923 | DAS 상세 상태값 + 소스 ID | `0x02@923`이 에코 트리거 상태 |
| `Verdict` | NO_880 | **이번 5초의 최종 Nag 판정 결과** | 아래 판정 코드표 참고 |

---

### 🚦 [B-GATE] 에코 게이트 통계

```
🚦 [B-GATE] Skip OFF/AP/HO/DAS:+0/0/0/0 | NoDAS Echo:+0 | Last=ECHO
```

에코를 막은 원인별 카운터다. "왜 에코가 발사되지 않는가"를 파악할 때 핵심 줄이다.

| 필드 | 예시 | 의미 | 증가 조건 |
|------|------|------|-----------|
| `Skip OFF` | +0 | 기능이 OFF로 설정되어 스킵된 880 수 | 웹 UI에서 Nag Killer를 끄면 증가 |
| `Skip AP` | +0 | AP 게이트에서 차단된 수 | AP가 활성 상태가 아닐 때 880이 들어와도 차단 |
| `Skip HO` | +0 | 핸들 잡고 있어 스킵된 수 | 운전자가 실제로 핸들을 잡으면 증가 |
| `Skip DAS` | +0 | DAS가 Nag를 요청하지 않는 상태라서 스킵된 수 | AP 상태값이 1(정상)이면 증가 |
| `NoDAS Echo` | +0 | DAS 없이 강제 에코한 횟수 (특수 케이스) | 거의 발생 안 함 |
| `Last` | ECHO | 마지막 게이트 판정 결과 | ECHO=정상 발사, 기타=스킵 이유 |

> **예시 해석: `Skip OFF/AP/HO/DAS:+0/500/0/0`**  
> 5초 동안 880이 500개 들어왔지만, 전부 AP 게이트에서 막혔다.  
> AP가 활성화되지 않은 상태(예: 정차, 차량 수동 운전 중)였다는 뜻이다.

---

### 🔬 [B-DEEP] TWAI 심층 진단

```
🔬 [B-DEEP] ArbLost:21894|BusErr:17|TxFailed:0|RxMissed:0|EchoLat:0us|EchoDrop:0|Skip RT:0/AP:97568/HO:355/DAS:15076
```

TWAI 하드웨어 레벨의 에러 통계와 누적 스킵 카운터다.

| 필드 | 예시 | 의미 | 정상 기준 |
|------|------|------|-----------|
| `ArbLost` | 21894 | 버스 중재(Arbitration) 패배 누적. 같은 시각 여러 장치가 동시에 송신할 때 발생 | 천천히 증가하는 건 정상. 갑자기 폭증하면 동일 ID 충돌 |
| `BusErr` | 17 | TWAI 하드웨어 버스 에러 누적 (bit error, stuff error 등) | 낮을수록 좋음. 100 이하는 허용 |
| `TxFailed` | 0 | TX 최종 실패 누적 (재시도 다 했는데도 실패) | **반드시 0** |
| `RxMissed` | 0 | 수신 놓친 프레임 수 | **반드시 0** |
| `EchoLat` | 0us | 880 수신 → 에코 전송까지의 지연 시간 | 수백 us 이하 |
| `EchoDrop` | 0 | 타이밍 초과로 에코를 포기한 누적 횟수 | **0이어야 이상적** |
| `Skip RT` | 0 | 런타임 OFF로 누적 스킵 (기능 껐을 때) | 기능 ON이면 0 |
| `Skip AP` | 97568 | AP 게이트로 누적 스킵 (가장 많은 이유) | AP 비활성 시간만큼 자연스럽게 증가 |
| `Skip HO` | 355 | 핸즈온 감지로 누적 스킵 | 운전자가 핸들 잡은 시간만큼 증가 |
| `Skip DAS` | 15076 | DAS 상태 부적합으로 누적 스킵 | DAS 상태 1(정상 AP)일 때 증가 |

> **ArbLost가 많은 이유:**  
> EPAS CAN 버스는 초당 수백~수천 개의 프레임이 오가는 고트래픽 버스다.  
> 에코를 주입할 때 차량의 ECU들과 동시에 송신 시도를 하면 중재 패배가 발생한다.  
> `ArbLost`가 증가하더라도 `TxFailed`가 0이면 결국 재시도로 성공한 것이다.

---

### 🔍 [DBG] 요약 진단

```
🔍 [DBG] B-Other:3578006 | B-LastID:297 | TWAI-Code:1 | CanTaskOK:1 | BDriverOK:1 | TWAIerr:0/0
```

| 필드 | 예시 | 의미 | 정상값 |
|------|------|------|--------|
| `B-Other` | 3578006 | 감시 대상(880/921/923/297) 외의 나머지 프레임 수. 이 버스에 얼마나 많은 다른 트래픽이 있는지를 나타냄 | 매우 많아도 정상 |
| `B-LastID` | 297 | 가장 마지막에 수신된 프레임의 CAN ID | 297, 880, 910, 923 등이 번갈아 |
| `TWAI-Code` | 1 | TWAI 상태 코드 숫자값 | **1=RUNNING** (아래 표 참고) |
| `CanTaskOK` | 1 | B채널 CAN 태스크(FreeRTOS)가 살아있는지 여부 | **1** |
| `BDriverOK` | 1 | TWAI 드라이버 초기화 성공 여부 | **1** |
| `TWAIerr` | 0/0 | TWAI install 에러코드 / start 에러코드 (0=성공) | **0/0** |

> **TWAI-Code 값 의미:**
> | 코드 | 상태 | 의미 |
> |------|------|------|
> | 0 | STOPPED | 드라이버 설치됐지만 시작 안 됨 |
> | **1** | **RUNNING** | **정상 동작 중** |
> | 2 | BUS_OFF | BUS-OFF 상태. 복구 대기 중 |
> | 3 | RECOVERING | BUS-OFF 복구 절차 진행 중 |
> | 4 | NOT_INSTALLED | 드라이버 미설치 |

---

## 4. 섹션 2 — BUS-OFF 이벤트 로그

```
=== [2] BUS-OFF 이벤트 로그 ===
seq,wall_time,timestamp_ms,tec,rec,recovery_dur_ms,since_last_ms,recovered
(BUS-OFF 없음)
```

BUS-OFF가 한 번도 발생하지 않으면 `(BUS-OFF 없음)`으로 표시된다.  
BUS-OFF가 발생했을 때는 아래와 같이 기록된다:

```
1,2026-05-08 19:30:00.000,500000,256,0,1200,0,1
```

| 컬럼 | 의미 |
|------|------|
| `seq` | BUS-OFF 발생 순번 (1번부터 시작) |
| `wall_time` | 발생 시각 |
| `timestamp_ms` | 기기 업타임 기준 발생 시각 |
| `tec` | 발생 당시 TEC 값 (256이면 완전 BUS-OFF) |
| `rec` | 발생 당시 REC 값 |
| `recovery_dur_ms` | 복구 완료까지 걸린 시간 (ms) |
| `since_last_ms` | 이전 BUS-OFF로부터 경과 시간 (첫 번째는 0) |
| `recovered` | 복구 성공 여부 (1=성공, 0=실패) |

---

## 5. 섹션 3 — 채널 상태 스냅샷

```
=== [3] 채널 상태 스냅샷 ===
A채널: RX=8727 1021=8727 mod=2908
A진단: TX OK=5732 Fail=85 | TEC=0/peak=0 REC=0 | MERRF=1 | RX-OVR=1 | EFLG=0x00/peak=0x40 | BUS-OFF=0 | Cfg=SPI10000000->10000000/OSOFF/GuardOFF | Guard=OFF/0ms/NONE skip=0 count=0
A진단2: REC=0/peak=0 | LastRx=238ms ago | LastTx=288ms ago | EFLG이벤트=1
B채널: RX=3867497 Filt=279228 Echo=17772 TxFail=0 TEC=0 REC=0 TECpeak=0 880=130771 921=0 923=3123 297=145334 DAS=1@923 Mode=A TWAI=OK InitErr=0/0
B심층: ArbLost=21894 BusErr=17 TxFailed=0 RxMissed=0 | EchoLat=0us EchoDrop=0 | Skip RT=0/AP=97568/HO=355/DAS=15076
B나그판정: 880=130771(age=95394ms) 921=0(age=0ms) 923=3123(age=403ms) 297=145334(age=7ms) Echo=17772(age=95394ms) | Mode=A AP=1 Phase=0 HO=0 Torque=0.06Nm DAS=0x01@923 Last=ECHO
B차단사유: OFF=0 AP_BLOCK=97568 HandsOn=355 DAS_IDLE=15076 LateDrop=0 NoDAS_Echo=0
복구: 시도=0 성공=0 실패=0 마지막=0ms 최대소요=0ms
사용자마커: count=0 last_age=0ms detail=0
Web진단: status=414(age=2508ms dur=153/153ms) nagStats=1227(age=519ms dur=4/7ms) logsBundle=1(age=27ms dur=0/0ms) heap=214192/176720 apSta=1 apChg=1(age=1452669ms)
```

로그 생성 시점의 최신 상태를 한 번에 모아 보여준다.

### 중요 필드 해설

**A진단 중 `Cfg=SPI10000000->10000000/OSOFF/GuardOFF`:**
- `SPI10000000->10000000`: 요청된 SPI 속도와 실제 설정된 속도 (10MHz → 10MHz 정상)
- `OSOFF`: One-Shot 모드 OFF (재전송 있음)
- `GuardOFF`: TX 가드 타이머 OFF

**B나그판정 중 `age` 필드:**
- `880=130771(age=95394ms)`: ID 880이 130771번 수신됐고, 마지막으로 수신된 것이 **95394ms(약 95초) 전**이라는 뜻
- `age=95394ms`면 880이 95초째 오지 않고 있다는 것 → **에코 불가 상태**

**`Torque=0.06Nm`:**
- 마지막으로 주입한 에코의 토크값 (Nm 단위)
- center=0Nm, 양수=오른쪽, 음수=왼쪽

**Web진단 `heap=214192/176720`:**
- `214192` = 현재 free heap (남은 메모리)
- `176720` = 최소 free heap (부팅 이후 가장 적었던 시점)
- 100KB 이상이면 안전. 50KB 이하면 메모리 부족 위험

---

## 6. 섹션 4 — 시계열 로그 섹션 완전 해설

```
=== [4] 10분 시계열 로그 ===
# reset_at_ms=0 rec_start_ms=0 rec=OFF samples=120 interval_s=5
wall_time,timestamp_ms,busoff,tec,rec,...
2026-05-08 19:33:05.631,862286,0,0,0,...
```

5초마다 한 줄씩 기록되며, 최대 10분치(120줄)를 RAM 링버퍼에 보관한다.  
`samples=120`이면 현재 120개(=10분치)가 통합 로그에 출력될 상태다.

---

### 표 컬럼 전체 해설

> 컬럼명 앞에 **`f`** 가 붙으면 **부팅 이후 누적값(Cumulative)**  
> 컬럼명 앞에 **`d`** 가 붙으면 **이번 5초 구간 증가량(Delta)**

#### 기본 정보 컬럼

| 컬럼 | 예시값 | 의미 | 분석 팁 |
|------|--------|------|---------|
| `wall_time` | 2026-05-08 19:33:05.631 | 실제 시계 시간 | 시간대 이상하면 폰 시계와 비교 |
| `timestamp_ms` | 862286 | 기기 부팅 이후 경과 ms. `wall_time`에서 `Generated`를 빼면 offset 확인 가능 | |

#### TWAI 에러 상태 컬럼 (누적)

| 컬럼 | 예시값 | 의미 | 정상값 |
|------|--------|------|--------|
| `busoff` | 0 | 현재 BUS-OFF 상태 여부 (1이면 BUS-OFF 진행 중) | **0** |
| `tec` | 0 | TWAI 송신 에러 카운터. 128 이상이면 에러 패시브, 256이면 BUS-OFF | **0** |
| `rec` | 0 | TWAI 수신 에러 카운터 | **0** |
| `arbLost` | 10337 | 버스 중재 패배 누적. 5초마다 조금씩 증가하면 정상 | 급증 시 경쟁 의심 |
| `busErr` | 0 | 버스 에러 누적 | 낮을수록 좋음 |
| `txFail` | 0 | TX 최종 실패 누적 | **0** |

#### Nag 핵심 카운터 컬럼 (누적, `f` 접두사)

| 컬럼 | 예시값 | 의미 | 분석 팁 |
|------|--------|------|---------|
| `echo` | 17772 | 에코 발사 총 횟수 (누적). 이 로그에서는 19:41:30 이후 고정됨 | 증가 멈추면 에코 조건 미충족 |
| `f880` | 130771 | ID 880 수신 누적. **Nag 에코의 직접 트리거** | 19:41:30 이후 고정 → 880 수신 중단 |
| `f921` | 0 | ID 921 수신 누적 | Model Y에서 0은 정상 |
| `f923` | 3087 | ID 923 수신 누적. AP 상태를 알려주는 핵심 ID | 로그 내내 계속 증가 → 정상 |
| `ho` | 0 | HandsOn 레벨 (현재값) | 0=손 안 잡음 |
| `dasState` | 1 | DAS Autopilot 상태값 | 2 이상이어야 Nag 에코 발생 |
| `nagMode` | 0 | Nag 동작 모드 (0=A모드, 1=B모드) | |
| `dasSource` | 923 | DAS 상태를 읽고 있는 소스 ID | 923 또는 921 |

#### 스킵 카운터 컬럼 (누적)

| 컬럼 | 예시값 | 의미 |
|------|--------|------|
| `echoDrop` | 0 | 타이밍 초과로 에코를 포기한 누적 횟수 |
| `skipOff` | 0 | 기능 OFF로 스킵된 880 수 누적 |
| `skipAP` | 97568 | **AP 게이트 차단 누적** — AP가 활성화되지 않은 상태에서 들어온 880 수. 가장 많은 스킵 원인 |
| `skipHO` | 355 | 핸들 잡고 있어 스킵된 수 누적 |
| `skipDAS` | 15076 | DAS 상태 불일치로 스킵된 수 누적. AP=1(정상 주행) 상태일 때 증가 |
| `noDAS` | 0 | DAS 없이 에코한 횟수 누적 |
| `userMark` | 0 | 사용자 마커 횟수 누적 |

#### Delta 컬럼 — 이번 5초간 변화량

| 컬럼 | 정상 시 예시값 | 의미 | 분석 팁 |
|------|--------------|------|---------|
| `d880` | 500 | 5초간 ID 880 수신 수. **100Hz라면 ~500이 정상** | 0이면 880 미수신 → 에코 불가 |
| `d921` | 0 | 5초간 ID 921 수신 수 | Model Y에서 0은 정상 |
| `d923` | 10~11 | 5초간 ID 923 수신 수. ~2Hz → 5초에 **10~11개가 정상** | 0이면 DAS 상태 알 수 없음 |
| `dEcho` | 0~500 | 5초간 에코 발사 수. AP Nag 조건 충족 시 d880과 비슷해야 함 | d880 있는데 dEcho=0이면 스킵 원인 확인 |
| `dDrop` | 0 | 5초간 에코 포기 수 | 0이 이상적 |
| `dSkipOff` | 0 | 5초간 기능 OFF 스킵 수 | 기능 켜면 0 |
| `dSkipAP` | 500 | 5초간 AP 게이트 스킵 수 | AP 비활성 구간에 d880과 같은 값 |
| `dSkipHO` | 0 | 5초간 HandsOn 스킵 수 | 운전자가 핸들 잡으면 증가 |
| `dSkipDAS` | 0~500 | 5초간 DAS 상태 스킵 수 | AP=1(정상) 상태면 d880과 같은 값 |
| `dNoDAS` | 0 | 5초간 DAS 없이 에코 수 | |
| `dUserMark` | 0 | 5초간 사용자 마커 수 | |

#### 판정 코드 컬럼

| 컬럼 | 예시값 | 의미 |
|------|--------|------|
| `lastDecision` | 6 | **마지막 880 프레임 처리 시 판정 코드** (가장 최근 880 하나에 대한 결과) |
| `intervalDecision` | 9 | **이번 5초 구간 전체의 요약 판정 코드** (이번 5초를 대표하는 판정) |

판정 코드 값은 아래 8번 섹션에서 상세 해설.

#### Mode B 타이밍 보강 컬럼

이 컬럼들은 Smart Torque, 즉 Mode B의 딜레이를 실차 로그로 판단하기 위해 추가된 값이다. 5초 시계열은 세밀한 300~500ms 차이를 직접 재는 용도라기보다, 해당 5초가 어떤 상태였는지 알려주는 지도 역할을 한다.

| 컬럼 | 예시값 | 의미 | 분석 팁 |
|------|--------|------|---------|
| `f297` | 2030 | ID 297 조향각 프레임 누적 수 | 0이면 Mode B가 조향 방향을 판단하기 어렵다 |
| `apState` | 4 | DAS Autopilot state | Mode B는 보통 3~6일 때만 주입 후보가 된다 |
| `modeBPhase` | 2 | Mode B 내부 phase | 2는 state2 딜레이 대기, 4는 strong 딜레이 대기다 |
| `steerDeg` | -12.4 | 최근 조향각(deg) | 주입 토크 방향 판단에 사용된다 |
| `realTorqueNm` | 0.18 | EPAS 880에서 읽은 실제 토션바 토크 | 운전자가 실제로 힘을 준 흔적을 보는 값이다 |
| `modeBInject` | 1250 | Mode B가 주입한 echo 누적 수 | 증가하면 Mode B 주입이 실제로 나간 것이다 |
| `modeBLastNm` | 1.20 | Mode B가 마지막으로 주입한 토크(Nm) | 0 근처면 약한 주입 또는 미주입 상태다 |
| `age880Ms` | 12 | 마지막 880 수신 후 경과 시간 | 크면 880 수신 중단이므로 딜레이 문제가 아니다 |
| `ageDasMs` | 320 | 마지막 921/923 DAS 상태 수신 후 경과 시간 | 너무 크면 AP/DAS 판단이 낡은 값일 수 있다 |
| `age297Ms` | 18 | 마지막 297 조향각 수신 후 경과 시간 | 너무 크면 조향각 기반 방향 판단이 낡은 값이다 |
| `ageEchoMs` | 8 | 마지막 echo 주입 후 경과 시간 | 주입이 계속되는지 확인한다 |
| `modeBStateAgeMs` | 1840 | 현재 DAS hands-on state에 진입한 뒤 경과 시간 | state2에서 이 값이 선택 프로파일의 `state2DelayMs` 직전인데 마커가 찍히면 딜레이가 긴 후보다 |
| `modeBPhaseAgeMs` | 740 | 현재 Mode B phase에 진입한 뒤 경과 시간 | strong phase 4에서 이 값이 선택 프로파일의 `strongDelayMs` 직전이면 strong 딜레이가 긴 후보다 |
| `modeBFirstEchoDelayMs` | 700 | 현재 DAS state 진입 후 첫 Mode B echo까지 걸린 시간 | 실제 차량에서 체감된 최초 반응 시간이다 |
| `modeBDelayTargetMs` | 700 | 현재 phase가 기다리는 목표 딜레이 | 프로파일별 `state2DelayMs` 또는 `strongDelayMs`다. 0이면 딜레이 대기 phase가 아니다 |
| `d297` | 500 | 이번 5초 동안 들어온 297 수 | 100Hz라면 5초에 약 500이 정상이다 |
| `dModeBInject` | 300 | 이번 5초 동안 Mode B가 주입한 횟수 | `d880`과 비교해 주입 비율을 본다 |

Mode B phase는 아래처럼 읽으면 된다.

| phase | 의미 | 타이밍 판단 |
|-------|------|-------------|
| 0 | idle, 차단, 미주입 | 딜레이 튜닝 대상이 아니다 |
| 1 | state1 grace | 500ms grace 구간이다 |
| 2 | state2 delay | 프로파일의 `state2DelayMs` 대기 구간이다 |
| 3 | state2 mild | mild random-walk 주입 구간이다 |
| 4 | strong delay | 프로파일의 `strongDelayMs` 대기 구간이다 |
| 5 | strong ramp | 강토크가 0에서 2.1Nm로 올라가는 구간이다 |
| 6 | strong hold | 강토크 hold 구간이다 |

---

### 시계열 로그 읽는 방법

실제 이 로그에서 중요한 구간을 분석하면:

#### 구간 1: 19:33~38:35 (정상 대기 — AP 게이트 차단 중)

```csv
2026-05-08 19:33:05.631,862286,0,0,0,10337,0,0,5976,80492,0,1863,...,500,0,10,...,0,500,0,0,...,9,9
```

- `d880=500` → 880이 100Hz로 정상 수신
- `dEcho=0` → 에코 발사 없음
- `dSkipAP=500` → 들어온 880 500개가 전부 AP 게이트에서 차단
- `intervalDecision=9` (AP_BLOCK) → "AP가 활성이 아니라서 전부 차단"

#### 구간 2: 19:38:40~41:30 (AP 조건 전환기)

```csv
2026-05-08 19:38:40.636,...,0,0,0,10365,0,0,6006,113993,0,2540,0,1,...,500,0,10,30,...,0,431,0,39,...,4,1
```

- `dasState=1` → AP 상태가 1로 바뀜 (Nag 없는 정상 AP)
- `dSkipAP=0` → AP 게이트는 통과함
- `dSkipDAS=431` → 이번에는 DAS 상태가 맞지 않아 차단
- `intervalDecision=1` (ECHO) → 나머지 일부는 에코 발사 성공

#### 구간 3: 19:41:35 이후 (880 수신 완전 중단)

```csv
2026-05-08 19:41:35.636,...,0,0,0,21894,...,17772,130771,0,2945,...,0,11,0,...,0,0,0,...,1,6
```

- `f880=130771` 이후 고정 → 880이 더 이상 오지 않음
- `d880=0` → 5초간 880 수신 없음
- `echo` 누적도 고정 → 에코도 중단
- `intervalDecision=6` (NO_880) → "880이 없어서 에코 불가"

**결론**: 19:41:30에 차량이 주차/시동 OFF되거나 AP를 비활성화하여 EPAS가 880을 보내지 않게 되었다.

---

## 7. 채널별 이벤트 CSV v2

통합 로그의 섹션 5는 이벤트 개수 요약만 표시한다. 전체 이벤트 행은 Web UI의 **채널별 이벤트 CSV** 또는 `/api/events.csv`에서 저장한다.

```csv
schema_version,wall_time_first,wall_time_last,uptime_first_ms,uptime_last_ms,sequence,channel,severity,event,type,occurrences,tec,rec,detail,detail_text
2,2026-07-24 19:37:13.459,2026-07-24 19:37:39.459,51801454,51827454,81,A,WARN,A_RX_OVERRUN,17,8,0,0,1230080,"eflg=0x80 state=RX_OVERRUN RX1OVR=1 RX0OVR=0 loop_gap_us=4800"
```

CSV 앞에 열 수가 다른 `#` 메타행을 두지 않는다. 첫 행부터 끝까지 같은 15열이며 UTF-8 BOM을 포함한다.

| 컬럼 | 의미 |
|------|------|
| `schema_version` | 현재 형식은 `2` |
| `wall_time_first` / `wall_time_last` | 동일 이벤트 묶음의 최초·최종 실제 시각 |
| `uptime_first_ms` / `uptime_last_ms` | 실제 시각과 함께 보존하는 기기 업타임 |
| `sequence` | 고유 이벤트 레코드 순번 |
| `channel` | `A`, `B`, `A/B`, `SYSTEM` |
| `severity` | `INFO`, `WARN`, `ERROR` |
| `event` / `type` | 사람이 읽는 이벤트 이름과 숫자 코드 |
| `occurrences` | 30초 안에 같은 반복 이벤트가 발생한 횟수 |
| `tec` / `rec` | 발생 당시 해당 채널 CAN 오류 카운터 |
| `detail` | 이벤트별 원시 추가값 |
| `detail_text` | 원시값을 사람이 읽을 수 있게 해석한 결과 |

반복 빈도가 높은 A EFLG·RX 오버런과 B alert는 30초 구간으로 묶는다. 단순히 버리지 않고 최초/최종 시각, 횟수, A 오버런 구간의 최대 `loop_gap_us`를 남긴다.

#### 이벤트 타입

| 코드 | 이름 | 채널 | 의미 |
|------|------|------|------|
| 0~3 | `BUSOFF`, `REC_OK`, `REC_FAIL`, `REC_SOFT` | B | BUS-OFF와 복구 단계 |
| 4~8 | `ERR_PASS`, `ARB_LOST`, `BUS_ERR`, `TX_FAIL`, `RX_FULL` | B | TWAI alert |
| 10 | `USER_MARK` | A/B | 사용자가 차량 경고 구간 시작·종료 표시 |
| 11~14 | `NAG_MODE`, `MODEB_STATE`, `MODEB_PHASE`, `MODEB_FIRST_ECHO` | B | Nag 모드와 상태 전이 |
| 15~18 | `A_EFLG_SET`, `A_EFLG_CLEAR`, `A_RX_OVERRUN`, `A_WAKE_FIRST_TX` | A | MCP2515 오류와 재수신 지연 |
| 19~21 | `CAPTURE_START`, `CAPTURE_STOP`, `CAPTURE_RESET` | SYSTEM | 수동 기록 제어 |
| 22 | `FEATURE_STATE` | SYSTEM | A TX, Summon, TSLLC, Nag, One-Shot, TX Guard 상태 |
| 23~24 | `A_TX_GUARD_SET`, `A_TX_GUARD_CLEAR` | A | A TX Guard 진입·해제 |
| 25 | `A_SPI_TARGET` | A | 재부팅 후 적용할 SPI 목표 변경 |
| 26 | `FEATURE_ACTIVITY` | A/B | 5초 구간 기능별 실제 주입·게이트·AP 전이 |
| 27 | `A_TX_FAILURE` | A | Summon/TSLLC별 MCP2515 송신 실패. 출처, 즉시 결과 또는 완료 폴링 단계, 실제 TX 버퍼와 `TXERR`·`MLOA`·`ABTF`를 함께 기록 |
| 28 | `SUMMONING_STATE` | A | INO 기준 `ACA + SPR` 실제 Summoning 시작·종료와 세션별 Summon TX 성공·실패·차단 수 |
| 29 | `NAG_INJECTION_SESSION` | B | 실제 Nag 송신 세션 시작·종료, 모드·AP 상태·마지막 판정·세션 주입 수 |
| 30 | `SUMMON_UNLOCK_ACTIVITY` | A | 주차·AP 안정·실제 Summoning 조건에서 HW3 bit19/46 주입 활동 시작·종료. 실제 차량 호출 상태와 구분 |
| 31 | `NAG_GATE_STATE` | B | Nag 허용·기능 OFF·준비·AP 차단·Hands-on·DAS 차단 사유의 밀리초 전이. 같은 30초 구간의 반복 전이는 한 행으로 합산 |
| 32 | `A_TX_QUALITY` | A | 5초 구간에서 기능별 Summon/TSLLC의 MLOA 비율이 시도 10건 이상·50% 이상일 때 기록하는 품질 관측. 완료가 0건이거나 ABTF가 동반될 때만 WARN |
| 33 | `B_BUS_ERR_SNAPSHOT` | B | `BUS_ERR` 원시 경보와 같은 폴링 시점의 TWAI 상태·TEC/REC·Nag/AP/Hands-on/DAS 문맥 |
| 34 | `SUMMON_TX_SESSION` | A | 실제 Summoning 종료 시 세션의 완료·MLOA·ABTF·TX 오류 결과 |
| 35 | `SUMMON_RETRY_SESSION` | A | 실제 Summoning의 MLOA 단발 재시도 예약·완료·재MLOA·폐기 결과 |
| 36 | `SUMMON_TX_TIMING` | A | 실제 Summoning의 최대 연속 MLOA, 완료 송신 최대 공백, TSLLC 보류 횟수 |
| 37 | `SUMMON_POLICY_STATE` | A | 실제 Summon `ACA+SPR` 정책의 허용·종료 전이와 주·보조 기어 문맥 |
| 38 | `BOOT_RESET` | SYSTEM | 현재 reset reason, RTC 부팅 횟수와 이전 부팅 스냅샷 유효 여부 |
| 39 | `A_SAFETY_HOLD` | A | 첫 RX overrun 뒤 대기 TX 취소와 최소 2초 A TX 보류 시작 |
| 40 | `A_SAFETY_RECOVER` | A | EFLG 정상·최근 수신·새 프레임 100개 확인 뒤 A TX 재개 |
| 41 | `A_SAFETY_LATCH` | A | 60초 안에 두 번째 RX overrun이 발생해 해당 부팅 A TX 잠금 |
| 42 | `TSLLC_GATE_STATE` | A | TSLLC 설정·시작·AP·Summon·재승인·A 안전 차단 사유 전이 |

`ARB_LOST`는 다른 프레임에 우선권을 양보했다는 뜻이다. TEC/REC, BUS_ERR, TX_FAIL, BUS-OFF가 모두 0이면 이것만으로 물리 통신 오류로 판단하지 않는다.

`A_TX_FAILURE`는 최종 `TXERR` 또는 컨트롤러 오류만 기록한다. TXREQ가 남아 있는 즉시 결과는 진행 중으로 등록하고 완료 폴링에서 다시 확인한다. `MLOA`는 중재 손실, `ABTF`는 중단으로 별도 누적되며 `a_summon_tx_*`와 `a_tsllc_tx_*` 열에서 기능별 완료 결과를 비교할 수 있다. `a_tx_queued`와 기존 `a_summon_tx_ok`·`a_tsllc_tx_ok`는 하드웨어 큐 등록 결과이므로 실제 완료값과 구분해야 한다.

`A_TX_QUALITY`는 5초마다 기능별로 계산하며 관측 조건(시도 10건 이상이며 MLOA 50% 이상)을 만족할 때만 남긴다. MLOA가 높아도 완료 프레임이 있고 ABTF가 없으면 정상 중재 경쟁으로 `INFO`, 완료가 0건이거나 ABTF가 있으면 `WARN`이다. 같은 출처·심각도의 반복 관측은 30초 동안 `occurrences`로 합산하며 `detail_text`는 마지막 5초 구간 값이다. 이는 A채널 주입의 경쟁 상태를 관찰하기 위한 기록이며 CAN 주기·비트·송신 조건을 변경하지 않는다.

통합 로그의 B채널 `Try/Queue/SelfRx`는 각각 송신 시도, TWAI 송신 큐 등록 성공, 보드가 자신의 송신 프레임을 다시 수신해 확인한 횟수다. 일반 TWAI 모드에서 `SelfRx=0`만으로 송신 실패를 뜻하지 않는다. 실제 통신 오류는 `TxFail/TxFailed`, TEC/REC, `BUS_ERR`, BUS-OFF를 함께 확인한다.

`SUMMONING_STATE`는 단순히 주차 상태에서 Unlock bit를 주입한 것을 Summon 실행으로 단정하지 않는다. 1.3.17의 `ACA_SPR_1315` 정책은 ACA와 확인된 SelfParkRequest가 함께 유지될 때만 `START`를 기록한다. 주차나 AP 안정 상태의 제한 해제 주입은 `SUMMON_UNLOCK_ACTIVITY`로 따로 기록한다.

`SUMMON_POLICY_STATE`는 `policy=ACA_SPR_1315`, ACA/SPR와 `ALLOWED` 또는 `IDLE` 전이를 남긴다. 1.3.16에서 사용한 `0x257` 속도 검증은 A채널 수신 부하 때문에 비활성화했으며 `speed_validation=0`, `speed_raw_sna=4095`로 표시한다. AP 주행의 ECE R79 경로는 실제 Summoning 판단과 분리되어 기존 AP 안정 게이트를 사용한다.

`SUMMON_RETRY_SESSION`은 MCP2515 One-shot이 MLOA 뒤 자동 재전송하지 않는 점을 실제 Summoning에서만 보완한 결과다. 최신 mux 1을 3ms 뒤 한 번만 재시도하며 원본 수신 후 20ms가 지나거나 새 mux 1, 게이트 종료, 기능/A TX OFF, TX Guard, OTA 차단이 들어오면 폐기한다. `SUMMON_TX_TIMING`의 `max_success_gap_ms`가 재시도 적용 후 줄었는지 다음 실차 로그에서 비교한다. `tsllc_held`는 실제 Summoning 중 주행용 TSLLC mux 0을 보류한 횟수다.

`NAG_INJECTION_SESSION`은 첫 실제 Nag 송신에서 시작하고 AP 차단·Hands-on·기능 OFF처럼 주입 허용 상태를 벗어날 때 종료한다. Mode 2의 1.5초 내부 휴지는 `NAG_GATE_STATE gate=READY decision=MODE_PAUSE`로 표시하되 세션은 종료하지 않는다.

`NAG_GATE_STATE`는 Hands-on과 AP 차단이 짧게 왕복해도 이벤트 버퍼를 소진하지 않도록 30초 단위로 합산한다. `wall_time_first`는 첫 전이, `wall_time_last`와 `detail_text`는 마지막 전이, `occurrences`는 그 구간의 전이 횟수다. 실제 주입 시작·종료는 `NAG_INJECTION_SESSION`에서 독립적으로 확인한다.

`BUS_ERR`는 CAN 프로토콜 오류 누적값이며 `BUS-OFF` 진입 횟수가 아니다. `BUS_ERR`만 증가하고 TWAI가 RUNNING이며 TEC/REC가 정상으로 복귀했다면 BUS-OFF 전용 이력이 비어 있을 수 있다. 전용 이력은 실제 BUS-OFF 진입 뒤 복구 성공 또는 실패가 확정될 때 한 행씩 기록된다.

`B_BUS_ERR_SNAPSHOT`은 바로 앞의 `BUS_ERR` 행에 대한 보조 문맥이다. 당시 Nag가 켜져 있었는지, AP/Hands-on 판정이 무엇이었는지와 TEC/REC를 함께 보므로 단일 BUS_ERR를 Nag 주입 문제나 BUS-OFF로 성급하게 단정하지 않게 한다.

`RecoveryQuiet`는 BUS-OFF 복구 성공 직후 B채널 수신은 유지하면서 Nag TX만 3초간 정지하는 안정화 구간이다. `RecoveryQuiet=잔여ms Skip=누적횟수`가 표시되며, 잔여시간이 0이 되면 정상 주입 조건으로 복귀한다.

### 개별 A/B 시계열 CSV v6

`/api/timeseries.csv`는 5초 간격 A/B 상태를 한 행에 저장한다.

- `wall_time`과 `uptime_ms`를 함께 기록한다.
- `a_`, `b_`, `system_` 접두사로 채널과 공통 값을 구분한다.
- `capture_mode=AUTO`는 기본 최근 20분 버퍼, `MANUAL`은 사용자가 시작한 고정 구간이다.
- A 오버런 분석 핵심 열은 `a_d_rx_overrun`, `a_d_eflg_events`, `a_rx0_overrun`, `a_rx1_overrun`, `a_loop_gap_last_us`, `a_loop_gap_peak_us`, `a_d_loop_gap_over_2ms`다.
- `a_rx_buffer0_frames`/`a_rx_buffer1_frames`는 MCP2515 RXB0/RXB1에서 실제 회수한 누적 프레임 수다. 한쪽만 빠르게 증가하면 필터 부하 편중을 의심한다.
- `a_rx_drain_frames`는 RAM 큐로 선회수한 누적 프레임 수이고 `a_rx_drain_calls`는 빈 폴링을 제외한 실제 회수 배치 수다.
- `a_rx_queue_high_water`는 32프레임 RAM 큐의 부팅 이후 최대 사용량이며 `a_rx_queue_drops`는 큐 포화로 넣지 못한 프레임 수다. 정상 목표는 드롭 0이다.
- `a_loop_gap_over_250us`, `a_loop_gap_over_500us`, `a_loop_gap_over_1ms`, `a_loop_gap_over_2ms`는 부팅 이후 처리 공백 누적 분포다. `a_last_overrun_phase`는 마지막 오버런을 발견한 CAN 태스크 단계다.
- A 송신 안전 플래그는 `a_tx_enabled`, `a_summon_enabled`, `a_tsllc_enabled`, `a_one_shot_enabled`, `a_tx_guard_enabled`에 샘플 시점 값으로 저장한다.
- Summon 게이트는 `a_summon_gate_open`, `a_summon_gate_reason`, `a_summon_ap_state`, `a_summon_ap_active`, `a_summon_ap_stable_ms`, `a_summon_parked`, `a_summoning`으로 허용 결과와 당시 근거를 함께 저장한다.
- 시계열 CSV 스키마 7의 `a_summon_retry_*`, `a_summon_session_mloa_streak_max`, `a_summon_session_success_gap_max_ms`, `a_tsllc_summoning_hold`로 실제 Summoning 재시도와 TSLLC 보류 결과를 확인한다.
- CSV 뒤쪽의 `reset_reason`, `rtc_boot_count`, `previous_*`, `a_tsllc_block_reason`, `a_tsllc_rearm_required`, `a_safety_*` 열로 재부팅 직전 문맥과 TSLLC/A RX 보호 상태를 함께 비교한다.
- `a_summon_session_allowed/reason`, 주·보조 기어와 SelfParkRequest로 `ACA_SPR_1315` 세션 전이를 비교한다. 기존 CSV 열 호환을 위해 `a_summon_vehicle_speed_raw=4095`, `a_summon_age_599_ms=65535`를 SNA로 유지하지만 1.3.17 정책에는 사용하지 않는다.
- B Nag 플래그는 `b_nag_enabled`, `b_nag_mode`, `b_driver_state`에 저장한다.

수동 기록을 정지하면 이후 샘플을 추가하지 않는다. 수동 기록을 시작하지 않은 상태에서는 자동 최근 20분 버퍼가 계속 갱신된다.

---

## 8. 판정 코드 완전 해설

### `lastDecision` vs `intervalDecision`

- **`lastDecision`**: 마지막으로 수신된 ID 880 프레임 **하나**에 대해 내린 판정. 순간 상태를 반영.
- **`intervalDecision`**: 이번 5초 구간 전체를 대표하는 판정. 구간 중 가장 대표적인 결과를 반영.

### 판정 코드 상세

| 코드 | 이름 | 의미 | 발생 조건 |
|------|------|------|-----------|
| **0** | NONE | 아직 판정 없음 | 부팅 직후, 880이 한 번도 안 들어왔을 때 |
| **1** | ✅ ECHO | **에코 발사 성공** | 모든 조건 충족 → 에코 정상 주입 |
| **2** | OFF | 기능이 OFF 상태 | 웹 UI에서 Nag Killer를 껐을 때 |
| **3** | HANDS_ON | 핸들 잡고 있어 스킵 | 운전자가 실제로 핸들을 잡은 상태 |
| **4** | DAS_IDLE | DAS가 Nag를 요청하지 않음 | AP 상태값이 에코 조건 미충족 (예: 상태 1 = 정상 주행) |
| **5** | LATE_DROP | 타이밍 초과로 에코 포기 | 880 수신 후 처리가 너무 오래 걸려 에코 시점 놓침 |
| **6** | NO_880 | **ID 880 미수신** | 차량이 880을 보내지 않는 상태 (주차/시동 OFF/AP 비활) |
| **7** | NO_921 | DAS 상태 ID 미수신 | 921도 923도 수신되지 않아 AP 상태를 알 수 없음 |
| **8** | NO_ECHO | 에코 TX 실패 | 조건은 충족했지만 TWAI TX가 실패한 경우 |
| **9** | AP_BLOCK | AP 게이트 차단 | AP가 활성화되지 않은 상태 (Mode B의 AP 상태 3~6 조건 미충족) |

> **가장 많이 보이는 코드와 해석:**
>
> | 상황 | 예상 코드 |
> |------|-----------|
> | 차량 주차/시동 꺼져 있음 | 6 (NO_880) |
> | 차량 수동 운전 중 (AP 없음) | 9 (AP_BLOCK) 또는 4 (DAS_IDLE) |
> | AP 켜고 정상 주행 중 (Nag 없음) | 4 (DAS_IDLE) |
> | AP 켜고 핸들 경고 발생 (Nag!) → 에코 작동 | 1 (ECHO) |
> | 운전자가 핸들 잡음 | 3 (HANDS_ON) |
> | 기능 꺼놓음 | 2 (OFF) |

---

## 9. 로그 분석 실전 가이드

### 체크리스트: 로그를 받으면 이 순서로 확인한다

**① 헤더에서 기본 상태 확인**
```
Uptime이 너무 짧으면 (< 1분) → 재부팅 직후, 데이터 부족
dirty=1 → 개발 중 빌드, 재현 불가
```

**② [DBG] 줄에서 드라이버 상태 확인**
```
CanTaskOK:1 BDriverOK:1 TWAIerr:0/0  ← 이 세 개가 1/0/0인지
```
아니면 B채널이 작동하지 않는 것이다. 에코도 당연히 없다.

**③ [B-CH] 줄에서 880 수신 여부 확인**
```
880:N  → N이 5초마다 증가하고 있는가?
       → 증가 안 하면 차량이 880을 안 보내는 상태
```

**④ 시계열에서 `d880`과 `dEcho` 비교**
```
d880=500, dEcho=0  → 880은 오는데 에코가 안 나감 → 스킵 원인 확인
d880=500, dEcho=500 → 정상 에코 발사 중
d880=0, dEcho=0  → 880 자체가 안 옴 → NO_880 상태
```

**⑤ `intervalDecision` 코드 흐름 확인**
```
9→9→9→...→4→1→1→6→6  

9(AP_BLOCK) 반복 = AP 비활성 구간
4(DAS_IDLE) 반복 = AP는 켜져 있지만 Nag 요청 없음
1(ECHO) = Nag 발생 + 에코 주입 성공
6(NO_880) = 880 수신 중단
```

**⑥ BUS-OFF 여부 확인**
```
[2] BUS-OFF 이벤트 로그에 내용 있으면 복구 성공/실패 확인
시계열 busoff 컬럼이 1로 찍힌 구간 확인
```

### 정상 동작 시 기대값 요약

| 항목 | 정상값 |
|------|--------|
| TWAI-Code | 1 (RUNNING) |
| CanTaskOK / BDriverOK | 1 / 1 |
| TWAIerr | 0/0 |
| tec / rec | 0 / 0 |
| txFail | 0 |
| busoff | 0 |
| d297 (5초) | ~500 (100Hz) |
| d923 (5초) | ~10~11 (2Hz) |
| d880 (주행 중) | ~500 (100Hz) |
| intervalDecision (AP Nag 발생 시) | 1 (ECHO) |
| intervalDecision (AP 비활성 시) | 6 또는 9 |
| EFLG | 0x00 |

---

*문서 최종 업데이트: 2026-07-24*
*참고 소스: `include/can_helpers.h`, `include/handlers.h`, `include/can_diag.h`, `include/event_log.h`, `include/timeseries.h`*
