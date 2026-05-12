---
description: "Validate LilyGo T2-CAN pin map and clock settings for MCP2515/TWAI builds, then propose minimal safe fixes."
name: "T2CAN Pin Clock Check"
argument-hint: "Target board/profile and observed issue"
agent: "agent"
---
Check the current T2CAN build configuration and report only actionable mismatches.

Required checks:
1. Verify MCP2515 pins (CS/SCK/MISO/MOSI/RST) and TWAI pins (TX/RX) are consistent across configuration and code.
2. Verify MCP2515 crystal and SPI clock settings are coherent for the target hardware.
3. Verify build flags needed for dual-channel operation are present and not contradictory.
4. Flag only high-confidence risks (for example, likely bus-off or no-RX conditions).
5. Propose the minimum code/config changes required.

Output format:
- Current effective settings summary
- Confirmed mismatches
- Minimal patch plan
- Validation commands to run
