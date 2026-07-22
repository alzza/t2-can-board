# 미사용 CAN 주입 실험 제거 계획

## 목표

- ID 1016 ULC/ALC, ID 1001 DAS 확인, ID 1021 Auto Turn 송신 경로를 제거한다.
- 관련 Web UI·API·런타임 상태·진단·테스트·NVS 사용을 제거한다.
- 부팅 시 퇴역 NVS 키를 삭제하되 Nag Killer·TSLLC·A 채널 안전 제어·수동 관찰·검증된 INO Summon 동작은 보존한다.

## 설계

비활성 호환 코드를 남기지 않고 실험 구현을 삭제한다. OTA 이후 과거 설정이 되살아나지 않도록 부팅 시 레거시 키만 정리한다. CAN 스케줄·드라이버 복구·TSLLC·Nag·Summon 게이트는 변경하지 않는다.

## 검증

1. 제거 심볼을 검색하고 명시한 레거시 NVS 키만 남았는지 확인한다.
2. Summon/TSLLC·Web UI·헬퍼·Nag 집중 테스트를 실행한다.
3. printf 검사와 LilyGo T2-CAN 보드 빌드를 실행한다.
