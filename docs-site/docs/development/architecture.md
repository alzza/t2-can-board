---
sidebar_position: 1
---

# Architecture

CanMod currently targets the LILYGO T2-CAN ESP32-S3 board through the PlatformIO `lilygo_t2can` environment. The firmware runs two physical CAN channels in one device and keeps the CAN hot path isolated from WiFi, HTTP, OTA, and log export work.

## Runtime Layout

| Core | Task | Priority | Role |
| --- | --- | ---: | --- |
| Core 1 | `nagKillerTask` | 10 | CAN hot path for both A and B channels |
| Core 1 | `loopTask` | 1 | Arduino default task, deleted after setup |
| Core 0 | ESP-IDF WiFi task | 23 | AP-only network stack for `TeslaCAN` |
| Core 0 | `esp_http_server` | internal | Dashboard, API, OTA, and log download |
| Core 0 | `canAlertTask` | 1 | TWAI alert polling every 20 ms |
| Core 0 | `statusLogTask` | 1 | Optional 5-second serial status summary |
| Core 0 | `timeseriesTask` | 1 | 5-second samples, 120-slot RAM ring buffer |

Core 1 owns the timing-sensitive CAN loops. Core 0 owns user interaction and background diagnostics. This split keeps dashboard polling and log downloads away from the A/B CAN polling loop.

## CAN Channels

**CAN-A** uses the external MCP2515 over SPI. It is initialized from `src/main.cpp`, then processed through `appLoop<MCP2515Driver>()` and `HW3Handler`. The active frame path is ID `0x3FD` / `1021`, with TSLLC and Enhanced Autopilot bit injection on the matching muxes. MCP2515 EFLG, TEC, REC, MERRF, RX overrun, TX guard, and TXBO recovery diagnostics are sampled once per second.

**CAN-B** uses the ESP32-S3 TWAI controller at 500 kbps. `TWAIDriver` keeps the hardware acceptance filter at accept-all, then `nagKillerTask` applies the runtime software filter for `880`, `921`, `923`, and `297`. `NagHandler` owns Mode A and Mode B echo decisions, checksum generation, Mode B phase timing, and diagnostic counters.

## TWAI Recovery

The TWAI code follows the ESP-IDF v4.4.x driver model used by Arduino-ESP32 2.0.17. Normal operation starts with `twai_driver_install()` and `twai_start()`. BUS-OFF recovery first uses `twai_initiate_recovery()`, waits for the driver to reach `Stopped`, then calls `twai_start()` again. If soft recovery cannot complete, `TWAIDriver` falls back to a hard reinstall path.

`ESP_INTR_FLAG_IRAM` is only set when `CONFIG_TWAI_ISR_IN_IRAM` is enabled. The default Arduino-ESP32 S3 sdkconfig usually leaves that option disabled, so the flag must not be set unconditionally.

## Web And Logging

The board has no SD card or persistent log file storage. The primary field log path is the Web UI save button, which downloads `/api/logs-bundle` as one text bundle. That bundle contains five sections.

1. Runtime log ring.
2. BUS-OFF event log.
3. Channel status snapshot.
4. Ten-minute time-series CSV from the 120-sample RAM ring.
5. Millisecond event CSV.

`/api/timeseries.csv` and `/api/events.csv` still exist as debug helper endpoints, but they are not the normal field collection path.

## Project Structure

```
include/
  app.h                   # CAN-A setup and loop template
  can_frame_types.h       # Portable CanFrame struct
  can_helpers.h           # Shared runtime settings and diagnostics
  event_log.h             # Millisecond event ring and CSV export
  handlers.h              # HW handlers and NagHandler
  timeseries.h            # 10-minute RAM time-series ring
  drivers/
    can_driver.h          # Abstract CanDriver interface
    mcp2515_driver.h      # CAN-A MCP2515 SPI driver
    twai_driver.h         # CAN-B ESP32-S3 TWAI driver
    mock_driver.h         # Mock driver for unit tests
  web/
    web_server.h          # ESP-IDF HTTP server handlers
    web_ui.h              # Single-file dashboard UI
src/
  main.cpp                # LILYGO T2-CAN entry point and task wiring
scripts/
  gen_box2.py             # East Asian width-safe C comment box generator
  platformio_set_ino_profile.py   # Switch shared board/vehicle/feature defines
  platformio_sync_ino_defines.py  # Sync shared sketch defines into PlatformIO envs
  platformio_native_env.py        # Add macOS native test compiler includes
test/
  test_native_helpers/    # Tests for bit manipulation helpers
  test_native_legacy/     # LegacyHandler tests
  test_native_hw3/        # HW3Handler tests
  test_native_hw4/        # HW4Handler tests
  test_native_twai/       # TWAI filter computation tests
```

## Driver Abstraction

The `CanDriver` interface defines the contract that board-specific drivers implement.

**Current T2-CAN drivers:**
- `MCP2515Driver` implements CAN-A over SPI.
- `TWAIDriver` implements CAN-B over ESP-IDF `driver/twai.h`.
- `MockDriver` supports native tests without hardware.

The handler layer receives the same `CanFrame` shape regardless of the physical driver.

## Handler Pattern

The A-channel path uses `HW3Handler` for the current `lilygo_t2can` build. `LegacyHandler` and `HW4Handler` remain covered by native tests, but they are not the default deployed path for this environment.

The B-channel path uses `NagHandler`. It processes EPAS/DAS/SCCM context frames and supports two runtime modes.

- Mode A uses stealth PRNG torque variation with DAS/AP gating.
- Mode B uses a Smart FSM with AP state, phase timing, steering angle, torque, and first-echo timing diagnostics.

All handlers inherit from `CarManagerBase` and share bit manipulation helpers from `can_helpers.h`.

## Entry Point

The active firmware entry point is `src/main.cpp`. It performs NVS and OTA boot checks, initializes CAN-A and CAN-B when not in OTA recovery mode, creates `nagKillerTask` on Core 1, starts the Web server on Core 0, then starts auxiliary diagnostic tasks.

The architecture box at the top of `src/main.cpp` is generated by `scripts/gen_box2.py` so Korean comments and box borders stay aligned by visual width.
