# Plan - 실험 탭 신호 관찰기

목표:
- 실험 탭에 송신 없는 신호 관찰기를 추가한다.
- 약 10개 신호 정의를 고정 배열로 관찰하고, 브라우저 JSON 업로드로 런타임 교체할 수 있게 한다.
- SCCM_turnIndicatorStalkStatus처럼 특정 값이 몇 프레임 동안 유지되는지 burst/run 카운터로 확인한다.

범위:
1. 관찰 전용 코어 추가.
   - 검증: raw, frame_count, active_frame_count, change_count, burst_count, current/last/max_run_frames가 status JSON에 나타남.
2. A/B 수신 경로 연결.
   - 검증: A채널은 MCP2515 6개 필터 슬롯 안에서만 observer ID를 추가하고, B채널은 read 직후 관찰만 수행.
3. Web UI와 API 추가.
   - 검증: 기본 프리셋, JSON 업로드, 카운터 리셋이 mock UI에서 동작.
4. 통합 로그 반영.
   - 검증: /api/logs-bundle에 observer snapshot이 포함됨.

비범위:
- 관찰 신호를 차량에 송신하지 않는다.
- 업로드한 JSON을 NVS에 영속 저장하지 않는다.
- MCP2515 하드웨어 필터 한계인 6개 ID를 넘겨 A채널 임의 10개 ID를 모두 수신하게 만들지 않는다.
