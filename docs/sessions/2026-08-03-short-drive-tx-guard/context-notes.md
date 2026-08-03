# 컨텍스트 노트

## 실차 로그 근거

- 기록 구간: 2026-08-03 15:23:55~15:29:30, 약 5분 35초.
- 실제 주행은 15:25:05~15:26:30, 약 85초. AP 상태 3~6 구간은 없었다.
- A/B BUS-OFF, TEC, REC, A MERRF, B Bus Error·TX Fail·RX Missed는 모두 0이다.
- A RX0 오버런은 부팅 직후 1회만 발생하고 재발하지 않았다. RAM 큐 드롭은 0이다.
- A TX_FAIL은 8건이며 각각 17~72초 간격의 단발이다. 기존 임계값 1은 매번 15초 Guard를 시작해 Guard 활성 표본이 24/68이 됐다.
- 해당 8건 중 TEC·REC·MERRF·현재 EFLG 상승은 없었다. 따라서 TX_FAIL 단독 조건은 1초 2회 이상일 때만 Guard하도록 조정한다.

## 유지한 안전 조건

- EFLG(TXBO/TXEP/TXWAR) 또는 TEC 24 이상이면 횟수와 무관하게 즉시 Guard한다.
- Guard 지속 시간 15초, MCP2515 One-Shot ON, TX Guard ON 기본값을 유지한다.
- Summon HW3 bit19/bit46, TSLLC, Nag 로직은 변경하지 않는다.

## 다음 검증

- AP 상태 3~6에서 ECE R79와 Nag AP 전용 동작을 포함해 10~15분 실차 로그를 수집한다.
- USER_MARK를 AP 시작·종료 또는 경고 시점에 한 쌍으로 남긴다.
