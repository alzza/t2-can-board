---
sidebar_position: 4
---

# M5Stack Atomic CAN Base

The most compact board option. Uses the CA-IS3050G CAN transceiver over the ESP32's built-in TWAI peripheral.

## Specifications

| Property | Value |
|---|---|
| CAN Controller | ESP32 TWAI via CA-IS3050G |
| Library | ESP32 TWAI |
| Driver Define | `DRIVER_TWAI` |
| Status | Tested |

## Configuration

In `sketch_config.h`:

```cpp
#define DRIVER_TWAI
```

## PlatformIO

The M5Stack has its own PlatformIO environment:

```bash
pio run -e m5stack-atomic-can-base
pio run -e m5stack-atomic-can-base --target upload
```

## Notes

- This board uses the same `DRIVER_TWAI` define as the generic ESP32 setup
- The CAN transceiver is integrated — no external transceiver module needed
- Very small form factor, making it easy to hide in the vehicle

For more details, see the [official M5Stack documentation](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base).
