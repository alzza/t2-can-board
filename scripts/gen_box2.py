import sys
import unicodedata

MIN_INNER = 72


def vw(s):
    w = 0
    for c in s:
        eaw = unicodedata.east_asian_width(c)
        w += 2 if eaw in ("W", "F") else 1
    return w


def box_line(content, inner):
    w = vw(content)
    if w > inner:
        raise ValueError(f"line too wide: {w} > {inner}: {content}")
    return f" *  │{content}{' ' * (inner - w)}│"


def box_header(title, inner):
    content = f"─ {title} "
    w = vw(content)
    if w > inner:
        raise ValueError(f"header too wide: {w} > {inner}: {title}")
    return f" *  ┌{content}{'─' * (inner - w)}┐"


def box_footer(inner):
    return f" *  └{'─' * inner}┘"


lines_c1 = [
    "",
    "  [nagKillerTask]  prio = 10  stack = 8192  pinned Core 1",
    "   │",
    "   ├─ CAN-A 폴링 (매 iter, MCP2515/SPI)",
    "   │   └─ appLoop<MCP2515Driver>()",
    "   │       ├─ HW3Handler (ID 0x3FD / 1021)",
    "   │       │   ├─ Mux 0 → TSLLC  bit38/39 인젝션 (스톱사인/초록불)",
    "   │       │   └─ Mux 1 → EAP    bit19/46 인젝션 (Enhanced Autopilot)",
    "   │       └─ EFLG/TEC/REC/MERRF 1초 폴링 + TX guard + TXBO 복구",
    "   │",
    "   └─ CAN-B 폴링 (매 iter, TWAI accept-all + SW 필터)",
    "       └─ TWAIDriver::read() → twai_receive(timeout=0)",
    "           ├─ RX 제한: iter당 최대 30프레임 처리로 WDT 보호",
    "           ├─ SW 필터: 880(EPAS) · 921/923(DAS)",
    "           ├─ NagHandler",
    "           │   ├─ MODE 1: +1.80Nm 고정 echo",
    "           │   ├─ MODE 2: 4단계 순환 + 휴지 echo",
    "           │   ├─ 실제 handsOn=0 + 선택한 AP 전용 범위 적용",
    "           │   └─ checksum: (sum + 0x73) & 0xFF",
    "           ├─ BUS-OFF 복구: soft(twai_initiate_recovery) → hard fallback",
    "           └─ TEC ≥ 96 조기 경고 / BUS-OFF 이벤트 로그 push",
    "",
    "  [loopTask]  prio = 1  (Arduino 기본)",
    "   └─ setup() 완료 후 loop()에서 vTaskDelete(NULL) → 즉시 종료",
    "       (태스크 스택 ~8KB 해제, nagKillerTask 간섭 없음)",
    "",
    "  [TWAI ISR]  IRAM flag 조건부 사용",
    "   └─ CONFIG_TWAI_ISR_IN_IRAM=y 빌드에서만 ESP_INTR_FLAG_IRAM 설정",
    "       (Arduino-ESP32 기본 S3 sdkconfig 비활성 시 flag 미설정)",
]

lines_c0 = [
    "",
    "  [WiFi AP]        SSID = TeslaCAN  AP-only (STA 비활성)",
    "",
    "  [esp_http_server]  stack = 16384",
    "   └─ Web Dashboard (single-file SPA, web_ui.h / web_server.h)",
    "       ├─ GET  /                     → 대시보드 HTML",
    "       ├─ GET  /api/status           → 통합 상태 JSON (3s polling)",
    "       ├─ GET  /api/nag-stats        → B채널 Mode A/B 진단 JSON",
    "       ├─ POST /api/nag-mode|update|reset → NagConfig 변경",
    "       ├─ POST /api/enhanced-autopilot | /api/tsllc | /api/nag-killer",
    "       ├─ POST /api/busoff-mode|cooldown | /api/twai-ss-tx",
    "       ├─ GET  /api/busoff-log[-dl]  DELETE /api/busoff-log",
    "       ├─ POST /api/can-diag/start   GET /api/can-diag/log",
    "       ├─ GET  /api/logs-bundle      → [1]~[5] 통합 로그 다운로드",
    "       ├─ GET  /api/timeseries.csv | /api/events.csv (디버그 보조)",
    "       └─ POST /api/ota | /api/reboot | /api/ota-confirm|rollback",
    "",
    "  [canAlertTask]   prio = 1  (20ms 주기)",
    "   └─ TWAIDriver::pollAlerts() → eventLog [5] 기록",
    "       (nagKillerTask 핫패스와 분리)",
    "",
    "  [statusLogTask]  prio = 1  (5초 주기, Core 0)",
    "   └─ 5초 상태 요약 Serial 출력",
    "",
    "  [timeseriesTask] prio = 1  (5s 주기)",
    "   └─ RAM 120샘플 × 5초 = 최근 10분, 통합 로그 [4] 섹션에 포함",
]


def write_boxes(out):
    all_lines = lines_c1 + lines_c0
    inner = max(MIN_INNER, max(vw(line) for line in all_lines) + 2)
    print(f"# max visual width = {max(vw(line) for line in all_lines)}, INNER = {inner}", file=sys.stderr)

    out.write(box_header("Core 1 (CAN 전용)", inner) + "\n")
    for line in lines_c1:
        out.write(box_line(line, inner) + "\n")
    out.write(box_footer(inner) + "\n")
    out.write(" *\n")
    out.write(box_header("Core 0 (WiFi / HTTP / 보조 태스크)", inner) + "\n")
    for line in lines_c0:
        out.write(box_line(line, inner) + "\n")
    out.write(box_footer(inner) + "\n")


if __name__ == "__main__":
    if "--file" in sys.argv:
        with open("/tmp/box_out.txt", "w", encoding="utf-8") as out_file:
            write_boxes(out_file)
        print("Written to /tmp/box_out.txt")
    else:
        write_boxes(sys.stdout)
