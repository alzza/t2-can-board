# Plan - OTA NVS Reset Button

1. OTA 탭에 위험 동작 버튼을 추가한다.
   - 검증: 버튼이 OTA 카드 안에 보이고 확인창 뒤에만 API를 호출한다.
2. 펌웨어에 NVS namespace 초기화 API를 추가한다.
   - 검증: `/api/nvs-reset`가 `canmod` namespace를 erase/commit하고 재부팅을 예약한다.
3. mock 서버도 같은 API를 받아 로컬 UI 테스트가 가능하게 한다.
   - 검증: `node --check`와 mock POST 응답 확인.
4. 빌드와 정적 검증을 실행한다.
   - 검증: diagnostics, `pio run -e lilygo_t2can`, `git diff --check` 통과.