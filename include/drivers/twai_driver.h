#pragma once

#include "../can_frame_types.h"
#include "can_driver.h"
#include <driver/twai.h>

class TWAIDriver : public CanDriver
{
public:
    static constexpr bool kSupportsISR = false;

    TWAIDriver(gpio_num_t txPin, gpio_num_t rxPin)
        : txPin_(txPin), rxPin_(rxPin) {}

    bool init() override
    {
        g_config_ = TWAI_GENERAL_CONFIG_DEFAULT(txPin_, rxPin_, TWAI_MODE_NORMAL);
        g_config_.rx_queue_len = 32;
        g_config_.tx_queue_len = 16;
        // OTA/플래시 쓰기 중 cache disable 구간에서도 RX FIFO를 보호하기 위해
        // ISR을 IRAM에 둘 것을 요청. 실제 효과는 sdkconfig CONFIG_TWAI_ISR_IN_IRAM
        // 활성 시점에 발생하며, flag 자체는 미설정 환경에서도 안전하다.
        g_config_.intr_flags = ESP_INTR_FLAG_IRAM;

        t_config_ = TWAI_TIMING_CONFIG_500KBITS();
        f_config_ = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config_, &t_config_, &f_config_) != ESP_OK)
            return false;
        if (twai_start() != ESP_OK)
            return false;
        driverOK_ = true;
        return true;
    }

    void setFilters(const uint32_t *ids, uint8_t count) override
    {
        if (count == 0)
            return;

        uint32_t differ = 0;
        for (uint8_t i = 1; i < count; i++)
        {
            differ |= ids[0] ^ ids[i];
        }

        uint32_t base = ids[0] & ~differ;
        f_config_.acceptance_code = base << 21;
        f_config_.acceptance_mask = (differ << 21) | 0x001FFFFF;
        f_config_.single_filter = true;

        // twai_stop() disables the TWAI ISR before uninstall
        twai_stop();
        twai_driver_uninstall();
        if (twai_driver_install(&g_config_, &t_config_, &f_config_) != ESP_OK ||
            twai_start() != ESP_OK)
        {
            driverOK_ = false;
        }
    }

    bool enableInterrupt(void (* /*onReady*/)()) override { return false; }

    bool read(CanFrame &frame) override
    {
        if (!driverOK_)
        {
            tryRecover();
            return false;
        }

        twai_message_t msg;
        if (twai_receive(&msg, 0) != ESP_OK)
        {
            if (isBusOff())
                recoverWithCooldown();
            return false;
        }
        frame.id = msg.identifier;
        frame.dlc = (msg.data_length_code <= 8) ? msg.data_length_code : 8;
        memset(frame.data, 0, 8);
        memcpy(frame.data, msg.data, frame.dlc);
        return true;
    }

    void send(const CanFrame &frame) override
    {
        if (!driverOK_)
            return;

        twai_message_t msg = {};
        uint8_t dlc = (frame.dlc <= 8) ? frame.dlc : 8;
        msg.identifier = frame.id;
        msg.data_length_code = dlc;
        memcpy(msg.data, frame.data, dlc);

        // Short timeout (2ms): modified frames should not be dropped, but
        // long blocks (10ms) risk overflowing the 32-deep RX queue.
        // At 500kbps, ~8 frames arrive in 2ms — queue handles this fine.
        if (twai_transmit(&msg, pdMS_TO_TICKS(2)) != ESP_OK)
        {
            if (isBusOff())
                recoverWithCooldown();
        }
    }

    // ── 진단 getters (web API, timeseries, BUS-OFF 이벤트 로그에서 사용) ──
    bool     isDriverOK()              const { return driverOK_; }
    bool     isBusOffState()                 { return isBusOff(); }
    uint32_t getBusoffCount()          const { return busoffCount_; }
    uint32_t getRecoveryAttemptCount() const { return recoveryAttemptCount_; }
    uint32_t getRecoverySuccessCount() const { return recoverySuccessCount_; }
    uint32_t getRecoveryFailCount()    const { return recoveryFailCount_; }
    uint32_t getLastRecoveryDurationMs() const { return lastRecoveryDurationMs_; }
    uint32_t getMaxRecoveryDurationMs()  const { return maxRecoveryDurationMs_; }
    uint32_t getLastBusOffMs()         const { return lastBusOffMs_; }
    uint32_t getBusOffTec()            const { return busOffTec_; }
    uint32_t getBusOffRec()            const { return busOffRec_; }
    uint32_t getTxSuppressedCount()    const { return 0; }   // TX 백오프 비활성 (TWAI 안전 체크리스트)
    uint32_t getCooldownMs()           const { return cooldownMs_; }
    void     setCooldownMs(uint32_t ms)      { cooldownMs_ = ms; }

    // 4/10 정상 기준에서 비활성된 토글: 호환용 getter (web UI 표시용)
    bool     getSoftRecovery()             const { return false; }
    uint32_t getSoftRecoveryFallbackCount() const { return 0; }
    bool     getSingleShotTx()             const { return false; }
    bool     getBusOffStopSkip()           const { return false; }

    // v4.4 alert 폴링 — TEC/REC 동시 노출
    uint32_t pollAlerts(uint16_t &tec, uint16_t &rec)
    {
        uint32_t alerts = 0;
        if (twai_read_alerts(&alerts, 0) != ESP_OK) alerts = 0;
        twai_status_info_t st;
        if (twai_get_status_info(&st) == ESP_OK) {
            tec = (uint16_t)st.tx_error_counter;
            rec = (uint16_t)st.rx_error_counter;
        } else {
            tec = rec = 0;
        }
        return alerts;
    }

private:
    bool isBusOff()
    {
        twai_status_info_t status;
        if (twai_get_status_info(&status) != ESP_OK)
            return false;
        return status.state == TWAI_STATE_BUS_OFF;
    }

    // BUS-OFF 진입 시 TEC/REC 캡처 후 카운터 갱신
    void captureBusOffSnapshot()
    {
        twai_status_info_t st;
        if (twai_get_status_info(&st) == ESP_OK) {
            busOffTec_ = st.tx_error_counter;
            busOffRec_ = st.rx_error_counter;
        }
        lastBusOffMs_ = millis();
        busoffCount_++;
    }

    void recoverWithCooldown()
    {
        uint32_t now = millis();
        if (now - lastRecovery_ < cooldownMs_)
            return;
        lastRecovery_ = now;

        captureBusOffSnapshot();
        recoveryAttemptCount_++;
        uint32_t startMs = millis();

        twai_stop();
        twai_driver_uninstall();
        if (twai_driver_install(&g_config_, &t_config_, &f_config_) != ESP_OK ||
            twai_start() != ESP_OK)
        {
            driverOK_ = false;
            recoveryFailCount_++;
        } else {
            recoverySuccessCount_++;
            uint32_t dur = millis() - startMs;
            lastRecoveryDurationMs_ = dur;
            if (dur > maxRecoveryDurationMs_) maxRecoveryDurationMs_ = dur;
        }
    }

    void tryRecover()
    {
        uint32_t now = millis();
        if (now - lastRecovery_ < cooldownMs_ * 10)
            return;
        lastRecovery_ = now;

        recoveryAttemptCount_++;
        uint32_t startMs = millis();
        if (twai_driver_install(&g_config_, &t_config_, &f_config_) == ESP_OK &&
            twai_start() == ESP_OK)
        {
            driverOK_ = true;
            recoverySuccessCount_++;
            uint32_t dur = millis() - startMs;
            lastRecoveryDurationMs_ = dur;
            if (dur > maxRecoveryDurationMs_) maxRecoveryDurationMs_ = dur;
        } else {
            recoveryFailCount_++;
        }
    }

    gpio_num_t txPin_;
    gpio_num_t rxPin_;
    twai_general_config_t g_config_;
    twai_timing_config_t t_config_;
    twai_filter_config_t f_config_;
    bool     driverOK_ = false;
    uint32_t lastRecovery_ = 0;
    uint32_t cooldownMs_ = 1000;

    // BUS-OFF / 복구 진단 카운터 (web API/timeseries에서 폴링)
    uint32_t busoffCount_ = 0;
    uint32_t recoveryAttemptCount_ = 0;
    uint32_t recoverySuccessCount_ = 0;
    uint32_t recoveryFailCount_ = 0;
    uint32_t lastRecoveryDurationMs_ = 0;
    uint32_t maxRecoveryDurationMs_ = 0;
    uint32_t lastBusOffMs_ = 0;
    uint32_t busOffTec_ = 0;
    uint32_t busOffRec_ = 0;
};
