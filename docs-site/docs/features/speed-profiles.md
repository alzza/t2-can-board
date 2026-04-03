---
sidebar_position: 3
---

# Speed Profiles

The speed profile controls how aggressively the vehicle drives under FSD. It is derived from the follow-distance stalk setting and injected into the CAN message.

## Profile Mapping

### Legacy & HW3

| Follow Distance | Profile |
|---|---|
| 2 | Hurry |
| 3 | Normal |
| 4 | Chill |

### HW4

HW4 supports an extended 5-level speed profile range:

| Follow Distance | Profile |
|---|---|
| 2 | Max |
| 3 | Hurry |
| 4 | Normal |
| 5 | Chill |
| 6 | Sloth |

## CAN Message Details

### Reading the Follow Distance

| Variant | CAN ID | Signal | Bits | Values |
|---|---|---|---|---|
| Legacy | 69 (STW_ACTN_RQ) | Stalk | 13-15 | 0-7 |
| HW3 | 1016 (UI_driverAssistControl) | UI_accFollowDistanceSetting | 45-47 | 0-7 |
| HW4 | 1016 (UI_driverAssistControl) | UI_accFollowDistanceSetting | 45-47 | 0-7 |

### Injecting the Profile

| Variant | CAN ID | Mux | Bits | Values |
|---|---|---|---|---|
| Legacy | 1006 | 0 | 49-50 | 0-2 |
| HW3 | 1021 | 0 | 49-50 | 0-2 |
| HW4 | 1021 | 2 | 60-62 | 0-4 |

## How to Change the Speed Profile

While driving with FSD active, use the **follow distance** control on the steering wheel stalk to cycle through profiles. The change takes effect immediately.
