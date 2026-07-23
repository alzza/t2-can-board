# 한글 README·변경 이력과 HW3 Summon 사용법 컨텍스트 노트

## 코드 근거

- INO와 포팅 코드 모두 Summon 수정 대상은 CAN ID `0x3FD`의 mux 1이며, 8바이트 미만 프레임은 처리하지 않는다.
- 전송 전제는 Summon 기능 ON, 게이트 OPEN, A TX 마스터 ON, TX Guard 비활성이다.
- 게이트는 `Parked || Summoning`이다. Parked는 CAN 280 기어를 우선하고 CAN 390은 280이 5초 이상 없을 때만 보조한다.
- Summoning은 CAN 280의 ACA와 CAN 1016의 SPR 관측이 모두 있을 때 성립한다. ACA가 꺼지면 SPR 래치를 지운다.
- CAN 280이 5초 이상 없으면 INO와 동일하게 Parked로 간주한다.
- EU Unlock은 mux 1 수정 프레임에서 bit 19를 0으로 하는 동작이며 별도 토글·독립 송신은 없다.
- HW3 Summon Unlock은 같은 프레임에서 bit 46을 1로 한다. CAN 921 AP 상태는 표시용이며 게이트를 열지 않는다.

## 구현과 검증

- 루트 README를 현재 T2-CAN HW3 전용 한글 안내로 재정리했다.
- 루트·docs-site 변경 이력과 docs-site Summon·이전 EAP 안내를 한글로 통일했다.
- `.github/copilot-instructions.md`에 README, CHANGELOG, docs-site 공개 문서를 한글로 새 작성·갱신하는 정책을 추가했다.
- `python3 scripts/check_release_metadata.py`, `python3 scripts/sync_web_ui.py --check`, `python3 scripts/check_printf_formats.py`, `git diff --check`가 통과했다.
