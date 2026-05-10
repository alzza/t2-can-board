# Web UI redesign context notes

- 실제 UI 소스는 `include/web/web_ui.h`의 `WEB_UI_HTML` raw literal이다.
- mock 서버는 이 raw literal을 직접 읽어 서빙하므로 UI 복사본을 만들지 않는다.
- 메인 화면은 운전 중 볼 정보 중심으로 줄이고, 원시 카운터는 진단 패널로 내린다.
- 새 메인 핵심 타일은 현재 핸들토크, 조향각, 오토파일럿 상태다.
- 탭 구조는 사용자 요청에 따라 메인 / 제어 / 진단 / OTA 네 개만 둔다. 플러그인 탭은 만들지 않는다.
- OTA는 이미 API stub/mock이 있으므로 우선 화면 구조와 상태 노출을 정리한다.
- 제어 화면에서 A Channel TX 마스터 토글은 제거했다. EAP/TSLLC를 켤 때 실제 의존성인 `aChannelTxRuntime`은 서버가 자동으로 켜고, mock 서버도 같은 동작을 흉내 낸다.
- 비상 기능해제는 숨겨진 A Channel TX까지 강제로 끄는 기존 안전 동작을 유지한다.
- 메인 A/B 타일은 상태 문구 없이 Hz만 보여준다. 채널 상태는 상단 배지에서 `A OK`, `B RUNNING`, `B BUS-OFF`, `NO FRAMES`처럼 보여주며 dot 색상은 ok/warn/err에 따라 초록/노랑/빨강으로 바뀐다.
- 진단 화면은 A/B 채널 카드를 세로로 쌓고, A/B 플래그 설명을 짧은 문장으로 표시한다.