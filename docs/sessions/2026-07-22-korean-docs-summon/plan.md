# 한글 README·변경 이력과 HW3 Summon 사용법 갱신 계획

## 목표

- 루트 README와 변경 이력을 한글로 유지하는 정책을 저장소 지침에 기록한다.
- 검증된 `summon_unlock.ino`와 현 HW3 포팅 코드 기준으로 EU Unlock과 Summon Unlock의 호출 방식·조건·정지 방법을 명시한다.

## 선택

- README는 현재 T2-CAN HW3 구현만 설명하도록 한글로 재정리한다.
- EU Unlock을 독립 기능으로 오인하지 않도록, Summon 수정 프레임의 bit 19=0 동작이며 별도 토글·상시 송신이 없음을 명시한다.
- docs-site의 Summon 및 변경 이력도 같은 한글 용어와 조건으로 맞춘다.

## 완료 기준

1. README에 Summon의 실제 전송 조건, 게이트 신호, HW3 bit 46, EU bit 19, AP 진단 전용 조건이 모두 있다.
2. README와 두 변경 이력의 새 문서가 한글이다.
3. Web UI 동기화, 릴리스 메타데이터, Markdown diff 검사가 통과한다.
