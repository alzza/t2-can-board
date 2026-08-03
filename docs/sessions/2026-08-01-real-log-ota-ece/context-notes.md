# 2026-08-01 실차 로그 OTA·ECE R79 검증 컨텍스트 노트

## 입력 로그

- `/Users/akanus/Downloads/canmod_20260801_193620.txt`
- `/Users/akanus/Downloads/can_events 6.csv`
- `/Users/akanus/Downloads/can_timeseries_ab 6.csv`

## 확인한 사실

- 시계열 CSV는 schema_version 3이고 2026-08-01 19:16:33.041부터 19:36:28.191까지 240행이다.
- 이벤트 CSV는 2026-08-01 19:13:07.973부터 19:36:27.450까지 256행이다.
- 이벤트 버퍼는 `records=256`, `overwritten=241` 상태라 OTA 시작 직후의 자세한 이벤트는 남아 있지 않을 수 있다.
- 이벤트 로그에는 19:13:07~19:14:27 사이 `AP_ACTIVE=1`, `SUMMON_GATE=1`, `SUMMON_TX=1`, `TSLLC_TX=1` 조합이 반복된다.
- 시계열 CSV는 19:16:33부터 시작해 AP state 3~6 구간이 남아 있지 않고, `a_summon_ap_state`는 1~2만 보인다.
- 현재 구현에서 Summon Unlock 송신은 ID `0x3FD` mux 1에서 bit19=0, bit46=1을 같은 프레임에 적용한다. 따라서 `SUMMON_TX=1`은 ECE R79 해제 bit19도 함께 송신됐다는 의미다.
- A채널은 BUS-OFF=0, TEC=0, REC=0으로 차량 통신에러 급의 물리 오류는 보이지 않는다.
- B채널은 BUS-OFF=0, TEC=0, REC=0, TX Fail=0으로 안정적이다.
- A채널은 RX_OVERRUN이 지속된다. 시계열 기준 `a_rx_overrun`은 300에서 680까지 증가했고, `a_d_rx_overrun` 합계는 383이다.
- A채널 TX hard error는 40에서 59까지 증가했다. Summon TX fail은 12에서 16, TSLLC TX fail은 28에서 43까지 증가했다.
- A채널 EFLG는 0x80 RX_OVERRUN 위주이며 TXBO, TXEP, RXEP, TXWAR, RXWAR, EWARN은 표시되지 않았다.

## OTA 무통신에러 성공 규칙

- 이번 실차 OTA에서 차량 통신에러가 발생하지 않았다는 사용자 관찰을 성공 사례로 기록한다.
- 유지해야 할 순서는 `새 송신 차단 → 기능 NVS OFF 저장 → 기존 송신 종료 확인 → A Listen-Only/B Stopped → OTA 실패 시 fail-closed → 첫 부팅 기능 OFF`다.
- 사용자가 OTA 전에 A TX와 Nag Killer를 먼저 OFF하는 절차는 계속 권장한다.

## 남은 의문

- AP 중 차선 변경 개선은 이벤트 로그로는 확인되지만, 시계열 CSV의 AP 구간이 잘려 있어 AP 안정 시간과 송신 변화량을 5초 단위로 완전히 재구성할 수 없다.
- A채널 RX 오버런은 차량 에러로 이어지지는 않았지만 빈도가 높다. 다음 작업에서 버퍼 처리량, A poll 우선순위, 로그 저장 중 부하, MCP2515 RX1 사용 패턴을 다시 검토할 가치가 있다.
