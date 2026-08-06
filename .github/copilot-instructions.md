---
name: karpathy-guidelines
description: "코딩 시 흔히 발생하는 LLM 실수를 줄이기 위한 행동 원칙. 코드 작성·리뷰·리팩터링 시 항상 적용. 오버엔지니어링 방지, 외과적 변경, 가정 명시, 검증 가능한 성공 기준 정의를 다룬다. Andrej Karpathy의 관찰에서 도출."
---

# Karpathy 코딩 원칙

> Andrej Karpathy의 [LLM 코딩 함정 관찰](https://x.com/karpathy/status/2015883857489522876)에서 도출한 4가지 원칙.

**트레이드오프:** 이 원칙들은 속도보다 신중함을 우선한다. 단순 오타 수정처럼 사소한 작업은 계획·검증 절차를 생략하고 바로 고쳐도 된다.

---

## 모델 호환성

이 스킬은 VS Code Copilot에서 Claude, Codex, GPT 계열 모델을 선택해도 동일하게 적용되는 모델 중립 지침이다. 특정 모델명이나 전용 런타임을 전제로 새 해석 규칙을 만들지 말고, 현재 대화와 작업공간에서 제공된 도구와 지침만 기준으로 적용해라.

같은 원칙을 `AGENTS.md`로도 공유해야 하면 프로젝트 루트에 두어 저장소 전역에 적용하고, 하위 폴더에 두면 그 폴더와 그 아래 모든 하위 폴더에 재귀적으로 적용한다. 지침 파일은 길이보다 밀도를 우선하고, 같은 의미를 한국어와 영어로 반복하기보다 새 판단 기준만 추가해라.

---

## 1. 코딩 전에 생각하라

**가정하지 마라. 혼란을 숨기지 마라. 트레이드오프를 드러내라.**

구현 전:
- 가정을 명시적으로 밝혀라. 불확실하면 추측 대신 질문해라.
- 여러 해석이 가능하면 제시해라 — 침묵 속에서 선택하지 마라.
- 더 단순한 접근이 있으면 말해라. 근거가 있으면 반론을 제기해라.
- 무언가 불명확하면 멈춰라. 무엇이 혼란스러운지 이름 붙이고 물어봐라.

### 이 프로젝트에서 발생한 실제 사례
> 2026-04-18: NagTask 우선순위가 10→4로 바뀐 것을 발견했지만 "BUS-OFF 처리 로직 문제"로 오진.
> → **가정 명시 없이 수정을 진행한 결과, 6주간 오버엔지니어링.**

---

## 2. 단순함을 먼저

**문제를 해결하는 최소한의 코드. 추측성 코드 금지.**

우선순위가 충돌하면 이 순서로 결정해라.

1. 사용자가 요청한 동작을 깨지 않는 선택지만 남긴다.
2. 남은 선택지 중 가장 단순한 해법을 고른다.
3. 단순성이 같으면 변경 범위가 가장 작은 해법을 고른다.
4. 선택한 해법을 기존 코드베이스 스타일에 맞춰 표현한다 (단, 기존 스타일이 과한 복잡도를 유지하는 이유가 되어서는 안 된다).

요청 자체가 복잡도를 요구하면 그 복잡도만 허용하고 추측성 확장은 하지 마라.

- 요청된 것 이상의 기능 추가 금지.
- 단일 용도 코드에 추상화 금지.
- 요청하지 않은 "유연성"이나 "설정 가능성" 금지.
- 발생 불가능한 시나리오에 대한 에러 처리 금지.
- 같은 동작을 기존 스타일 안에서 200줄 대신 50줄로 명확히 표현할 수 있으면, 더 단순한 구현으로 줄여라.

테스트: "시니어 엔지니어가 이게 과하게 복잡하다고 할까?" — 그렇다면 단순화해라.

### 이 프로젝트에서 발생한 실제 사례
> TX 백오프 값을 조정하고, ESP-NOW를 추가하고, 이중 핸들러를 만들고, `processBChannelFrame()` 헬퍼를 추가하는 등
> 문제 원인(우선순위 4)을 모르는 채로 복잡도만 증가.
> → **단 한 줄(`priority 4 → 10`) 수정이 진짜 해법이었다.**

---

## 3. 외과적 변경

**건드려야 하는 것만 건드려라. 자신이 만든 쓰레기만 치워라.**

기존 코드 편집 시:
- 인접한 코드·주석·포매팅을 "개선"하지 마라.
- 깨지지 않은 것은 리팩터하지 마라.
- 선택한 해법의 표현 방식은 기존 스타일에 맞춰라.
- 관련 없는 데드 코드를 발견하면 언급만 해라 — 삭제하지 마라.

자신의 변경이 고아를 만들었을 때:
- **자신의** 변경으로 불필요해진 import·변수·함수는 제거해라.
- 기존에 이미 있던 데드 코드는 요청 없이 제거하지 마라.

테스트: 변경된 모든 줄이 사용자 요청과 직접 연결되어야 한다.

### 이 프로젝트에서 발생한 실제 사례
> `66a394f` 커밋: Web UI JS 중괄호 버그 수정 중 `src/main.cpp`의 NagTask 우선순위가
> 사이드 이펙트로 변경됨 — 이것이 모든 문제의 시작.
> → **무관한 파일을 건드린 결과.**

---

## 4. 목표 중심 실행

**성공 기준을 정의해라. 검증될 때까지 루프해라.**

명령형 작업을 검증 가능한 목표로 변환:
- "검증 추가" → "잘못된 입력에 대한 테스트 작성 후 통과"
- "버그 수정" → "버그를 재현하는 테스트 작성 후 통과"
- "X 리팩터" → "전후 테스트 모두 통과 보장"

다단계 작업엔 간략한 계획을 먼저 제시:
```
1. [단계] → 검증: [확인 방법]
2. [단계] → 검증: [확인 방법]
3. [단계] → 검증: [확인 방법]
```

강한 성공 기준은 독립적인 루프를 가능하게 한다.
약한 기준("되게 해줘")은 끊임없는 재확인을 요구한다.

### 이 프로젝트 성공 기준 예시

```
BUS-OFF 수정 성공 기준:
  - pio run -e lilygo_t2can → BUILD SUCCESS
  - 차량 연결 10분 후 busoff_count < 2
  - B채널 frameHz ~100Hz 안정 유지
  - Web UI /api/status 응답 정상

기능 추가 성공 기준:
  - 빌드 성공
  - pio test -e native 통과
  - 웹 UI 토글 ON/OFF 후 /api/status 값 반영 확인
```

---

## 5. 편집 전 워크스페이스 근거 확인

**수정할 파일을 실제로 읽고 나서 편집해라. 기억, 요약, 열린 탭만 믿지 마라.**

코드를 바꾸기 전:
- `rg` 같은 검색 도구로 실제 구현 위치를 찾아라.
- 수정할 파일 본문과 가까운 호출부를 직접 읽어라.
- 열린 탭, 파일명, README, 이전 대화 요약은 힌트로만 취급해라.
- 로컬 코드가 자신의 가정과 다르면 가정이 아니라 코드를 기준으로 계획을 고쳐라.

이 규칙은 사소한 변경 예외 조항이 아니다. 존재하지 않는 코드를 상상하고 자신 있게 고치는 실수를 막기 위한 기본 규칙이다.

---

## 6. 작업 트리를 존중하라

**커밋되지 않은 변경은 기본적으로 사용자의 작업이라고 가정해라.**

아래 제약은 git 워크트리에만 적용된다.

워크트리가 더럽다면:
- 관련 없는 변경을 되돌리거나 덮어쓰거나 전면 포매팅하지 마라.
- 같은 파일에 사용자 변경이 있으면 먼저 읽고 그 위에 맞춰라.
- 관련 없는 더티 파일은 무시해라.
- 사용자가 명시적으로 요청하지 않은 파괴적 git 명령은 실행하지 마라.

---

## 7. 한국어 문장 끝에 콜론 쓰지 마라

**한국어 문장은 가능하면 마침표, 물음표, 느낌표로 끝내라.**

사용자가 한국어로 쓰면 출력도 한국어다.
- 다음 줄에 목록이나 예시가 오더라도 한국어 문장을 `:`로 끝내지 마라.
- 영어 문서 습관이 한국어 문장 끝으로 새지 않게 잡아라.
- 테스트 기준은 한국어 문장 종결이 `.`, `?`, `!` 중 하나인지 보는 것이다.
- 콜론은 코드, 키-값, 타임스탬프, 라벨 내부에서는 써도 된다.

---

## 8. 새 소스 파일 첫 줄에는 한 줄 역할 주석을 둬라

**새 소스 파일의 첫 줄에는 파일 역할을 설명하는 한 줄 한국어 주석을 둬라.**

새 소스 파일을 만들 때:
- TypeScript/JavaScript: `// 사용자 인증 상태를 관리하는 Context Provider`
- Python: `# KIS API 호출을 비동기로 래핑하는 클라이언트`
- SQL: `-- 일별 집계 결과를 저장하는 머티리얼라이즈드 뷰`
- `'use client'`, `'use server'`, shebang 같은 필수 지시문 바로 아래에 둬라.
- `*.config.ts`, `package.json`, lockfile, 생성 파일에는 생략해라.

에이전트는 파일을 부분적으로 읽는 경우가 많다. 한 줄 헤더는 다음 세션이 전체 파일을 다시 훑지 않고도 맥락을 잡게 해준다.

---

## 9. 1시간 이상 또는 3개 이상 파일 작업 전에는 계획, 체크리스트, 컨텍스트 노트를 만든다

**예상 작업 시간이 1시간 이상이거나, 3개 이상의 파일을 수정하거나, CAN/TWAI 동작처럼 실차 검증에 영향을 주는 작업이면 세 가지 산출물을 먼저 정리해라.**

- **Plan** — 무엇을 왜 만드는지.
- **Checklist** (`checklist.md`) — 체크박스로 추적할 구체 작업.
- **Context Notes** (`context-notes.md`) — 작업 중 결정과 근거를 계속 기록하는 메모.

위 기준(1시간 이상, 3개 이상 파일, 실차 검증 영향)을 충족하는 작업이면, 사용자가 바로 코딩을 원하더라도 체크리스트와 컨텍스트 노트를 먼저 만든 뒤 코딩을 시작해라. 다음 세션이 처음부터 다시 추론하지 않게 만드는 장치다.

---

## 10. 완료 전에 관련 검증을 먼저 실행해라

**코드를 건드렸다면 완료 선언 전에 가장 관련 있는 검증부터 실행해라.**

- `npm test`, `pytest`, `cargo test`처럼 프로젝트가 쓰는 검증을 가장 좁은 범위부터 실행해라.
- 영향 범위가 크면 그 다음에 더 넓은 빌드나 테스트를 실행해라.
- 테스트가 실패하면 실제 에러를 읽고 고친 뒤 다시 실행해라.
- 테스트 체계가 없으면 최소한 빌드, 컴파일, 타입체크 같은 대체 검증을 실행해라.
- 검증을 실행할 수 없으면 왜 못 했는지 정확히 적어라.

이 단계는 코딩 에이전트가 가장 자주 빼먹는다. 협상 가능한 권고가 아니라 기본 절차로 취급해라.

---

## 11. 최종 응답에는 실제 검증 근거만 적어라

**검증했다고 말할 때는 의도가 아니라 실제 실행 결과를 적어라.**

최종 응답에는 가능하면 아래를 포함해라.
- 실행한 명령이나 확인 절차.
- 결과가 통과, 실패, 미실행 중 무엇이었는지.
- 아직 남은 리스크가 있다면 무엇인지.

구체 검증 없이 `고쳤다`, `된다`, `완료됐다` 같은 표현을 단정적으로 쓰지 마라.

---

## 12. 의미 단위로 커밋하라

**하나의 논리적 변경이 끝났다면 그 단위로 커밋해라.**

- 매 수정마다 커밋하지 마라. 기능이나 버그 수정의 논리 단위가 완성되고 관련 검증이 통과했을 때 커밋 후보로 본다.
- 보편적인 기준은 "작업 중 저장"이 아니라 "되돌릴 수 있는 완성 단위"다.
- 한 문장으로 설명 가능한 변경이면 한 커밋 후보다.
- 설명이 길어지면 변경이 섞인 것이니 분리해라.
- 관련 없는 수정이 쌓여 롤백 단위를 잃지 않게 해라.
- 커밋은 사용자가 명시적으로 요청하거나 승인했을 때만 수행한다. 사용자가 수동 커밋을 선호하는 흐름이면 에이전트는 커밋하지 말고 변경 단위와 검증 결과만 정리한다.
- 커밋이 금지된 환경이거나 사용자가 원치 않으면 커밋하지 말고, 대신 변경 단위를 분명히 요약해라.

프로토타입이나 일회성 스크립트에서는 지나친 의식보다 되돌리기 쉬운 단위를 우선해라.

---

## 13. 테스트용 펌웨어 bin 이름 규칙

**펌웨어 테스트 후 OTA/실차 테스트용 bin을 만들 때는 날짜와 변경 요약이 파일명에 남아야 한다.**

- 산출물 이름은 `버전_YY-MM-DD_짧은변경요약.bin` 형식을 사용한다.
- 버전은 `VERSION`, 날짜는 빌드·테스트한 당일 날짜를 KST 기준 2자리 연·월·일로 쓴다.
- 변경 요약은 오늘 수정한 핵심 내용을 최대한 압축해 적고, 공백 대신 `-` 또는 `_`를 쓴다.
- 예시는 `1.3.8_26-08-07_summon-txdiag.bin`, `1.3.8_26-08-07_ota-tx-safe.bin`이다.
- `include/version.h`의 `FIRMWARE_ARTIFACT_NOTE`을 당일 핵심 변경 요약으로 갱신한다. `lilygo_t2can` 빌드 성공 시 스크립트가 원본 `.pio/build/lilygo_t2can/firmware.bin`을 유지한 채 프로젝트 루트에 위 이름의 복사본을 자동 생성한다.
- 최종 응답에는 생성한 bin 파일 경로와 어떤 변경분을 담은 빌드인지 함께 적는다.

---

## 14. OTA 실차 안전 규칙

**OTA 관련 코드를 수정하거나 OTA용 펌웨어를 만들 때는 검증된 TX 정지 순서를 깨지 마라.**

- OTA 업로드 시작 전 `prepareOtaUploadCanQuiet()` 흐름을 유지한다.
- 먼저 `canTxQuiesceBegin()`으로 새 A/B 수정 송신을 원자적으로 막는다.
- 기능 NVS는 A TX, Summon Unlock, TSLLC, Nag Killer OFF/stock 안전값으로 저장한다.
- 이미 시작된 송신 호출은 최대 250ms 안에 종료 확인하고, 실패하면 업로드를 시작하지 않는다.
- 그 다음 CAN-A MCP2515는 Listen-Only, CAN-B TWAI는 Stopped로 물리 TX를 차단한다.
- OTA 시작·수신·기록·검증 실패 시 CAN TX를 재개하지 않고 재부팅 전까지 fail-closed로 둔다.
- 새 펌웨어 첫 부팅에서는 CAN 초기화 전에 기능 OFF 안전값을 다시 적용한다.
- 사용자가 OTA 전에 Web UI에서 A TX와 Nag Killer를 먼저 OFF하는 절차는 계속 권장한다. 자동 차단은 이중 보호이지 차량 CAN 오류를 되돌리는 기능이 아니다.
- 2026-08-01 실차 OTA에서는 이 순서 적용 후 차량 통신에러가 발생하지 않았다. 이 성공 사례를 근거로 OTA 관련 변경 시 위 순서를 회귀 검증 기준으로 삼는다.

---

## 15. 에러를 읽고 추측하지 마라

**실패했을 때는 기억 속 패턴이 아니라 실제 에러와 로그를 기준으로 움직여라.**

문제가 생기면:
- 전체 에러 메시지와 스택 트레이스를 읽어라.
- 기대했던 출력이 아니라 실제 로그를 확인해라.
- 원인을 확인하기 전 흔한 해결책부터 던지지 마라.
- 불명확하면 print나 log를 추가해 상태를 검증한 뒤 고쳐라.

이 단계는 테스트 실행 다음으로 자주 생략된다. 에러 키워드만 보고 최근 패턴으로 수리하면 한 줄짜리 버그가 세 파일짜리 리팩터로 커진다.

---

## 이 원칙이 작동하고 있다는 신호

- diff에 불필요한 변경이 없다 — 요청된 변경만 나타난다.
- 오버엔지니어링으로 인한 재작업이 없다 — 처음부터 코드가 단순하다.
- 구현 전에 명확화 질문이 나온다 — 실수 후가 아니라.
- 실제 코드, 로그, 워크트리를 확인한 뒤에만 수정이 진행된다.
- 최종 응답에 검증 명령과 결과가 구체적으로 남는다.



### ESP32 CAN(TWAI) 레퍼런스 (필수 참조)

> **⚠️ MANDATORY**: ESP32 CAN/TWAI 관련 코드를 작성·수정·검토할 때는
> 반드시 아래 공식 문서를 먼저 확인해야 한다.
> 현재 환경: Arduino ESP32 2.0.17 = **ESP-IDF v4.4.x** (v3.3/v5.x 문서와 다름)

**필수 레퍼런스**:
- **[ESP-IDF v4.4.8 ESP32-S3 Programming Guide (Index)](https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32s3/index.html)**
  → TWAI 작업 전 버전 기준, 드라이버 동작 전제, API 탐색 시작점으로 먼저 확인
- **[ESP-IDF v4.4 TWAI API (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32s3/api-reference/peripherals/twai.html)**
   
**확인 필수 항목**:
1. State Machine 준수: `Stopped → Running → Bus-Off → Recovering → Stopped`
2. `twai_message_t.ss` 플래그 — Single Shot TX (충돌 시 재전송 금지, TEC 억제)
3. `twai_message_t.extd` 플래그 — 11bit vs 29bit ID 구분
4. BUS-OFF 복구 절차: `twai_initiate_recovery()` 후 Recovering → Stopped → `twai_start()`
5. Alert 시스템: `TWAI_ALERT_BUS_OFF`, `TWAI_ALERT_ERR_PASS`, `TWAI_ALERT_TX_FAILED` 등

**금지 사항**:
- v3.3 `can_` 구 API 참조 금지 (deprecated, 현재 환경에서 존재하지 않음)
- 임의 ESP32 예제 복붙 금지 (버전 불일치로 `msg.ss` 누락, state 오류 발생 이력 있음)

### TWAI 안전 체크리스트 (수정 전 항상 확인)

> 이 항목들은 2026-04-18 사고(우선순위 실수 변경)에서 도출된 교훈이다.

```bash
# NagTask 우선순위 → 항상 10
grep "NagTask.*nullptr" src/main.cpp

# TX 백오프 로직 → 없어야 함
grep "setTxBackoff" src/main.cpp

# 빌드
pio run -e lilygo_t2can
```
### Available Skills & Context Utilization
질문에 아래와 관련된 도메인이 포함되어 있다면, 반드시 해당 경로의 `SKILL.md` 파일을 먼저 읽고 그 지침에 따라 답변하세요.

- **Coding Principles:** `.github/skills/karpathy-guidelines/SKILL.md` (오버엔지니어링 금지, 최소한의 외과적 변경 원칙 준수)
- **Workflow Methodology:** `.github/skills/using-superpowers/SKILL.md` (설계→계획→구현→검증 흐름 강제, 비사소한 작업 품질 일관화)
### Usage Rule
1. 사용자가 CAN 통신이나 특정 안정화 로직을 물으면, 위 Skills 디렉토리의 컨텍스트를 최우선으로 반영한다.
2. 코드를 수정할 때는 'karpathy-guidelines'를 참조하여 기존 코드를 최대한 보존하며 필요한 부분만 수정한다.
3. 사용자가 방법론 적용이나 큰 범위 작업을 요청하면 'using-superpowers'를 먼저 적용해 목표/설계/계획을 명시한 뒤 구현한다.
4. 사용자가 명시적으로 `A채널 동작완벽함!` 또는 `B채널 동작완벽함`이라고 선언하기 전까지, 진단/가이드/테스트 우선순위는 A채널(주행/정차 통신 테스트)을 1순위로 유지한다.

### T-CAN 신호 상세 자동 참조 규칙

CAN signal 이름, frame ID, start bit, length, endian, scale, offset, enum, bit layout이 필요한 코드 작성·검토·문서화 작업에서는 추측하지 말고 먼저 `scripts/tcan_signal_detail.py`를 사용한다. T-CAN Explorer의 공개 JSON을 기준으로 frame metadata, source presence, `Visual bit map`, `Bit layout table`, `Bit positions`, DBC-style `SG_` line, raw extraction pseudocode를 확인한 뒤 구현한다.

기본 사용 예시다.

```bash
.venv/bin/python scripts/tcan_signal_detail.py DAS_autopilotState DAS_autopilotHandsOnState --platform ModelY --bus VEH,CH --format markdown --output docs/tcan_signal_reference_ModelY_VEH_CH.md
```

코드 생성이나 검증에 구조화된 값이 필요하면 JSON 출력으로 확인한다.

```bash
.venv/bin/python scripts/tcan_signal_detail.py EPAS3P_torsionBarTorque EPAS3P_handsOnLevel SCCM_steeringAngle --platform ModelY --bus VEH,CH --format json
```

비트 레이아웃을 눈으로 비교해야 하면 HTML 출력도 함께 생성한다.

```bash
.venv/bin/python scripts/tcan_signal_detail.py DAS_autopilotState DAS_autopilotHandsOnState EPAS3P_torsionBarTorque EPAS3P_handsOnLevel SCCM_steeringAngle --platform ModelY --bus VEH,CH --format html --output docs/tcan_signal_reference_ModelY_VEH_CH.html
```

- 기본값은 `--platform ModelY --bus VEH,CH`다.
- bus/view 조건이 다르면 같은 signal 이름도 frame ID가 달라질 수 있으므로 `matchedFrameSources`, `matchedSignalSources`, T-CAN URL을 함께 기록한다.
- 펌웨어 코드에 반영할 때는 출력의 `Byte order`, `Visual bit map`, `Bit layout table`, `Raw extraction pseudocode`, `Physical formula`를 우선 근거로 삼는다.

### 세션 로그 기록 규칙

작업이 끝나면 다음 세션이 같은 추론을 반복하지 않도록 `docs/sessions/chat_log_YYYY-MM-DD.md`에 기록한다.

- 해당 날짜 파일이 있으면 이어 붙이고, 없으면 새로 만든다.
- 사소한 질의응답이나 파일을 건드리지 않은 단순 설명은 기록하지 않아도 된다.
- 코드, 설정, 진단 로직, 실차 로그 해석, OTA, CAN/TWAI 동작 판단처럼 다음 작업에 영향을 주는 내용은 기록한다.
- 기능이 완성된 경우에는 무엇을 바꿨는지보다 성공 기준과 실제 검증 결과를 우선 기록한다.
- 기능이 미완성인 경우에는 현재까지 확인한 사실, 남은 의문, 다음 액션을 명확히 남긴다.
- 사용자가 수동 커밋을 선호하는 경우, 세션 로그에는 커밋을 만들었다고 쓰지 말고 커밋 후보 단위와 검증 결과만 적는다.
- `checklist.md`, `context-notes.md`, `plan.md`, `chat_log_YYYY-MM-DD.md`는 기본적으로 한글로 작성한다.
- 루트 `README.md`, `CHANGELOG.md`와 docs-site의 공개 기능 안내·변경 이력도 새 작성 및 갱신은 한글로 작성한다. 코드 식별자, 명령어, API 경로, 표준 고유명사는 원문을 유지한다.

권장 포맷은 아래 순서를 따른다.

```markdown
## YYYY-MM-DD HH:MM KST - 짧은 작업 제목

사용자 요청:
- 사용자가 원한 동작이나 질문을 1~3줄로 요약한다.

확인:
- 실제 코드, 로그, DBC, 문서에서 확인한 사실을 적는다.
- 추정은 사실과 분리해 적는다.

수정:
- 변경한 파일과 핵심 동작만 적는다.
- 관련 없는 리팩터링이나 배경 설명은 길게 쓰지 않는다.

검증:
- 실제 실행한 명령과 결과를 적는다.
- 실패하거나 미실행한 검증은 이유를 적는다.

다음 액션:
- 이어서 볼 실차 확인 포인트나 코드 작업을 적는다.
- 완전히 끝난 작업이면 생략할 수 있다.
```

---

### 유틸리티 스크립트

#### Web UI 원본 (`/web/web_ui.html`)

- 일반 대시보드 UI는 `include/web/web_ui.h`의 raw literal을 직접 편집하지 말고 `web/web_ui.html`만 편집한다.
- 변경 후 `python3 scripts/sync_web_ui.py --sync`로 펌웨어 임베디드 헤더를 갱신한다.
- 검증 시 `python3 scripts/sync_web_ui.py --check`를 실행한다.
- 복구모드 UI인 `WEB_RECOVERY_UI_HTML`은 계속 `include/web/web_ui.h`에서 관리하며 요청 범위가 아니면 수정하지 않는다.
- 로컬 mock 서버는 HTML 원본을 직접 읽으므로 UI 수정 후 헤더 전체를 다시 읽지 않고 바로 확인할 수 있다.

#### C 주석 박스 정렬 생성기 (`/scripts/gen_box2.py`)

C/C++ 주석 내 박스 다이어그램 (`┌─ ... ─┐ / │ ... │ / └─...─┘`)을 **유니코드 동아시아 문자 너비를 고려해** 오른쪽 `│`가 정확히 열 맞춤되도록 생성한다.

**한글 포함 주석에서 열 맞춤이 어긋날 때** 이 스크립트를 사용한다.

```python
# /scripts/gen_box2.py 핵심 구조
import unicodedata

def vw(s):  # 시각적 너비: 한글·전각 = 2, 그 외 = 1
    w = 0
    for c in s:
        eaw = unicodedata.east_asian_width(c)
        w += 2 if eaw in ('W', 'F') else 1
    return w

INNER = 72  # 박스 내부 너비 (내용 최대 vw 기준으로 여유 2 추가)

def box_line(content):
    pad = INNER - vw(content)
    return f" *  \u2502{content}{' ' * pad}\u2502"

def box_header(title):
    dashes = '\u2500' * (INNER - 1 - vw(title))
    return f" *  \u250c\u2500{title}{dashes}\u2510"

def box_footer():
    return f" *  \u2514{'\u2500' * INNER}\u2518"
```

**사용 절차**:
1. `lines_c1`, `lines_c0` 리스트에 내용 줄 편집 (유니코드 이스케이프 또는 직접 한글)
2. `max_w = max(vw(l) for l in all_lines)` 로 최대 너비 확인
3. `INNER = max_w + 2` 로 여유 설정
4. `python3 /scripts/gen_box2.py --file` → `/tmp/box_out.txt` 생성
5. `scripts/gen_box2.py` 로 `src/main.cpp` 내 해당 섹션 교체

**주의**: `heredoc << 'EOF'` 방식은 한글+유니코드 혼용 시 터미널에서 깨짐 → 반드시 `.py` 파일로 저장 후 `python3 파일.py` 실행.
