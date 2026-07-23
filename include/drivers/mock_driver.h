#pragma once

#include <vector>
#include "../can_frame_types.h"
#include "can_driver.h"

class MockDriver : public CanDriver
{
public:
    static constexpr bool kSupportsISR = false;

    std::vector<CanFrame> sent;
    bool sendSucceeds = true;

    bool init() override { return true; }
    void setFilters(const uint32_t * /*ids*/, uint8_t /*count*/) override {}
    bool enableInterrupt(void (* /*onReady*/)()) override { return false; }

    bool read(CanFrame & /*frame*/) override
    {
        return false;
    }

    void send(const CanFrame &frame) override
    {
        (void)sendCheck(frame);
    }

    bool sendCheck(const CanFrame &frame) override
    {
        if (!sendSucceeds) return false;
        sent.push_back(frame);
        return true;
    }

    void reset()
    {
        sent.clear();
        sendSucceeds = true;
    }
};
