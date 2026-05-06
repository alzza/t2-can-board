import unicodedata

def vw(s):
    w = 0
    for c in s:
        eaw = unicodedata.east_asian_width(c)
        w += 2 if eaw in ('W', 'F') else 1
    return w

INNER = 72  # visual cols for box content (between the two │)

def box_line(content):
    w = vw(content)
    pad = INNER - w
    return f" *  \u2502{content}{' ' * pad}\u2502"

def box_header(title_after_dash):
    w = 1 + vw(title_after_dash)  # ─ is 1
    dashes = INNER - w
    dash_str = '\u2500' * dashes
    return f" *  \u250c\u2500{title_after_dash}{dash_str}\u2510"

def box_footer():
    dash_str = '\u2500' * INNER
    return f" *  \u2514{dash_str}\u2518"

lines_c1 = [
    "",
    "  [nagKillerTask]  prio = 10  \u2190 \uc774 \ud0dc\uc2a4\ud06c\uac00 10, Core 1 \uc790\uccb4\uac00 10 \uc544\ub2d8",
    "   \u2502",
    "   \u251c\u2500 CAN-A \ud3f4\ub9c1 (\ub9e4 iter)",
    "   \u2502   \u2514\u2500 appLoop<MCP2515Driver>()",
    "   \u2502       \u2514\u2500 HW3Handler (ID 0x3FD / 1021)",
    "   \u2502           \u251c\u2500 Mux 0 \u2192 TSLLC  bit38/39 \uc778\uc81d\uc158 (\uc2a4\ud1b1\uc0ac\uc778/\ucd08\ub85d\ubd88)",
    "   \u2502           \u2514\u2500 Mux 1 \u2192 EAP    bit19/46 \uc778\uc81d\uc158 (Enhanced Autopilot)",
    "   \u2502       \u2514\u2500 EFLG/TEC/REC/MERRF 1\ucd08 \uc8fc\uae30 \ud3f4\ub9c1",
    "   \u2502",
    "   \u2514\u2500 CAN-B \ud3f4\ub9c1 (\ub9e4 iter)",
    "       \u2514\u2500 TWAI RX (twai_receive, timeout=1ms)",
    "           \u251c\u2500 SW \ud544\ud130: ID 0x370(880) \u00b7 0x399(921) \uc678 \uc989\uc2dc skip",
    "           \u251c\u2500 NagHandler",
    "           \u2502   \u251c\u2500 Mode A (Stealth PRNG): \ub098\uadf8 \uba54\uc2dc\uc9c0 \uc5b5\uc81c",
    "           \u2502   \u251c\u2500 Mode B (Smart FSM): \uc0c1\ud0dc\uba38\uc2e0 \uae30\ubc18 \uc801\uc751\ud615 \uc5b5\uc81c",
    "           \u2502   \u2514\u2500 checksum: (sum + 0x73) & 0xFF",
    "           \u251c\u2500 BUS-OFF \ubcf5\uad6c: hard re-install + \ucfe8\ub2e4\uc6b4(\ub7f0\ud0c0\uc784 \uc124\uc815 \uac00\ub2a5)",
    "           \u2514\u2500 TEC \u2265 96 \uc870\uae30 \uacbd\uace0 / BUS-OFF \uc774\ubca4\ud2b8 \ub85c\uadf8 push",
    "",
    "  [loopTask]  prio = 1  (Arduino \uae30\ubcf8)",
    "   \u2514\u2500 setup() \uc644\ub8cc \ud6c4 loop()\uc5d0\uc11c vTaskDelete(NULL) \u2192 \uc989\uc2dc \uc885\ub8cc",
    "       (\ud0dc\uc2a4\ud06c \uc2a4\ud0dd ~8KB \ud574\uc81c, nagKillerTask \uac04\uc12d \uc5c6\uc74c)",
    "",
    "  [TWAI ISR]  intr_flags = ESP_INTR_FLAG_IRAM",
    "   \u2514\u2500 RX FIFO \ubcf4\ud638: OTA \uc911 \uce90\uc2dc disable \uad6c\uac04\uc5d0\uc11c\ub3c4 \uc778\ud130\ub7fd\ud2b8 \ucc98\ub9ac \uc720\uc9c0",
    "       (sdkconfig CONFIG_TWAI_ISR_IN_IRAM=y \ud544\uc694)",
]

lines_c0 = [
    "",
    "  [WiFi task]      prio = 23  (ESP-IDF \ub0b4\ubd80 \u2014 \uc9c1\uc811 \uc0dd\uc131 \uc544\ub2d8)",
    "",
    "  [esp_http_server]  stack = 16384",
    "   \u2514\u2500 Web Dashboard (single-file SPA, web_ui.h / web_server.h)",
    "       \u251c\u2500 GET  /                     \u2192 \ub300\uc2dc\ubcf4\ub4dc HTML",
    "       \u251c\u2500 GET  /api/status           \u2192 \ud1b5\ud569 \uc0c1\ud0dc JSON (3s polling)",
    "       \u251c\u2500 POST /api/nag-mode|update  \u2192 NagConfig \ubcc0\uacbd",
    "       \u251c\u2500 POST /api/enhanced-autopilot | /api/tsllc | /api/nag-killer",
    "       \u251c\u2500 POST /api/busoff-mode|cooldown",
    "       \u251c\u2500 GET  /api/busoff-log[-dl]  DELETE /api/busoff-log",
    "       \u251c\u2500 POST /api/twai-ss-tx | /api/twai-busoff-stop",
    "       \u251c\u2500 POST /api/can-diag/start   GET /api/can-diag/log",
    "       \u251c\u2500 GET  /api/logs-bundle | /api/timeseries-csv",
    "       \u2514\u2500 POST /api/ota | /api/reboot | /api/ota-confirm|rollback",
    "",
    "  [canAlertTask]   prio = 1  (20ms \uc8fc\uae30)",
    "   \u2514\u2500 TWAIDriver::pollAlerts() \u2192 eventLog \uae30\ub85d",
    "       (nagKillerTask \ud56b\ud328\uc2a4\uc640 \uc644\uc804 \ubd84\ub9ac)",
    "",
    "  [statusLogTask]  prio = 1  (5s \uc8fc\uae30, T2CAN_STATUS_LOG_TASK=0 \uc2dc OFF)",
    "   \u2514\u2500 5\ucd08 \uc0c1\ud0dc \uc694\uc57d Serial \ucd9c\ub825",
    "",
    "  [timeseriesTask] prio = 1  (5s \uc8fc\uae30)",
    "   \u2514\u2500 B\ucc44\ub110 Hz/\uc5d0\ub7ec\uce74\uc6b4\ud130/\uacb0\uc815\ubd84\ud3ec \uc218\uc9d1 (\ucd5c\uadfc 30\ubd84, CSV \ub2e4\uc6b4\ub85c\ub4dc)",
]

import sys
out = open('/tmp/box_out.txt', 'w') if '--file' in sys.argv else sys.stdout

# Verify widths
all_lines = lines_c1 + lines_c0
max_w = max(vw(l) for l in all_lines)
print(f"# max visual width = {max_w}, INNER = {INNER}", file=sys.stderr)

out.write(box_header("─ Core 1 (CAN 전용) ") + "\n")
for l in lines_c1:
    out.write(box_line(l) + "\n")
out.write(box_footer() + "\n")
out.write(" *\n")
out.write(box_header("─ Core 0 (WiFi / HTTP / 보조 태스크) ") + "\n")
for l in lines_c0:
    out.write(box_line(l) + "\n")
out.write(box_footer() + "\n")
if out != sys.stdout:
    out.close()
    print("Written to /tmp/box_out.txt")
