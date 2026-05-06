# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Dual-channel CAN polling unified on Core 1** — `nagKillerTask` (prio 10) now polls both CAN-A (MCP2515/SPI) and CAN-B (TWAI) in a single task loop, matching the Saturn Tesla CAN Debugger architecture
- **TWAI ISR IRAM flag** (`ESP_INTR_FLAG_IRAM`) — protects RX FIFO during OTA cache-disable window; requires `CONFIG_TWAI_ISR_IN_IRAM=y`
- **NagKiller Mode B** — smart FSM-based adaptive nag suppression for B-channel (TWAI); complements existing Mode A (Stealth PRNG)
- **NVS first-boot initialization** — on first flash, `canmod` namespace is wiped clean using `nvs_init_ok` sentinel key; prevents stale NVS state from affecting behaviour after firmware update
- **BUS-OFF event log** — ring buffer recording TEC/REC at BUS-OFF, recovery duration, and time-since-last event; downloadable as CSV via `/api/busoff-log-dl`
- **Timeseries data collection** — 5-second interval sampling of B-channel Hz, error counters, and decision distribution; 30-minute rolling window, CSV download via `/api/timeseries-csv`
- **`canAlertTask`** (Core 0, prio 1, 20 ms) — decouples TWAI alert polling from hot-path `nagKillerTask`
- **`statusLogTask`** (Core 0, prio 1, 5 s) — periodic status summary log; disable with `T2CAN_STATUS_LOG_TASK=0` build flag
- **OTA recovery mode** (`pending=5`) — CAN stack skipped entirely, only web server active; dedicated recovery UI served from `WEB_RECOVERY_UI_HTML`
- **OTA watchdog task** — auto-rollback on confirmation timeout (3 min for new FW, 1 min for recovery FW)
- **Runtime serial log toggle** — `POST /api/enable-print`; default off (`T2CAN_RUNTIME_SERIAL_LOGS=0`)
- **Web UI features checkbox sync fix** — `tASpi8`, `tAOneShot`, `tATxGuard` checkboxes now correctly reflect server state via `d.features.*` in `poll()`

### Changed

- **`loop()` simplified to `vTaskDelete(NULL)`** — loopTask is deleted immediately after `setup()`, freeing ~8 KB stack RAM at runtime; all work is handled by RTOS tasks
- **MCP2515 SPI default frequency raised to 10 MHz** — matches datasheet maximum rating and Saturn project default; 8 MHz fallback still available via web UI toggle (`/api/a-spi-8mhz`)
- **Core assignment clarified in `src/main.cpp` header** — `nagKillerTask` runs on Core 1 at prio 10; the priority belongs to the task, not to the core

### Fixed

- Web UI `tASpi8` / `tAOneShot` / `tATxGuard` checkboxes out of sync with server state after page reload

## [1.1.0] - 2026-04-06

### Added

- Added m5stack-atoms3-mini-can-base as new ESP32 board
- Added enhanced autopilot to enable summon related features and surpress some nags

### Fixed

- HW3Handler: removed obsolete speed-offset-to-profile mapping that overwrote stalk-derived `speedProfile`
- NagHandler: fixed incomplete torque override in echo frame — `data[2]` lower nibble now set to `0x08` to match fixed torque raw value `0x08B6` (1.80 Nm)
- Fixed webui with the new features

## [1.0.0] - 2026-04-05

### Added

- FSD activation bypass for HW3 and HW4 vehicles
- `BYPASS_TLSSC_REQUIREMENT` build flag to bypass Tesla Live Service SC requirement for regions without traffic light toggle
- Autosteer nag suppression via CAN frame interception
- Autosteer Nag Killer hardware mode: echoes CAN frame 0x370 with counter+1 to suppress nag at hardware level (X179 connector, CAN bus 4)
- ISA speed chime suppression for HW3 and HW4
- Emergency vehicle detection and response
- Speed profiles support (distance control stalk mapped)
- Smart Summon support (EU region restriction removed)
- ESP32 web dashboard for live CAN status and runtime settings
- OTA firmware updates via web interface for ESP32 boards
- Hardware support: Adafruit Feather RP2040 CAN
- Hardware support: Adafruit Feather M4 CAN Express (tested)
- Hardware support: ESP32 with TWAI driver
- Hardware support: Adafruit Feather ESP32 V2 with MCP2515 CAN Featherwing
- Hardware support: M5Stack Atomic CAN Base
- Hardware support: LILYGO T-CAN485
- CAN driver abstraction layer (MCP2515, SAME51, TWAI)
- TWAI driver: non-blocking TX, bus-off cooldown and recovery, driver-fail guard
- TWAI driver: DLC clamped to 8 bytes on read and send
- DLC validation guards on all CAN frame handlers
- Bounds check in `setBit()` to prevent buffer overrun
- STL case model for Feather RP2040 CAN
- FSD subscription guide for unsupported regions (Canadian account method)
- Wiring guide for Tesla Model 3/Y including legacy connector pinouts
- Comprehensive NagHandler unit test suite

### Fixed

- Nag handler torque value: output is now fixed at safe 1.80 Nm (0x08B6) instead of copying torque from the original frame
- FSDEnabled variable shadowing bug in HW3 and HW4 handlers
- TWAI TX timeout changed from 0 ms to 2 ms to avoid bus starvation

### Changed

- Build flag renamed from `FORCE_FSD` / `FORCE_FSC` to `BYPASS_TLSSC_REQUIREMENT` for clarity
- Arduino sketch renamed from `canFeather.ino` to `RP2040CAN.ino` for multi-board support
- Firmware configuration consolidated: all user-selectable options moved to `sketch_config.h`
