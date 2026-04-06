#pragma once

#include <SPI.h>
#include <mcp2515.h>
#include "../can_frame_types.h"
#include "can_driver.h"
 
class MCP2515Driver : public CanDriver
{
public:
    // 폴링 방식으로 강제 지정    // [보호 조치] T-2Can일 때만 폴링(false), 아니면 원래대로(true)
    #ifdef BOARD_T2CAN
        static constexpr bool kSupportsISR = false;
    #else
        static constexpr bool kSupportsISR = true;
    #endif

    explicit MCP2515Driver(uint8_t csPin) : mcp_(csPin) {}

    bool init() override
    {
        mcp_.reset();

        // [핵심] T-2Can A채널 전용 크리스탈 16MHz 설정
        MCP2515::ERROR e = mcp_.setBitrate(CAN_500KBPS, MCP_16MHZ);

        if (e != MCP2515::ERROR_OK) {
            Serial.print("[FAIL] MCP2515 setBitrate Error: ");
            Serial.println((int)e);
            return false;
        }

        Serial.println("[OK] 500kbps @ 16MHz 설정 완료");

        mcp_.setNormalMode();
        Serial.println("[OK] Normal Mode 진입 완료");

        return true;
    }

    void setFilters(const uint32_t *ids, uint8_t count) override
    {
        mcp_.setConfigMode();
        mcp_.setFilterMask(MCP2515::MASK0, false, 0x7FF);
        mcp_.setFilter(MCP2515::RXF0, false, ids[0]);
        mcp_.setFilterMask(MCP2515::MASK1, false, 0x7FF);
        for (uint8_t i = 1; i < count && i < 6; i++) {
            mcp_.setFilter(static_cast<MCP2515::RXF>(MCP2515::RXF0 + i), false, ids[i]);
        }
        mcp_.setNormalMode();
    }

    bool enableInterrupt(void (*onReady)()) override { return false; }

    bool read(CanFrame &frame) override
    {
        can_frame raw;
        if (mcp_.readMessage(&raw) != MCP2515::ERROR_OK)
            return false;

        frame.id = raw.can_id;
        frame.dlc = raw.can_dlc;
        memcpy(frame.data, raw.data, frame.dlc > 8 ? 8 : frame.dlc);
        return true;
    }

    void send(const CanFrame &frame) override
    {
        can_frame raw;
        raw.can_id = frame.id;
        raw.can_dlc = frame.dlc;
        memcpy(raw.data, frame.data, 8);
        mcp_.sendMessage(&raw);
    }

private:
    MCP2515 mcp_;
};