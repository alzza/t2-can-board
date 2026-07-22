# Summon Unlock UI 병합 체크리스트

- [x] 사용자 결정 7개 항목 확정.
- [x] INO 제어 API와 상태 필드 확인.
- [x] 현재 Summon/API/NVS/UI 연결 확인.
- [x] EAP 빌드·런타임·API·NVS 명칭 제거.
- [x] Summon Unlock 독립 변수와 기본 ON NVS 적용.
- [x] 기존 `enh_autopilot` NVS 값 무시·삭제 처리.
- [x] 메인 화면 Summon 요약 표시 검증.
- [x] 제어 화면 Summon 토글과 Gate 상세 표시 검증.
- [x] 진단 화면 INO 수신·송신 카운터 표시 검증.
- [x] A채널 Summon/TSLLC, B채널 Nag Killer 역할 문구 검증.
- [x] A채널 TX 마스터 유지 검증.
- [x] 관련 native 테스트 통과.
- [x] printf 포맷 검사 통과.
- [x] `git diff --check` 통과.
- [x] HW3 보드 빌드 통과.
- [x] 실차 확인 항목 기록.

## 실차 확인 대기

- [ ] 새 펌웨어 첫 부팅 시 Summon Unlock 기본 ON 확인.
- [ ] 기존 `enh_autopilot` 값과 무관하게 새 `summon_unlock` 설정이 저장되는지 확인.
- [ ] Park에서 Gate OPEN과 A채널 Summon TX 증가 확인.
- [ ] 주행 중 일반 상태에서 Gate BLOCKED와 차단 카운터 증가 확인.
- [ ] 실제 Summon 중 ACA·SPR·Summoning 전이와 Gate OPEN 확인.
- [ ] A TX 마스터 OFF 시 Summon `TX OFF` 표시와 TSLLC/Summon 송신 중단 확인.
- [ ] B채널 Nag Killer 상태·에코·BUS-OFF 진단이 기존과 동일한지 확인.
