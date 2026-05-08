#pragma once

#include <memory>
#include <SPI.h>
#include <mcp2515.h>
#include "../can_frame_types.h"
#include "../can_helpers.h"
#include "can_driver.h"

#ifndef NATIVE_BUILD
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

#if !defined(MCP2515_CRYSTAL_MHZ)
#define MCP2515_CRYSTAL_MHZ 16
#endif

#if MCP2515_CRYSTAL_MHZ == 8
static constexpr CAN_CLOCK kMcpClock = MCP_8MHZ;
#elif MCP2515_CRYSTAL_MHZ == 16
static constexpr CAN_CLOCK kMcpClock = MCP_16MHZ;
#else
#error "MCP2515_CRYSTAL_MHZ must be 8 or 16"
#endif
 
class MCP2515Driver : public CanDriver
{
    struct Lock {
#ifndef NATIVE_BUILD
        explicit Lock(SemaphoreHandle_t mutex) : mutex_(mutex)
        {
            if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
        }
        ~Lock()
        {
            if (mutex_) xSemaphoreGive(mutex_);
        }
        SemaphoreHandle_t mutex_;
#else
        explicit Lock(void*) {}
#endif
    };

public:
    // 폴링 방식으로 강제 지정 T-2Can일 때만 폴링(false), 아니면 원래대로(true)
    #ifdef BOARD_T2CAN
        static constexpr bool kSupportsISR = false;
    #else
        static constexpr bool kSupportsISR = true;
    #endif

    explicit MCP2515Driver(uint8_t csPin, int8_t rstPin = -1)
        : csPin_(csPin), rstPin_(rstPin), currentSpiFreqHz_((uint32_t)aMcpSpiFreqHz)
    {
        rebuildMcpUnlocked();
#ifndef NATIVE_BUILD
        mutex_ = xSemaphoreCreateMutex();
#endif
    }

    bool init() override
    {
        Lock lock(mutex_);
        return configureChipUnlocked(true);
    }

    void setFilters(const uint32_t *ids, uint8_t count) override
    {
        Lock lock(mutex_);
        mcp_->setConfigMode();
        mcp_->setFilterMask(MCP2515::MASK0, false, 0x7FF); // 11비트 ID 전체 비교 (표준 CAN)
        mcp_->setFilterMask(MCP2515::MASK1, false, 0x7FF); // 11비트 ID 전체 비교 (표준 CAN)
        // RXF0~5 전체를 명시적으로 설정:
        // 미설정 필터의 reset default(0x000)가 ID 0x000 프레임을 수신하는 버그 방지
        // 여분 슬롯은 ids[0]으로 채워 정상 ID와 동일하게 처리
        for (uint8_t i = 0; i < 6; i++) {
            uint32_t fid = (i < count) ? ids[i] : ids[0];
            mcp_->setFilter(static_cast<MCP2515::RXF>(MCP2515::RXF0 + i), false, fid);
        }
        applyModeUnlocked();
    }

    void applyRuntimeSettings() override
    {
        Lock lock(mutex_);
        applyModeUnlocked();
    }

    bool enableInterrupt(void (*onReady)()) override { return false; }

    bool read(CanFrame &frame) override
    {
        Lock lock(mutex_);
        can_frame raw;
        if (mcp_->readMessage(&raw) != MCP2515::ERROR_OK)
            return false;

        frame.id = raw.can_id;
        frame.dlc = raw.can_dlc;
        memcpy(frame.data, raw.data, frame.dlc > 8 ? 8 : frame.dlc);
        return true;
    }

    void send(const CanFrame &frame) override
    {
        Lock lock(mutex_);
        can_frame raw;
        raw.can_id = frame.id;
        raw.can_dlc = frame.dlc;
        memcpy(raw.data, frame.data, 8);
        mcp_->sendMessage(&raw);
    }

    // 프레임 송신 + 결과 반환. ERROR_OK=true, ALLTXBUSY/FAILTX=false.
    // 진단용: HW3Handler 에서 TX 성공/실패 카운트 추적.
    bool sendCheck(const CanFrame &frame) override
    {
        Lock lock(mutex_);
        can_frame raw;
        raw.can_id = frame.id;
        raw.can_dlc = frame.dlc;
        memcpy(raw.data, frame.data, 8);
        return mcp_->sendMessage(&raw) == MCP2515::ERROR_OK;
    }

    // EFLG 레지스터 읽기 — Normal Mode 유지, 통신 중단 없음
    // bit5=TXBO(BUS-OFF), bit4=TXEP(TX에러패시브≥128), bit2=TXWAR(TEC≥96)
    // bit7=RX1OVR, bit6=RX0OVR
    uint8_t getErrorFlags() override
    {
        Lock lock(mutex_);
        return mcp_->getErrorFlags();
    }

    // TEC/REC 카운터 읽기 (0~255). 진단용 상시 폴링 안전.
    void getErrorCounters(uint8_t &tec, uint8_t &rec) override
    {
        Lock lock(mutex_);
        tec = mcp_->errorCountTX();
        rec = mcp_->errorCountRX();
    }

    // CANINTF.MERRF (Message Error: ACK/Bit/Stuff) — set이면 1 반환 + 클리어.
    // ACK 부재(상대 노드 미응답)나 동일 ID 충돌 진단의 핵심 신호.
    uint8_t readAndClearMerrf() override
    {
        Lock lock(mutex_);
        uint8_t intf = mcp_->getInterrupts();        // CANINTF 읽기
        if (intf & MCP2515::CANINTF_MERRF) {
            mcp_->clearMERR();                        // CANINTF.MERRF=0 (W1C)
            return 1;
        }
        return 0;
    }

    // EFLG.RX0OVR/RX1OVR 클리어 — sticky 비트라 SW가 명시적으로 0 써야 함.
    void clearRxOverrun() override
    {
        Lock lock(mutex_);
        mcp_->clearRXnOVR();
    }

    bool recoverBusOff() override
    {
        Lock lock(mutex_);
        pulseResetPinUnlocked();
        mcp_->clearMERR();
        mcp_->clearERRIF();
        mcp_->clearInterrupts();
        return configureChipUnlocked(false);
    }

private:
    bool configureChipUnlocked(bool verbose)
    {
        mcp_->reset();

        MCP2515::ERROR e = mcp_->setBitrate(CAN_500KBPS, kMcpClock);
        if (e != MCP2515::ERROR_OK) {
            if (verbose) {
                Serial.print("[FAIL] MCP2515 setBitrate Error: ");
                Serial.println((int)e);
            }
            return false;
        }

        if (verbose) {
            Serial.printf("[OK] 500kbps @ %dMHz 설정 완료 (SPI %lu Hz)\n",
                  MCP2515_CRYSTAL_MHZ, (unsigned long)currentSpiFreqHz_);
        }

        applyModeUnlocked();
        if (verbose) {
            Serial.printf("[OK] %s Mode 진입 완료\n", (bool)aMcpOneShotRuntime ? "Normal One-Shot" : "Normal");
        }
        return true;
    }

    void pulseResetPinUnlocked()
    {
#ifndef NATIVE_BUILD
        if (rstPin_ < 0) return;
        pinMode(rstPin_, OUTPUT);
        digitalWrite(rstPin_, HIGH); delay(20);
        digitalWrite(rstPin_, LOW);  delay(20);
        digitalWrite(rstPin_, HIGH); delay(20);
#endif
    }

    void rebuildMcpUnlocked()
    {
#if defined(BOARD_T2CAN)
        mcp_ = std::make_unique<MCP2515>(csPin_, currentSpiFreqHz_, &SPI);
#else
        mcp_ = std::make_unique<MCP2515>(csPin_, currentSpiFreqHz_);
#endif
    }

    void applyModeUnlocked()
    {
        if ((bool)aMcpOneShotRuntime) {
            mcp_->setNormalOneShotMode();
        } else {
            mcp_->setNormalMode();
        }
    }

    uint8_t csPin_;
    int8_t rstPin_;
    uint32_t currentSpiFreqHz_;
    std::unique_ptr<MCP2515> mcp_;
#ifndef NATIVE_BUILD
    SemaphoreHandle_t mutex_{nullptr};
#else
    void* mutex_{nullptr};
#endif
};