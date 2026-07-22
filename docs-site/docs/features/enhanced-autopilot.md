---
sidebar_position: 4
---

# Enhanced Autopilot Legacy Configuration

`ENHANCED_AUTOPILOT` is not used by the current LILYGO T2-CAN HW3 firmware. Its previous EAP setting, runtime variable, and NVS key were retired in favor of the separately controlled **Conditional Summon Unlock (HW3)** feature.

Use the [Conditional Summon Unlock for HW3](/docs/features/smart-summon) guide instead. The current build uses the `SUMMON_UNLOCK` build flag, the `/api/summon-unlock` API, and the `summon_unlock` NVS key.

:::note
Old `enh_autopilot` NVS values are intentionally not migrated. After updating, review the new Summon control and CAN-A TX master before enabling transmission.
:::
