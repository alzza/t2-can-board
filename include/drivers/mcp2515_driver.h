#pragma once

#include <memory>
#include <SPI.h>
#include <mcp2515.h>
#include "../can_frame_types.h"
#include "../can_helpers.h"
#include "../event_log.h"
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
        if (txQuiesced_) return;
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

    uint8_t drainReceived(CanFrame *frames, uint8_t capacity) override
    {
        if (!frames || capacity == 0) return 0;
        Lock lock(mutex_);
        uint8_t count = 0;
        while (count < capacity) {
            // getStatus(): bit0=RX0IF, bit1=RX1IF. RXB0를 먼저 비운 뒤
            // 다시 상태를 확인해 회수 중 RXB1로 rollover된 프레임도 읽는다.
            const uint8_t status = mcp_->getStatus();
            MCP2515::RXBn source;
            if (status & 0x01U) source = MCP2515::RXB0;
            else if (status & 0x02U) source = MCP2515::RXB1;
            else break;

            can_frame raw;
            if (mcp_->readMessage(source, &raw) != MCP2515::ERROR_OK) break;
            CanFrame &frame = frames[count++];
            frame.id = raw.can_id;
            frame.dlc = raw.can_dlc;
            memcpy(frame.data, raw.data, frame.dlc > 8 ? 8 : frame.dlc);
            if (source == MCP2515::RXB0)
                aChannelDiag.aRxBuffer0Frames = (uint32_t)aChannelDiag.aRxBuffer0Frames + 1U;
            else
                aChannelDiag.aRxBuffer1Frames = (uint32_t)aChannelDiag.aRxBuffer1Frames + 1U;
        }
        return count;
    }

    void send(const CanFrame &frame) override
    {
        (void)sendDetailed(frame, CanTxSource::Unknown);
    }

    // 프레임 송신 + 결과 반환. ERROR_OK=true, ALLTXBUSY/FAILTX=false.
    // 진단용: HW3Handler 에서 TX 성공/실패 카운트 추적.
    bool sendCheck(const CanFrame &frame) override
    {
        return canTxQueued(sendDetailed(frame, CanTxSource::Unknown));
    }

    CanTxResult sendDetailed(const CanFrame &frame,
                             CanTxSource source = CanTxSource::Unknown) override
    {
        Lock lock(mutex_);
        if (txQuiesced_) return CanTxResult::Aborted;
        if (frame.dlc > 8) return CanTxResult::InvalidFrame;

        pollTransmitResultsUnlocked();

        can_frame raw;
        raw.can_id = frame.id;
        raw.can_dlc = frame.dlc;
        memcpy(raw.data, frame.data, 8);

        for (uint8_t i = 0; i < 3; ++i) {
            const uint8_t ctrl = readRegisterUnlocked(kTxCtrlRegisters[i]);
            if ((ctrl & kTxReqMask) != 0) continue;

            // 이전 송신 결과 sticky 비트를 지운 뒤 지정 TX 버퍼에 등록한다.
            bitModifyUnlocked(kTxCtrlRegisters[i], kTxResultMask, 0);
            const MCP2515::ERROR result =
                mcp_->sendMessage(static_cast<MCP2515::TXBn>(i), &raw);
            if (result == MCP2515::ERROR_OK) {
                txPending_[i] = true;
                txPendingSource_[i] = source;
                return CanTxResult::Queued;
            }
            if (result == MCP2515::ERROR_ALLTXBUSY) return CanTxResult::Busy;
            // autowp sendMessage(TXBn)는 TXREQ 설정 직후 결과 비트를 읽는다.
            // 실차에서 MLOA/ABTF와 TXREQ가 동시에 관측됐으므로 이 순간값을
            // 하드 오류로 확정하지 않고, 진행 중이면 정상 pending으로 회수한다.
            const uint8_t resultCtrl = readRegisterUnlocked(kTxCtrlRegisters[i]);
            if ((resultCtrl & kTxReqMask) != 0) {
                txPending_[i] = true;
                txPendingSource_[i] = source;
                return CanTxResult::Queued;
            }
            return classifyFinishedTxUnlocked(source, false, i, resultCtrl,
                                              (uint8_t)result);
        }
        return CanTxResult::Busy;
    }

    void pollTransmitResults() override
    {
        Lock lock(mutex_);
        pollTransmitResultsUnlocked();
    }

    bool quiesceTransmit() override
    {
        Lock lock(mutex_);
        if (txQuiesced_) return true;

        // ABAT로 진행 중인 세 TX 버퍼를 모두 중단한 뒤 Listen-Only로 전환한다.
        // OTA 실패 시에도 이 상태를 유지하며 재부팅만이 Normal mode로 복귀시킨다.
        bitModifyUnlocked(kCanCtrlRegister, kAbortAllTxMask, kAbortAllTxMask);
        const MCP2515::ERROR modeResult = mcp_->setListenOnlyMode();
        for (bool &pending : txPending_) pending = false;
        txQuiesced_ = true;
        return modeResult == MCP2515::ERROR_OK;
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

    // EFLG.RX0OVR/RX1OVR와 ERRIF만 클리어한다.
    // 라이브러리의 clearRXnOVR()는 CANINTF 전체를 0으로 만들어, 이 순간 새로
    // 도착한 RX0IF/RX1IF까지 지우고 수신 버퍼를 읽지 못하게 할 수 있다.
    void clearRxOverrun() override
    {
        Lock lock(mutex_);
        mcp_->clearRXnOVRFlags();
        mcp_->clearERRIF();
    }

    bool recoverBusOff() override
    {
        Lock lock(mutex_);
        if (txQuiesced_) return false;
        pulseResetPinUnlocked();
        mcp_->clearMERR();
        mcp_->clearERRIF();
        mcp_->clearInterrupts();
        return configureChipUnlocked(false);
    }

private:
    static constexpr uint8_t kSpiReadInstruction = 0x03;
    static constexpr uint8_t kSpiBitModifyInstruction = 0x05;
    static constexpr uint8_t kCanCtrlRegister = 0x0F;
    static constexpr uint8_t kAbortAllTxMask = 0x10;
    static constexpr uint8_t kTxReqMask = 0x08;
    static constexpr uint8_t kTxAbortedMask = 0x40;
    static constexpr uint8_t kTxArbitrationLostMask = 0x20;
    static constexpr uint8_t kTxErrorMask = 0x10;
    static constexpr uint8_t kTxResultMask =
        kTxAbortedMask | kTxArbitrationLostMask | kTxErrorMask;
    static constexpr uint8_t kTxCtrlRegisters[3] = {0x30, 0x40, 0x50};

    uint8_t readRegisterUnlocked(uint8_t reg)
    {
        SPI.beginTransaction(SPISettings(currentSpiFreqHz_, MSBFIRST, SPI_MODE0));
        digitalWrite(csPin_, LOW);
        SPI.transfer(kSpiReadInstruction);
        SPI.transfer(reg);
        const uint8_t value = SPI.transfer(0x00);
        digitalWrite(csPin_, HIGH);
        SPI.endTransaction();
        return value;
    }

    void bitModifyUnlocked(uint8_t reg, uint8_t mask, uint8_t data)
    {
        SPI.beginTransaction(SPISettings(currentSpiFreqHz_, MSBFIRST, SPI_MODE0));
        digitalWrite(csPin_, LOW);
        SPI.transfer(kSpiBitModifyInstruction);
        SPI.transfer(reg);
        SPI.transfer(mask);
        SPI.transfer(data);
        digitalWrite(csPin_, HIGH);
        SPI.endTransaction();
    }

    void pollTransmitResultsUnlocked()
    {
        for (uint8_t i = 0; i < 3; ++i) {
            if (!txPending_[i]) continue;
            const uint8_t ctrl = readRegisterUnlocked(kTxCtrlRegisters[i]);
            if ((ctrl & kTxReqMask) != 0) continue;

            txPending_[i] = false;
            (void)classifyFinishedTxUnlocked(txPendingSource_[i], true, i, ctrl, 0);
            txPendingSource_[i] = CanTxSource::Unknown;
            bitModifyUnlocked(kTxCtrlRegisters[i], kTxResultMask, 0);
        }
    }

    void incrementSourceOutcomeUnlocked(CanTxSource source, CanTxResult result)
    {
        Shared<uint32_t> *counter = nullptr;
        if (result == CanTxResult::Queued) {
            if (source == CanTxSource::Summon) counter = &aChannelDiag.aTxCompletedSummon;
            else if (source == CanTxSource::Tsllc) counter = &aChannelDiag.aTxCompletedTsllc;
            else counter = &aChannelDiag.aTxCompletedOther;
        } else if (result == CanTxResult::ArbitrationLost) {
            if (source == CanTxSource::Summon) counter = &aChannelDiag.aTxArbitrationLostSummon;
            else if (source == CanTxSource::Tsllc) counter = &aChannelDiag.aTxArbitrationLostTsllc;
            else counter = &aChannelDiag.aTxArbitrationLostOther;
        } else if (result == CanTxResult::Aborted) {
            if (source == CanTxSource::Summon) counter = &aChannelDiag.aTxAbortedSummon;
            else if (source == CanTxSource::Tsllc) counter = &aChannelDiag.aTxAbortedTsllc;
            else counter = &aChannelDiag.aTxAbortedOther;
        }
        if (counter) *counter = (uint32_t)(*counter) + 1U;
    }

    CanTxResult classifyFinishedTxUnlocked(CanTxSource source, bool polledResult,
                                           uint8_t buffer, uint8_t ctrl,
                                           uint8_t driverCode)
    {
        if ((ctrl & kTxErrorMask) != 0 ||
            ((ctrl & kTxResultMask) == 0 && driverCode != 0)) {
            aChannelDiag.aTxFail = (uint32_t)aChannelDiag.aTxFail + 1U;
            recordTxFailureUnlocked(source, polledResult, buffer, ctrl, driverCode);
            return CanTxResult::ControllerError;
        }
        if ((ctrl & kTxArbitrationLostMask) != 0) {
            aChannelDiag.aTxArbitrationLost =
                (uint32_t)aChannelDiag.aTxArbitrationLost + 1U;
            incrementSourceOutcomeUnlocked(source, CanTxResult::ArbitrationLost);
            return CanTxResult::ArbitrationLost;
        }
        if ((ctrl & kTxAbortedMask) != 0) {
            aChannelDiag.aTxAborted = (uint32_t)aChannelDiag.aTxAborted + 1U;
            incrementSourceOutcomeUnlocked(source, CanTxResult::Aborted);
            return CanTxResult::Aborted;
        }
        aChannelDiag.aTxCompleted = (uint32_t)aChannelDiag.aTxCompleted + 1U;
        incrementSourceOutcomeUnlocked(source, CanTxResult::Queued);
        return CanTxResult::Queued;
    }

    void recordTxFailureUnlocked(CanTxSource source, bool polledResult, uint8_t buffer,
                                 uint8_t ctrl, uint8_t driverCode)
    {
        const uint8_t sourceValue = (uint8_t)source;
        if (source == CanTxSource::Summon) {
            aChannelDiag.aTxFailSummon = (uint32_t)aChannelDiag.aTxFailSummon + 1U;
        } else if (source == CanTxSource::Tsllc) {
            aChannelDiag.aTxFailTsllc = (uint32_t)aChannelDiag.aTxFailTsllc + 1U;
        } else {
            aChannelDiag.aTxFailOther = (uint32_t)aChannelDiag.aTxFailOther + 1U;
        }
        eventLogPush(EV_A_TX_FAILURE,
                     (uint16_t)(uint8_t)aChannelDiag.aTec,
                     (uint16_t)(uint8_t)aChannelDiag.aRec,
                     eventATxFailureDetail(sourceValue, polledResult, buffer, ctrl, driverCode));
    }

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

        applyModeUnlocked();
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
    bool txPending_[3] = {};
    CanTxSource txPendingSource_[3] = {};
    bool txQuiesced_{false};
#ifndef NATIVE_BUILD
    SemaphoreHandle_t mutex_{nullptr};
#else
    void* mutex_{nullptr};
#endif
};
