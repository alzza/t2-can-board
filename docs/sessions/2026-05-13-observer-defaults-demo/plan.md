# Plan

목표
- signal observer가 부팅 직후 정지 상태로 시작하고 A채널 단일 기본 신호만 갖는 현재 동작을 회귀 테스트로 고정한다.
- mux 지정 신호가 mux 값 불일치 프레임을 무시하는 현재 추출 로직을 회귀 테스트로 고정한다.

성공 기준
- native helper 테스트에 observer 기본값과 mux 필터 테스트가 추가된다.
- `pio test -e native -f test_native_helpers`가 통과한다.
