# 컨텍스트 노트

- 차량은 HW3이며 A채널은 Summon/TSLLC, B채널은 Nag Killer 전용이다.
- bit19는 ECE R79 해제, HW3 bit46은 Summon 제한 해제로 확정한다.
- AP 주행 중 ECE R79 주입을 유지해야 하므로 외부 프로젝트의 전역 Summon-only 정책은 사용하지 않는다.
- 실제 Summon 후보는 ACA와 확인된 SelfParkRequest 조합이며, 여기에 500ms 최근 수신 기어·속도·AP 검증을 추가한다.
- MCP2515 One-shot과 TX Guard, 실제 Summoning의 3ms·1회·20ms 제한 재시도는 유지한다.
