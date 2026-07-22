# 계획 - OTA/NVS 안전성 재검토와 Web UI 원본 분리

## 목표

- 2026-05-14 차량 동작 펌웨어에서 현재 펌웨어로 OTA할 때 CAN TX가 안전하게 정지되는지 확인한다.
- OTA 첫 부팅의 NVS 보존·기본값·삭제 키 순서를 검토해 차량 오류 가능성을 판정한다.
- 현재 Web UI를 독립 HTML 원본으로 저장해 이후 UI 작업 범위를 줄인다.

## 범위

1. OTA 업로드 직전 안전값 저장과 B채널 TX 큐 정리 확인.
2. `ota_pending` 상태 전이와 CAN 드라이버/태스크 시작 순서 확인.
3. 구 펌웨어 NVS 키와 신규 `summon_unlock` 키의 마이그레이션 확인.
4. `web/web_ui.html`을 편집 원본으로 만들고 임베디드 헤더 동기화 도구 추가.
5. 동기화 검사, Web UI 회귀 테스트, 전체 펌웨어 빌드 검증.

## 변경 제한

- 이번 단계에서는 검토 결과만으로 OTA/CAN 런타임 동작을 임의 변경하지 않는다.
- OTA 안전성 보완이 필요하면 위험과 수정안을 먼저 사용자에게 제시한다.
- 복구모드 UI와 OTA 파티션/롤백 상태 머신은 요청 없이 변경하지 않는다.

## 사용자 승인 후 보완 범위

- 사용자가 CAN 시작 전 NVS 선로드, OTA 첫 부팅 안전값 재기록, NVS 실패 시 복구 UI fail-closed를 모두 승인했다.
- 포팅·병합으로 신규 키가 생기거나 기본값이 바뀌어도 OTA 첫 부팅은 현재 펌웨어가 아는 모든 차량 기능을 OFF/stock으로 다시 저장한다.
- OTA 상태 `0~5`, 범위 밖 값, USB 플래시 파티션 불일치, fallback 부재, NVS init/open/read/write/commit 실패를 각각 명시적으로 처리한다.
- OTA metadata 저장 실패 시 새 파티션으로 무관리 재부팅하지 않도록 현재 파티션을 다시 boot 대상으로 복구한다.

## 성공 기준

1. CAN 드라이버와 태스크 생성 전에 Summon, Nag, TSLLC, A TX/하드웨어, Nag profile, BUS-OFF 설정이 모두 확정된다.
2. `ota_pending==1`에서 현재 안전값 저장과 `pending=2` commit이 모두 성공해야 CAN이 시작된다.
3. NVS 또는 rollback 준비 실패 시 A/B CAN 드라이버를 만들지 않고 복구 Web UI만 시작한다.
4. OTA 상태표 native 테스트, 기존 전체 native 테스트, HW3 빌드, Web UI 동기화, printf 검사가 통과한다.
