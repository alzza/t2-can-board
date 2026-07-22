> [!CAUTION]
> Tesla vehicle firmware and CAN behavior can change without notice. Validate this firmware on a parked vehicle after every vehicle OTA before enabling any transmission feature.

<br>
<hr>

# Tesla Open Can Mod — LILYGO T2-CAN Edition

[Website](https://teslaopencanmod.org) | [Documentation](https://teslaopencanmod.org/docs/intro) | [Community Discord](https://discord.gg/ZTQKAUTd2F)

An open-source CAN bus modification firmware for Tesla HW3 vehicles, running on the **LILYGO T2-CAN (ESP32-S3)**. Features dual-channel CAN (MCP2515 + TWAI), a WiFi web interface with OTA updates, and real-time runtime toggles — no recompile needed.

Some sellers charge up to 500 € for a solution like this. The hardware costs around 20 €, and even with labor factored in, a fair price is no more than 50 €. This project exists so nobody has to overpay.

## Disclaimer

> [!WARNING]
> **FSD is a premium feature and must be properly purchased or subscribed to.** Any attempt to bypass the purchase or subscription requirement will result in a permanent ban from Tesla services.

> [!WARNING]
> **Modifying CAN bus messages can cause dangerous behavior or permanently damage your vehicle.** The CAN bus controls everything from braking and steering to airbags — a malformed message can have serious consequences. If you don't fully understand what you're doing, **do not install this on your car**.

This project is for testing and educational purposes only and for use on **private property**. The authors accept no responsibility for any damage to your vehicle, injury, or legal consequences resulting from the use of this software. This project may void your vehicle warranty and **may not comply with road safety regulations in your jurisdiction**.

For any use beyond private testing, you are responsible for complying with all applicable local laws and regulations. Always keep your hands on the wheel and stay attentive while driving.

## Supported Board

This firmware targets the **LILYGO T2-CAN** (ESP32-S3) exclusively.

| Board | MCU | CAN-A | CAN-B | WiFi |
|-------|-----|-------|-------|------|
| LILYGO T2-CAN | ESP32-S3 | MCP2515 (SPI, 10 MHz) | TWAI built-in | AP mode |

> Other boards (Feather RP2040 CAN, Feather M4 CAN Express, M5Stack Atomic CAN Base) are supported in the upstream project at [teslaopencanmod.org](https://teslaopencanmod.org). This branch is T2-CAN specific.

## Features

All features are toggled at **runtime** via the web interface — no recompile required.

| Feature | Channel | Description |
|---------|---------|-------------|
| **Conditional Summon Unlock (HW3)** | CAN-A | Modifies ID 1021 mux 1 only while the vehicle is Parked or Summoning and A TX is enabled |
| **TSLLC (Traffic Light & Stop Sign)** | CAN-A | Sets the TSLLC and green-light continue bits in CAN ID 1021 |
| **Autosteer Nag Killer** | CAN-B | Echoes modified EPAS torque frame (CAN ID 880) to suppress hands-on-wheel nag |
| **Web Interface & OTA** | WiFi | Real-time monitoring, runtime toggles, over-the-air firmware updates |

### Prerequisites for FSD features

**An active FSD package (purchased or subscribed) is required.** The firmware enables FSD at the CAN level but the vehicle still needs a valid entitlement from Tesla.

If FSD subscriptions are unavailable in your region, a foreign Tesla account workaround exists — see [teslaopencanmod.org/docs/getting-started/fsd-subscription](https://teslaopencanmod.org/docs/getting-started/fsd-subscription).

## Quick Start

### 1. Install PlatformIO

```bash
pip install platformio
```

### 2. Clone and build

```bash
git clone <repo-url>
cd tesla-open-can-mod-can-stabilization
pio run -e lilygo_t2can
```

### 3. Flash

Connect the T2-CAN via USB, then:

```bash
pio run -e lilygo_t2can -t upload
```

### 4. Connect

- Connect to WiFi AP **`TeslaCAN`** (no password)
- Open **`http://192.168.4.1`** in a browser
- Toggle features, view live CAN stats, and manage OTA updates

Full installation guide: [teslaopencanmod.org/docs/getting-started/firmware-flash](https://teslaopencanmod.org/docs/getting-started/firmware-flash)

## Wiring

Connect to the vehicle's **X179 connector** (under the front trunk, main CAN backbone):

| X179 Pin | Signal | Connect to |
|----------|--------|------------|
| 13 | CAN-H | T2-CAN CAN-H |
| 14 | CAN-L | T2-CAN CAN-L |

The T2-CAN exposes **two independent CAN channels**:

| Channel | Interface | GPIO | Purpose |
|---------|-----------|------|---------|
| CAN-A | MCP2515 (SPI) | CS=10, SCK=12, MISO=13, MOSI=11, RST=9 | FSD / EAP / TSLLC (CAN bus 13/14) |
| CAN-B | TWAI built-in | TX=7, RX=6 | Nag Killer — EPAS torque frame echo (CAN bus 2/3, X179) |

> [!IMPORTANT]
> Remove or bypass the onboard 120 Ω termination resistor on your CAN transceiver board. The vehicle CAN bus is already terminated; a second resistor will cause communication errors.

## Web Interface

Once connected to the **`TeslaCAN`** WiFi AP and navigating to `http://192.168.4.1`, the web dashboard provides:

- **Live CAN stats** — per-channel frame rate, TX/RX counters, BUS-OFF events
- **Feature toggles** — enable/disable any feature without reflashing
- **BUS-OFF event log** — timestamped history, CSV download
- **Timeseries data** — 5-second interval rolling 30-minute history, CSV download
- **Signal Observer** — upload generated JSON to watch selected T-CAN signals without transmitting frames
- **OTA firmware update** — drag-and-drop `.bin` upload with rollback safety

### Conditional Summon Unlock on HW3

This build targets **HW3**. Summon Unlock is a conditional CAN-A feature, not a continuous injector.

Before enabling it, park the vehicle, connect to the dashboard, and confirm CAN-A has stable receive traffic with no BUS-OFF event. After an OTA update, the firmware deliberately starts with Summon Unlock, TSLLC, Nag Killer, and the CAN-A TX master switch OFF.

1. Open **Controls** and enable **Conditional Summon Unlock (HW3)**.
2. In the **CAN-A** card, enable the **A TX master** switch.
3. Verify the Summon card reports `Enabled`, `Active`, and `Gate OPEN` only when the vehicle is `PARKED` or `SUMMONING`.
4. Monitor CAN-A `TX OK / Fail`, `TEC`, `BUS-OFF`, and the 280/390/921/1016 receive counters before relying on the result.

The firmware modifies CAN ID `0x3FD` (decimal 1021), mux 1, only when all of the following are true:

- Summon Unlock is enabled.
- The CAN-A TX master is enabled.
- The gate is open: `Parked || Summoning`.

For HW3, the modified frame clears bit 19 and sets bit 46. The displayed AP state is diagnostic only; it does not open the injection gate. Turn off either **Conditional Summon Unlock** or the **A TX master** to stop Summon/TSLLC transmission immediately.

### Signal Observer JSON

The Signal Observer is a receive-only diagnostic tool. It extracts raw values from selected CAN signals, records transitions in a bounded event log, and exports a CSV through the web UI.
Each signal must use one channel only: `A`/`VEH` or `B`/`CH`. Mixed `A+B`/`both`/`AB` configs are rejected to avoid cross-channel raw-value mixing.

Generate uploadable JSON from T-CAN signal names:

```bash
.venv/bin/python scripts/tcan_signal_observer_json.py SCCM_turnIndicatorStalkStatus UI_autoLaneChangeEnable --output docs/tcan_observer.json
```

Big-endian Motorola signals are supported. The generator writes `byte_order` from T-CAN metadata, and the firmware decodes both `little` and `big` values:

```bash
.venv/bin/python scripts/tcan_signal_observer_json.py EPAS3P_torsionBarTorque EPAS3P_handsOnLevel --bus CH --channel B --output docs/epas_observer.json
```

Muxed signals include `mux_start_bit`, `mux_length`, and `mux_value`. The firmware ignores frames whose mux selector does not match, so shared-bit frames such as `0x3FD UI_autopilotControl` do not create false raw changes.

Observer limits:

- Maximum observer slots: **10 signals**.
- CAN-A MCP2515 hardware filters: **6 total frame IDs**.
- CAN-A always reserves `0x293`, `0x3F8`, `0x3FD`, and `0x3E9`, so generated A observers normally have room for **2 additional distinct A-side frame IDs**.
- B-only observers do not consume CAN-A filter slots.
- The generator fails early if the requested A-side signals would exceed the 6-ID filter budget.
- Generated JSON includes `aFilterIds`, `aFilterUsed`, and `aFilterRemaining` metadata for quick review before upload.

The CAN controller handles physical CAN CRC validation in hardware. Tesla payload checksum/counter handling is separate and is required only for transmitted modified frames, not for receive-only Signal Observer extraction.

### OTA auto-refresh

After uploading a new firmware `.bin` file in the OTA tab:

1. **Upload completes** → device reboots automatically
2. **Within 9 seconds** → web dashboard polls device status
3. **Device responds** → page automatically refreshes with cache-busting
4. **Display updates** → shows new firmware version

No manual page refresh needed.

**Metadata display format:**
- Firmware: `1.3.4 · YY-MM-DD` (version · build date)
- Build: `YY-MM-DD HH:MM:SS` (local build time)

### OTA rollback safety

| State | Meaning |
|-------|---------|
| `pending=0` | Normal operation |
| `pending=1` | OTA payload written; first boot records current safe feature values |
| `pending=2` | New firmware confirmation window — confirm within 60 seconds or auto-rollback |
| `pending=3/4` | Previous firmware rollback and its 60-second confirmation window |
| `pending=5` | Recovery mode — CAN disabled, web server only |

If a new firmware fails to confirm, the device automatically reverts to the previous firmware. Any NVS or OTA preparation failure enters CAN-disabled recovery mode instead of starting either CAN driver.

## Firmware Architecture

```
Core 1 (CAN-dedicated)
  nagKillerTask  prio=10
    ├─ CAN-A poll  →  HW3Handler (ID 1021)  →  FSD / EAP / TSLLC injection
    └─ CAN-B poll  →  NagHandler (ID 880/921)  →  EPAS echo
  loopTask  prio=1  →  vTaskDelete(NULL) immediately

Core 0 (WiFi / HTTP / auxiliary)
  WiFi AP task  prio=23
  esp_http_server  (45 API endpoints)
  canAlertTask  prio=1  20 ms  →  TWAI alert polling
  statusLogTask  prio=1  5 s   →  status summary log
  timeseriesTask prio=1  5 s   →  rolling timeseries
```

Settings are persisted to NVS (namespace `canmod`) — all runtime toggles survive reboots.

## CAN Message Details

> [!IMPORTANT]
> Every modified frame that contains a counter or checksum must have those fields recalculated. Failure to do so causes the receiving ECU to silently discard the frame.

### CAN-A — HW3 (ID 1021)

| CAN ID | Mux | Bit(s) | Value | Signal | Description |
|--------|-----|--------|-------|--------|-------------|
| 1021 | 0 | 38 | 0/1 | `UI_fsdStopsControlEnabled` | Read FSD trigger / TSLLC inject |
| 1021 | 0 | 39 | 1 | `UI_fsdContinueOnGreenWithCIPV` | Green-light auto-go with lead car |
| 1021 | 0 | 46 | 1 | — | Enable FSD |
| 1021 | 1 | 19 | 0 | `UI_applyEceR79` | Suppress nag (EAP) |
| 1021 | 1 | 47 | 1 | `UI_hardCoreSummon` | Unlock summon range (EAP) |

### CAN-B — Nag Killer (ID 880)

| CAN ID | Description |
|--------|-------------|
| 880 | EPAS steering torque frame — echoed with nag bit cleared |
| 921 | `DAS_status` — monitored, not modified |

> Signal names sourced from [tesla-can-explorer](https://github.com/mikegapinski/tesla-can-explorer) by [@mikegapinski](https://x.com/mikegapinski).

## Development & Testing

### Project structure

```
include/
  app.h                   # appSetup<Driver>() / appLoop<Driver>() templates
  handlers.h              # HW3Handler (CAN-A), NagHandler (CAN-B)
  can_helpers.h           # Runtime toggles, diagnostics, checksum helpers
  t2can_pins.h            # T2-CAN pin definitions
  event_log.h             # BUS-OFF event ring buffer
  timeseries.h            # 5 s interval timeseries collector
  drivers/
    mcp2515_driver.h      # CAN-A SPI driver
    twai_driver.h         # CAN-B TWAI driver
    mock_driver.h         # Mock driver for unit tests
  web/
    web_server.h          # 45 HTTP handler functions
    web_ui.h              # Embedded UI and OTA recovery UI
web/
  web_ui.html             # Editable source for the normal Web UI
src/
  main.cpp                # setup() / loop() / nagKillerTask / canAlertTask
scripts/
  gen_box2.py             # Unicode-aware comment box alignment generator
  tcan_signal_detail.py   # T-CAN signal detail and bit-layout renderer
  tcan_signal_observer_json.py # Uploadable Signal Observer JSON generator
test/
  test_native_nag/        # NagHandler unit tests (31 tests)
  test_native_helpers/    # Bit manipulation helpers
  test_native_hw3/        # HW3Handler tests
  test_native_log_buffer/ # Log ring buffer tests
```

### Build & test commands

```bash
# Build
pio run -e lilygo_t2can

# Upload
pio run -e lilygo_t2can -t upload

# Core nag tests
pio test -e native_nag

# All native tests
pio test -e native_nag -e native

# Check Web UI source/header sync and release metadata
python3 scripts/sync_web_ui.py --check
python3 scripts/check_release_metadata.py
```

## Versioning

- The project version is tracked in [`VERSION`](VERSION) using Semantic Versioning.
- Release notes are tracked in [`CHANGELOG.md`](CHANGELOG.md).
- Ongoing work should be added to the `Unreleased` section before merge.

## Third-Party Libraries

Full license texts are in [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES).

| Library | License | Copyright |
|---------|---------|-----------|
| [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) | MIT | (c) 2013 Seeed Technology Inc., (c) 2016 Dmitry |
| [espressif/esp-idf](https://github.com/espressif/esp-idf) (TWAI driver) | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## License

This project is licensed under the **GNU General Public License v3.0** — see the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html) for details.
