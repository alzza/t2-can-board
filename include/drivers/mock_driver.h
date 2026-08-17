#pragma once

#include <vector>
#include "../can_frame_types.h"
#include "can_driver.h"

class MockDriver : public CanDriver
{
public:
    static constexpr bool kSupportsISR = false;

    std::vector<CanFrame> sent;
    std::vector<CanTxSource> sentSources;
    std::vector<CanTxSource> canceledSources;
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

    CanTxResult sendDetailed(const CanFrame &frame,
                             CanTxSource source = CanTxSource::Unknown) override
    {
        if (!sendSucceeds) return CanTxResult::ControllerError;
        sent.push_back(frame);
        sentSources.push_back(source);
        return CanTxResult::Queued;
    }

    void cancelPendingTransmit(CanTxSource source) override
    {
        canceledSources.push_back(source);
    }

    void reset()
    {
        sent.clear();
        sentSources.clear();
        canceledSources.clear();
        sendSucceeds = true;
    }
};
