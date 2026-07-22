# 1.3.4 릴리스 문서·버전 갱신 계획

## 목표

- 오늘 반영된 HW3 조건부 Summon Unlock, A/B 채널 역할 분리, OTA/NVS fail-closed 동작을 공개 문서에 정확히 반영한다.
- 루트 버전, 펌웨어 헤더, 변경 이력을 `1.3.4`로 일치시킨다.
- 검증된 변경 전체를 하나의 릴리스 커밋으로 만들고 GitHub `origin`에 업로드한다.

## 선택한 방식

- 기존 영어 README와 docs-site 문서 형식을 유지한다.
- 기존 `ENHANCED_AUTOPILOT` 안내는 삭제하지 않고, HW3 T2-CAN에서는 Summon Unlock으로 대체되었음을 명시한다.
- 새 기능 설명은 코드에서 확인한 HW3 bit 46, bit 19, `Parked || Summoning`, A TX 마스터 조건만 적고 실차 미검증 동작은 주장하지 않는다.

## 완료 기준

1. `VERSION`, `include/version.h`, `CHANGELOG.md`가 `1.3.4`로 일치한다.
2. README와 docs-site Summon 문서가 현 HW3 제어 순서와 OTA 60초 확인 절차를 설명한다.
3. release metadata 검사, UI 동기화 검사, native 테스트, HW3 빌드, diff 검사가 통과한다.
4. 검증된 전체 변경이 하나의 릴리스 커밋으로 `origin`에 push된다.
