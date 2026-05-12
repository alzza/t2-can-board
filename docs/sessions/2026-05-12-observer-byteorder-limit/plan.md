# Plan - Signal Observer Byte Order And Limits

1. 관찰기 펌웨어가 `little`과 `big` byte order를 모두 추출하게 한다.
   - 검증: little/big raw 추출 native 테스트 통과.
2. JSON 업로드 파서와 mock 서버가 `byte_order`를 보존하고 잘못된 값을 거부하게 한다.
   - 검증: diagnostics와 mock upload parsing 확인.
3. T-CAN 관찰기 JSON 생성기가 big-endian 신호도 생성하고 A필터 예산을 사전 검사하게 한다.
   - 검증: py_compile과 실제 생성 명령 통과.
4. README에 관찰기 JSON 생성 사용법, 10개 슬롯과 A필터 6 ID 제한을 문서화한다.
   - 검증: 문서 내용과 실제 CLI 옵션 일치.
5. 펌웨어 빌드와 관련 테스트를 실행한다.
   - 검증: `pio test -e native -f test_native_helpers`, `pio run -e lilygo_t2can` 통과.