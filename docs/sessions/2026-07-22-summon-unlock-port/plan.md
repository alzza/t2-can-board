# HW3 Summon Unlock 포팅 계획

1. 복원한 2026-05-14 기준과 검증된 `summon_unlock.ino`를 보존한다.
2. INO 게이트 상태 머신과 감시 타이머를 A 채널 HW3 처리기에 포팅한다.
3. 활성화 상태이고 `Parked || Summoning`일 때만 HW3 bit 46으로 CAN 1021 mux 1을 주입한다.
4. 실험용 CAN 659 AutoLC 수신/송신 경로와 Web UI 제어를 제거한다.
5. 기존 Web UI/API에 INO 게이트 상태와 카운터를 표시한다.
6. 집중 회귀 테스트 후 형식 검사와 보드 빌드를 실행한다.
