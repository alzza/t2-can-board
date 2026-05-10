# Web UI redesign plan

목표:
- 메인 타일에서 중복 진단 카운터를 줄이고 운전 중 판단에 필요한 상태를 전면에 배치한다.
- 제어 화면은 큰 카드형 토글 중심으로 정리한다.
- OTA 화면을 별도 탭/섹션으로 명확히 노출한다.
- 실제 embedded Web UI를 mock 서버로 띄워 브라우저에서 검증한다.

범위:
- `include/web/web_ui.h`의 HTML/CSS/JS를 중심으로 수정한다.
- mock 서버는 새 UI가 호출하는 API가 부족하거나 production 의존성을 흉내 내야 할 때만 보강한다.
- `include/web/web_server.h`는 UI 제거로 숨겨진 런타임 의존성이 생길 때만 최소 수정한다.
- CAN/TWAI 로직은 변경하지 않는다.

성공 기준:
- `node --check scripts/mock_webui_server.mjs` 통과.
- `git diff --check -- include/web/web_ui.h include/web/web_server.h scripts/mock_webui_server.mjs docs/sessions/chat_log_2026-05-10.md` 통과.
- mock Web UI에서 메인, 제어, 진단, OTA 화면이 렌더링된다.
- 메인 화면에 현재 핸들토크, 조향각, 오토파일럿 상태가 표시된다.
- 프로파일 선택과 주요 토글이 브라우저에서 에러 없이 동작한다.
- 제어 화면에서 EAP/TSLLC를 켜면 숨겨진 A Channel TX 의존성이 자동으로 만족된다.
- 메인 화면의 A/B 채널 타일은 Hz만 표시하고, 상태 문구와 색상은 상단 배지에서 확인된다.
