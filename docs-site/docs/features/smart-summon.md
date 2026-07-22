---
sidebar_position: 5
---

# Conditional Summon Unlock for HW3

The LILYGO T2-CAN HW3 build exposes a conditional Summon Unlock control through the WiFi dashboard. It is based on the validated `summon_unlock.ino` frame logic and does not transmit continuously.

## Conditions for CAN-A Transmission

The firmware modifies CAN ID `0x3FD` (decimal 1021), mux 1 only when every condition below is true.

- **Conditional Summon Unlock** is enabled in the dashboard.
- The **CAN-A TX master** is enabled.
- The vehicle state gate is open: `Parked || Summoning`.

AP state is shown for diagnosis but is not an injection condition. If the gate is closed, the firmware records a blocked event and sends no Summon-modified frame.

## HW3 Frame Change

| CAN ID | Mux | Bit | Value | Meaning |
|---|---|---|---|---|
| 1021 | 1 | 19 | 0 | Clear `UI_applyEceR79` |
| 1021 | 1 | 46 | 1 | HW3 Summon Unlock bit |

## Safe Use Sequence

1. Park the vehicle and open `http://192.168.4.1` after connecting to the `TeslaCAN` WiFi AP.
2. Confirm CAN-A RX is active and CAN-A reports no BUS-OFF condition.
3. Enable **Conditional Summon Unlock (HW3)** in **Controls**.
4. Enable the **CAN-A TX master** only after the receive diagnostics are stable.
5. Confirm the dashboard reports `Gate OPEN` only for `PARKED` or `SUMMONING`, then monitor `TX OK / Fail`, TEC, BUS-OFF, and the 280/390/921/1016 receive counters.

Turn off either the feature toggle or CAN-A TX master to stop Summon/TSLLC transmission. After an OTA update, the firmware begins with all vehicle-impacting features and CAN-A TX OFF; verify CAN receive health and confirm the firmware within the 60-second OTA window before enabling features one at a time.

:::caution
Tesla vehicle firmware and CAN behavior can change without notice. Use only while parked for private testing, and validate after every vehicle firmware update. This firmware does not create or alter Tesla service entitlements.
:::
