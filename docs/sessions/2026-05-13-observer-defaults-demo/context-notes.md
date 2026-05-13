# Context Notes

- 현재 워크트리에는 observer를 기본 정지 상태와 A채널 단일 기본 신호로 바꾸는 더티 변경이 이미 있다.
- 기존 `test/test_native_helpers/test_helpers.cpp`는 signal observer raw 추출만 검증하고 기본 preset/runtime 기본값은 검증하지 않는다.
- 이번 데모 작업은 런타임 코드 추가 수정 없이 테스트로 현재 의도를 고정하는 데 집중한다.
