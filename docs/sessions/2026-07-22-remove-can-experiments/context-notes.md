# 컨텍스트 노트

- 범위는 사용자와 확인한 미사용 CAN 송신 실험 세트(ID 1016 ULC/ALC, ID 1001 DAS 확인, ID 1021 Auto Turn)다.
- A 채널 TX 마스터·MCP One-Shot·TX Guard·오류 진단·수동 신호 관찰은 안전/진단 기능이므로 유지한다.
- 검증된 Summon SPR 게이트에는 ID 1016 수신이 필요하므로 수신만 유지한다.
- 검증된 INO 동작은 변경하지 않는다. 새 NVS 기본값은 Summon 활성, 부팅/감시시간 초과 시 parked=true, `Parked || Summoning`일 때만 HW3 bit46을 주입한다.
- CAN 스케줄·필터·TWAI·MCP2515 복구 변경은 범위에 포함하지 않는다.

## 처리 결과

- 세 실험의 송신 로직·상태·Web UI/API·진단·로그·비상 백업/복구를 제거했다.
- ID 1016은 Summon SPR 게이트만 갱신하며 송신하지 않는다.
- ID 1001은 처리하지 않으며 기본 관찰 필터에서도 제거했다.
- ID 1021은 TSLLC mux 0 또는 조건부 Summon mux 1로만 송신한다. 짧은 프레임은 비트 접근 전에 거부한다.
- 관찰 탭은 읽기 전용으로 유지하고, 일반 MCP2515 필터는 INO와 같은 다섯 ID로 맞췄다.
- 부팅 시 다음 퇴역 NVS 키를 삭제한다: `ulc_stalk`, `alc_offhwy`, `ulc_offhwy`, `ulc_speed`, `ulc_blind`, `auto_lc`, `bk_ui_ulc`, `bk_alc_off`.
- INO Summon 기본값과 게이트 동작은 변경하지 않았다.

## 검증

- 집중 native 테스트: 56/56 통과
- Nag Killer 테스트: 43/43 통과
- printf 형식 검사: 33개 파일 통과
- `git diff --check`: 통과
- 보드 빌드: 통과 (RAM 32.6%, Flash 49.7%)
- 펌웨어 SHA-256: `5b08a49dc410c44f9f0fed76061dbcfa5c03d4c41b1dfdaf295a0cdd53d7ca4e`
