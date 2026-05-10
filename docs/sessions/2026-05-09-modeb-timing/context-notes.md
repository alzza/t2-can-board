# 2026-05-09 Mode B timing context notes

- 실차 로그 `canmod_20260509_165201.txt`의 마지막 10분은 모두 Mode B였다.
- 최종 약 160초 무동작 구간은 `d880=16005`, `dSkipAP=16005`, `dEcho=0`으로 AP gate 차단이었다.
- 이번 수정은 AP=1/2 `AP_BLOCK` 해소가 아니라 AP=3 허용 구간에서 첫 echo 반응을 앞당기는 1차 타이밍 실험이다.
- 분석된 첫 echo delay는 state 2에서 주로 2000/2001ms, strong path에서 1000ms였다.
- 적용값은 state 2 delay 700ms, strong delay 400ms다.
