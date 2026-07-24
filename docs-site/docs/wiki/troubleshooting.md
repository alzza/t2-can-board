---
sidebar_position: 2
---

# Troubleshooting

## Board Not Detected

**Feather boards:** Double-press the **Reset** button to enter the UF2 bootloader, then retry the upload command.

**ESP32 boards:** Hold the **BOOT** button during upload if auto-reset does not work.

## Serial 출력이 보이지 않을 때

- Serial Monitor 속도를 **115200 baud**로 설정합니다.
- 정상 운행 중에는 주기 디버그 출력이 없으므로 출력이 적은 것이 정상입니다.
- 부팅 직후에도 출력이 없다면 충전 전용이 아닌 데이터 USB 케이블인지 확인합니다.
- 상세 상태는 Web UI 진단 화면과 CSV/전체 로그 저장에서 확인합니다.

## CAN Communication Errors

- **Check termination:** Make sure you cut/removed the onboard 120 Ohm termination resistor
- **Check wiring:** CAN-H and CAN-L must not be swapped
- **Check baud rate:** The vehicle uses 500 kbit/s — this is set automatically by the firmware

## FSD Not Activating

1. Verify you have an active FSD subscription or purchase
2. Check that "Traffic Light and Stop Sign Control" is enabled in Autopilot settings (unless using `BYPASS_TLSSC_REQUIREMENT`)
3. Open the Serial Monitor and check if the handler output shows `FSD: 1`
4. Verify you selected the correct vehicle variant in `sketch_config.h`
5. HW4 on FSD v13: Make sure you compiled with `HW3`, not `HW4`

## Build Errors

### Board/driver mismatch

If the PlatformIO `-e` environment doesn't match the `DRIVER_*` define in `sketch_config.h`, the build will fail with a clear error. Make sure they match.

### Missing libraries

- **Feather RP2040:** Install **MCP2515 by autowp** in Arduino IDE
- **Feather M4:** Install **Adafruit CAN** in Arduino IDE
- **ESP32:** No additional libraries needed

### Native test compilation fails (Windows)

Install MinGW-w64 GCC:
```bash
winget install BrechtSanders.WinLibs.POSIX.UCRT
```
Restart your terminal so `gcc` is on PATH.

## Speed Profile Not Changing

- Make sure you're adjusting the **follow distance** on the steering wheel stalk, not cruise speed
- Check Serial Monitor output to see if the distance value changes
- Verify the correct vehicle variant is selected (HW4 has 5 levels, HW3 has 3)
