# Context Notes

- 5/13 세션로그만으로는 D모드 실패 직접 원인 근거가 없다.
- 현재 코드에는 D모드(profile 4)와 sign hold 로직이 이미 존재한다.
- 따라서 이번 TDD 첫 단계는 구현 부재가 아니라 구현이 놓친 판정 mismatch를 찾는 것이다.
- 첫 수정 전에는 native_nag에서 깨지는 테스트를 반드시 먼저 만든다.
