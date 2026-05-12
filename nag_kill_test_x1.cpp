/**
 * NagHandler — DAS-aware autosteer nag suppression (counter+1 echo method)
 *
 * Listens for two CAN frames:
 *   - 880  (0x370) EPAS3P_sysStatus  — steering torque / handsOnLevel source
 *   - 921  (0x399) DAS_status        — DAS hands-on demand state
 *
 * When DAS is actively requesting hands-on (dasHandsOnState != LC_HANDS_ON_NOT_REQD):
 *   1. Copies the real EPAS frame
 *   2. Sets byte 3 = 0xB6 (fixed torsionBarTorque = 1.80 Nm)
 *   3. Sets byte 4 |= 0x40 (handsOnLevel = 1)
 *   4. Increments counter (byte 6 lower nibble + 1)
 *   5. Recalculates checksum (byte 7 = sum(b0..b6) + 0x73)
 *
 * Improvement over the naive approach (echo whenever EPAS handsOnLevel=0):
 *   EPAS naturally reports handsOnLevel=0 during normal AP driving with no nag
 *   pending — that is the resting state. Echoing unconditionally injects ~25
 *   unsolicited frames/s onto the bus during normal driving. By gating on
 *   DAS_autopilotHandsOnState from 0x399, we only inject when DAS is actually
 *   demanding a response, producing zero spurious traffic during satisfied driving.
 *
 * DAS_autopilotHandsOnState (42|4@1+ LE in 0x399): (d[5] >> 2) & 0x0F
 *   0 = LC_HANDS_ON_NOT_REQD  — DAS satisfied, no nag: echo suppressed
 *   1 = LC_HANDS_ON_REQD_DETECTED
 *   2 = LC_HANDS_ON_REQD_NOT_DETECTED — orange nag: echo fires
 *   3 = LC_HANDS_ON_REQD_VISUAL
 *   4 = LC_HANDS_ON_REQD_CHIME_1
 *   5 = LC_HANDS_ON_REQD_CHIME_2
 *   6 = LC_HANDS_ON_REQD_SLOWING
 *   7 = LC_HANDS_ON_REQD_STRUCK_OUT
 *   8 = LC_HANDS_ON_SUSPENDED     — AP paused, not struck: echo suppressed
 *   9 = LC_HANDS_ON_REQD_ESCALATED_CHIME_1
 *  10 = LC_HANDS_ON_REQD_ESCALATED_CHIME_2
 *  15 = LC_HANDS_ON_SNA
 *
 * Tested: Model X HW3, Legacy platform, FW 2026.8.3
 * Bus: X179 pin 18/19 (Chassis CAN)
 *
 * Enable with build flag: -D NAG_KILLER
 * 
 * 
 * https://github.com/zdenekbouresh/ev-open-can-tools/blob/feat/das-aware-nag-suppression/include/handlers.h
 */
struct NagHandler : public CarManagerBase
{
    Shared<bool> nagKillerActive{true};
    Shared<uint32_t> nagEchoCount{0};
    // Last decoded DAS_autopilotHandsOnState from 0x399.
    // Initialised to 0xFF (unseen) so the handler echoes conservatively
    // until the first DAS_status frame arrives — identical to the pre-DAS
    // behaviour for the brief startup window.
    Shared<uint8_t> dasHandsOnState{0xFF};

    // ── Organic torque variation state ────────────────────────────────────
    // Not atomic — only accessed from handleMessage() (serialised CAN context).
    // xorshift32 PRNG avoids Arduino's random() (safe for NATIVE_BUILD).
    uint32_t _prngState = 0xDEADBEEF;
    int16_t _torqWalk = 2230;       // raw starting point = 1.80 Nm
    uint8_t _excFrames = 0;         // frames remaining in grip excursion
    uint16_t _framesUntilExc = 175; // frames until next excursion (~7 s @ 25 Hz)

    const uint32_t *filterIds() const override
    {
        // 880 = 0x370 EPAS3P_sysStatus, 921 = 0x399 DAS_status
        static constexpr uint32_t ids[] = {880, 921};
        return ids;
    }
    uint8_t filterIdCount() const override { return 2; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        // ── 0x399 DAS_status: track DAS hands-on demand level ────────────────
        if (frame.id == 921)
        {
            if (frame.dlc >= 6)
                dasHandsOnState = (frame.data[5] >> 2) & 0x0F;
            return;
        }

        // ── 0x370 EPAS3P_sysStatus: conditional echo ─────────────────────────
        if (frame.id != 880 || frame.dlc < 8)
            return;

        uint8_t handsOn = (frame.data[4] >> 6) & 0x03;

        if (!nagKillerActive || !nagKillerRuntime || handsOn != 0)
            return;

        // Gate: only inject when DAS is actually requesting hands-on.
        // State 0 (NOT_REQD) and state 8 (SUSPENDED) mean DAS is satisfied —
        // no echo needed. 0xFF = no DAS frame seen yet, echo as fallback.
        uint8_t dasState = dasHandsOnState;
        if (dasState == 0 || dasState == 8)
            return;

        // ── Organic torque variation ──────────────────────────────────────
        // Constant torque is trivially detectable in Tesla telemetry — a flat
        // torsionBarTorque signal for 30+ minutes is a statistical impossibility
        // from a real hand. We simulate muscle tremor and slow grip drift using
        // a smooth random walk with brief "tighten grip" excursions every 5–9 s.
        //
        // torsionBarTorque encoding (19|12@0+ Motorola, 0.01 Nm/LSB, -20.5 offset):
        //   tRaw = (Nm + 20.5) / 0.01    e.g. 1.80 Nm → 0x08B6
        //   d[2] lower nibble = tRaw >> 8,   d[3] = tRaw & 0xFF
        //
        // Normal walk: [2150–2290] ≈ [1.00–2.40 Nm] — light resting touch, handsOnLevel 1
        // Excursion:   2350 ± 20  ≈  [3.10–3.30 Nm] — brief grip pulse every ~5–9 s
        {
            uint32_t r = _prngState;
            r ^= r << 13;
            r ^= r >> 17;
            r ^= r << 5;
            _prngState = r;

            if (_excFrames > 0)
            {
                // Grip excursion — elevated torque for a few frames
                _torqWalk = static_cast<int16_t>(2350 + static_cast<int16_t>(r % 41) - 20);
                _excFrames--;
            }
            else
            {
                // Normal walk: step ±15 raw units per frame (±0.15 Nm per 40 ms)
                _torqWalk += static_cast<int16_t>(r % 31) - 15;
                if (_torqWalk < 2150)
                    _torqWalk = 2150;
                if (_torqWalk > 2290)
                    _torqWalk = 2290;
                if (_framesUntilExc == 0)
                {
                    _excFrames = 3 + static_cast<uint8_t>(r % 3);           // 3–5 frames ≈ 120–200 ms
                    _framesUntilExc = 125 + static_cast<uint16_t>(r % 101); // 125–225 frames ≈ 5–9 s @ 25 Hz
                }
                else
                {
                    _framesUntilExc--;
                }
            }
        }
        uint16_t torqRaw = static_cast<uint16_t>(_torqWalk);

        CanFrame echo;
        echo.id = 880;
        echo.dlc = 8;

        echo.data[0] = frame.data[0];
        echo.data[1] = frame.data[1];
        echo.data[2] = (frame.data[2] & 0xF0) | static_cast<uint8_t>(torqRaw >> 8);
        echo.data[3] = static_cast<uint8_t>(torqRaw & 0xFF);
        echo.data[5] = frame.data[5];

        // handsOnLevel = 1 — torque stays in level-1 range throughout walk
        echo.data[4] = frame.data[4] | 0x40;

        // Counter + 1
        uint8_t cnt = (frame.data[6] & 0x0F);
        cnt = (cnt + 1) & 0x0F;
        echo.data[6] = (frame.data[6] & 0xF0) | cnt;

        // Checksum: sum(byte0..byte6) + 0x73
        uint16_t sum = echo.data[0] + echo.data[1] + echo.data[2] + echo.data[3] + echo.data[4] + echo.data[5] + echo.data[6];
        echo.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);

        framesSent++;
        nagEchoCount++;
        driver.send(echo);

        if (enablePrint && (nagEchoCount % 500 == 1))
        {
            char buf[LogRingBuffer::kMaxMsgLen];
            snprintf(buf, sizeof(buf), "NagHandler: echo=%u das=%u",
                     (unsigned int)(uint32_t)nagEchoCount, (unsigned int)dasState);
            logRing.push(buf,
#ifndef NATIVE_BUILD
                         millis()
#else
                         0
#endif
            );
#ifndef NATIVE_BUILD
            Serial.println(buf);
#endif
        }
    }
};
