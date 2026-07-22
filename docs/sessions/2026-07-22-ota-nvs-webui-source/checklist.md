# 체크리스트 - OTA/NVS 안전성 재검토와 Web UI 원본 분리

- [x] 2026-05-13 OTA/TX/NVS 기록 확인.
- [x] 2026-05-14 기준 코드와 현재 OTA 코드 비교.
- [x] OTA 업로드 전 A/B 기능 및 A TX NVS OFF 저장 확인.
- [x] OTA 첫 부팅 설정 로드 순서 확인.
- [x] Web UI HTML 원본 저장.
- [x] HTML ↔ 임베디드 헤더 동기화 검사 도구 추가.
- [x] 로컬 미리보기 서버가 HTML 원본을 직접 사용하도록 변경.
- [x] Web UI 회귀 테스트 및 펌웨어 빌드 통과.
- [x] OTA/NVS 위험도와 실차 업데이트 순서 기록.
- [x] B채널 부팅 초기값 레이스 보완 여부 사용자 확인.

## 승인 후 구현

- [x] 사용자 승인 확인.
- [x] 정상·OTA·롤백·복구·비정상 상태표 작성.
- [x] OTA 상태 정책을 native 테스트 가능한 순수 함수로 분리.
- [x] 차량 영향 NVS 설정을 CAN 시작 전에 원자적으로 선로드.
- [x] `pending=1` 현재 펌웨어 안전값 재기록.
- [x] NVS/rollback 실패 시 복구 UI fail-closed.
- [x] OTA metadata 저장 실패 시 boot 파티션 복원.
- [x] 복구 UI에 CAN 차단 사유 표시.
- [x] 관련 native 테스트와 전체 빌드 검증.
- [x] 실차 OTA 순서와 확인 항목 기록.
