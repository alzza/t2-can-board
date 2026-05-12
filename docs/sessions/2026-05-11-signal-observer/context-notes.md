# Context Notes - 실험 탭 신호 관찰기

확인한 신호:
- SCCM_turnIndicatorStalkStatus: 0x249 SCCM_leftStalk, startBit 16, length 4, little, ModelY VEH/CH.
- DAS_autosteerHealthState: 0x24A DAS_visualDebug, startBit 21, length 3, little, ModelY CH.
- DAS_ulcType: 0x24A DAS_visualDebug, startBit 58, length 2, little, ModelY CH.
- DAS_ulcConfirmationRequestActive: 0x3E9 DAS_bodyControls, startBit 28, length 1, little, ModelY VEH.

결정:
- 1차 구현은 관찰 전용이다. 송신 실험은 별도 승인 전까지 넣지 않는다.
- JSON 업로드는 브라우저가 파일을 읽고 `/api/signal-observer/config`로 전달하는 런타임 방식으로 한다.
- NVS 저장은 제외한다. 실차에서 안전하게 바꿀 수 있도록 기본 프리셋을 펌웨어에 두고, 임시 관찰 정의는 업로드로 교체한다.
- A채널은 MCP2515 하드웨어 필터 슬롯이 총 6개다. 기존 659, 1016, 1021에 더해 observer A ID는 최대 3개까지만 허용한다.
- B채널은 TWAI accept-all 뒤 SW 필터 구조라 read 직후 관찰하고 기존 NAG 필터는 그대로 둔다.

용어:
- active_frame_count: raw != 0인 프레임 수.
- change_count: raw 값이 이전 관찰값과 달라진 횟수.
- burst_count: raw가 0에서 nonzero로 올라간 run 개수.
- current_run_frames/last_run_frames/max_run_frames: 턴시그널처럼 몇 프레임 동안 nonzero였는지 보는 카운터.

구현 결과:
- 관찰기 정의는 최대 10개 슬롯이다.
- JSON은 `signals` 배열 또는 배열 자체를 받는다. 지원 필드는 `name`, `channel`, `id` 또는 `frame_id`, `start_bit` 또는 `startBit`, `length`, `idle`, `byte_order`다.
- `byte_order`는 `little`과 `big`을 지원한다. 누락 시 기존 호환을 위해 `little`로 해석한다.
- active 판정은 `raw != idle`이다. `idle`을 생략하면 0이다.
- A채널 ID가 기존 필터 659, 1016, 1021까지 포함해 6개를 넘으면 설정 API가 400으로 거부한다.
- B채널 관찰은 TWAI read 직후 수행하고 기존 NAG SW 필터는 그대로 유지한다.

검증:
- VS Code diagnostics: 수정한 주요 파일 오류 없음.
- `node --check scripts/mock_webui_server.mjs` 통과.
- `pio run -e lilygo_t2can` 통과. RAM 93,288 bytes, Flash 952,405 bytes.
- mock Web UI `http://127.0.0.1:8788/`에서 실험 탭 관찰기 표시, JSON 업로드, 카운터 리셋, A필터 초과 400 응답을 확인했다.
