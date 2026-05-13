# Plan

목표
- D모드 실차 실패 원인을 바로 추측 수정하지 않고, native_nag에서 재현 가능한 실패 조건을 먼저 고정한다.
- 첫 번째 TDD 대상은 D모드 sign hold 경로가 아니라 주입 자체를 죽일 수 있는 gate/판정 mismatch 후보를 작은 단위로 검증하는 것이다.
- 테스트가 가리키는 최소 코드만 수정한다.

설계 선택
- 안 1: D모드 토크/타이밍 상수를 바로 바꾼다. 빠르지만 원인 분리가 안 된다.
- 안 2: native_nag에 실패 테스트를 먼저 추가하고, 그 테스트가 지적하는 판정 로직만 수정한다.
- 선택: 안 2. 이유는 실차 영향이 큰 NAG 로직에서 추측성 상수 조정은 재작업 위험이 크기 때문이다.

단계
1. 기존 D모드 테스트와 handlers hot path 읽기
   - 검증: 실패 가설 1개를 문장으로 고를 수 있어야 함
   - 성공 조건: failing test 대상 함수/분기 확정
2. failing test 추가
   - 파일: test/test_native_nag/test_nag_handler.cpp
   - 검증: `pio test -e native_nag -f test_native_nag`에서 새 테스트 실패
3. 최소 코드 수정
   - 파일: include/handlers.h 필요시 include/can_helpers.h
   - 검증: 같은 native_nag 테스트 재실행
4. 결과 기록
   - 파일: docs/sessions/chat_log_2026-05-13.md, checklist.md, context-notes.md
   - 검증: git diff --check
