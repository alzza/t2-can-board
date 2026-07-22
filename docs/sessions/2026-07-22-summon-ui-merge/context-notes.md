# Summon Unlock UI 병합 컨텍스트 노트

## 사용자 확정 사항

- INO의 `INDEX_HTML` 원본은 없으며 API와 상태 필드 기준으로 새 UI를 작성한다.
- Summon Unlock을 기존 EAP에서 독립 기능으로 분리한다.
- 기존 EAP NVS는 이전하지 않고 새 Summon Unlock 기본값을 ON으로 한다.
- UI는 메인 요약, 제어 상세, 진단 카운터로 나눈다.
- A채널 TX 마스터는 진단 안전장치로 유지한다.
- ID 921 AP 상태는 모니터링 전용으로 유지한다.
- A채널은 Summon Unlock/TSLLC 전용, B채널은 Nag Killer 전용이다.

## INO 근거

- 제어 상태는 `summonEnabled` 하나이며 NVS 기본값은 ON이다.
- 게이트는 `Parked || Summoning`이다.
- 모니터링 필드는 `enabled`, `gate`, `ap`, `parked`, `summon`, `aca`, `spr`, `rxMux1`, `txOk`, `txFail`, `rx280`, `rx390`, `rx921`, `rx1016`, `canState`, `uptimeS`이다.
- HW3 활성 비트는 46이고 bit 19는 0으로 만든다.

## 현재 프로젝트 매핑

- Summon 신호와 송신 상태는 A채널 MCP2515 기준으로 표시한다.
- Nag Killer와 TWAI 상태는 B채널 기준으로 표시한다.
- `summon_unlock` JSON 객체는 INO 필드를 유지하면서 A채널 상세 상태를 함께 해석한다.
- AP 상태는 화면에 표시하지만 게이트 계산에는 포함하지 않는다.

## 작업 중 주의사항

- 현재 작업 트리에는 이전 Summon 포팅과 실험 기능 제거 변경이 커밋되지 않은 상태로 존재한다.
- 해당 변경을 보존하며 이번 병합과 직접 관련된 줄만 수정한다.
- 실차 업로드와 NVS 실제 삭제 확인은 이 작업에서 수행하지 않는다.

## 구현 결과

- 빌드 플래그를 `SUMMON_UNLOCK`으로 변경하고 런타임 변수를 `summonUnlockRuntime`으로 분리했다.
- 제어 API는 `/api/summon-unlock`, NVS 키는 `summon_unlock`을 사용한다.
- 기존 `enh_autopilot`과 `bk_eap`은 부팅 시 퇴역 키 삭제 목록에서 제거한다.
- `summon_unlock_enabled`는 저장된 기능 설정이고, `summon_unlock.active`는 A TX 마스터까지 포함한 실제 동작 상태다.
- A TX 마스터가 OFF면 차단 사유를 `A_TX_OFF`, Web UI를 `TX OFF`로 표시한다.
- 메인에는 Gate, Parked/Summoning 상태, TX OK/Fail을 표시한다.
- 제어에는 Enabled, Active, Gate, Parked, Summoning, ACA, SPR, AP 진단, A CAN state, 280 age를 표시한다.
- 진단에는 ID 280/390/921/1016 수신, mux1 적용/차단, Summon TX OK/Fail을 표시한다.
- mock 서버의 퇴역 실험 상태/API와 ID 659/1001 기본 관찰 항목을 제거했다.
- 제거된 Legacy/HW4 핸들러를 계속 참조하던 고아 native 테스트 파일을 삭제했다. Git에서 복구 가능하다.

## 검증 결과

- Summon/HW3/헬퍼/Web UI 집중 테스트 57/57 통과.
- 전체 native 테스트 84/84 통과.
- B채널 Nag Killer 테스트 43/43 통과.
- `mock_webui_server.mjs` 모듈 문법 검사 통과.
- `python3 scripts/check_printf_formats.py` 통과. 32개 파일을 검사했다.
- `git diff --check` 통과.
- `pio run -e lilygo_t2can` 통과.
- 최종 빌드 RAM 106680바이트(32.6%), Flash 978689바이트(49.8%).
- 펌웨어 SHA-256은 `2bc44157252d6a11737a1d79ef53f4e9dbcce160c111c3936110b10f20912286`이다.

## 남은 리스크

- 로컬 테스트는 CAN 프레임과 UI/API 구조를 검증하지만 차량 게이트 전이와 실제 NVS 삭제는 확인하지 못한다.
- 펌웨어 업로드 후 위 체크리스트의 실차 항목을 확인해야 한다.
