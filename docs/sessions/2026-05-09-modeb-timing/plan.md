# 2026-05-09 Mode B timing adjustment plan

목표:
- `canmod_20260509_165201.txt` 분석에서 확인된 Mode B 첫 echo 지연을 줄인다.
- AP gate 허용 범위는 변경하지 않고, state 2와 strong demand의 대기 시간만 조정한다.
- 코드 주석, 포팅 가이드, 세션 로그에 변경 근거를 남긴다.

검증 기준:
- `git diff --check`가 변경 파일에서 통과한다.
- `pio run -e lilygo_t2can`가 통과한다.
- 가능하면 `pio test -e native`까지 통과한다.
