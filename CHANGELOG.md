# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-04-05

### Added
- FSD activation bypass for HW3 and HW4 vehicles
- Autosteer nag suppression via CAN frame manipulation
- Autosteer Nag Killer hardware support (wiring to X179 connector, CAN bus 4)
- ISA speed chime suppression
- Emergency vehicle detection and response
- Speed profiles support
- Smart summon support
- Web interface for ESP32-based boards (configuration, OTA updates)
- Hardware support: Adafruit Feather RP2040 CAN
- Hardware support: Adafruit Feather M4 CAN Express
- Hardware support: ESP32 (TWAI driver)
- Hardware support: ESP32 Feather V2 with MCP2515
- Hardware support: M5Stack Atomic CAN Base
- Hardware support: LILYGO T-CAN485
- NagHandler torque output fixed at safe value (1.80 Nm)
- Comprehensive test suite with native PlatformIO environments
- Documentation site at teslaopencanmod.org
