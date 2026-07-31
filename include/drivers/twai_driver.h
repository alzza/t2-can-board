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
        txQuiesced_ = false;
        driverOK_ = false;
        lastInstallErr_ = ESP_OK;
        lastStartErr_ = ESP_OK;
        g_config_ = TWAI_GENERAL_CONFIG_DEFAULT(txPin_, rxPin_, TWAI_MODE_NORMAL);
        // B채널은 accept-all 수신 후 소프트웨어 필터를 적용한다. 실차의 순간
        // 버스트를 흡수하되 오래된 프레임 체류를 과도하게 늘리지 않도록 128로 둔다.
        g_config_.rx_queue_len = 128;
        g_config_.tx_queue_len = 16;
        // ESP_INTR_FLAG_IRAM은 CONFIG_TWAI_ISR_IN_IRAM이 켜진 빌드에서만 유효하다.
        // Arduino-ESP32 기본 S3 sdkconfig는 보통 비활성이라 무조건 설정하면
        // twai_driver_install()이 실패할 수 있다.
    #if defined(CONFIG_TWAI_ISR_IN_IRAM) && CONFIG_TWAI_ISR_IN_IRAM
        g_config_.intr_flags = ESP_INTR_FLAG_IRAM;
    #endif

        t_config_ = TWAI_TIMING_CONFIG_500KBITS();
        f_config_ = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        lastInstallErr_ = twai_driver_install(&g_config_, &t_config_, &f_config_);
        if (lastInstallErr_ != ESP_OK) {
            lastStartErr_ = lastInstallErr_;
            return false;
        }
        lastStartErr_ = twai_start();
        if (lastStartErr_ != ESP_OK) {
            twai_driver_uninstall();
            return false;
        }
        driverOK_ = true;
        return true;
    }

    void setFilters(const uint32_t *ids, uint8_t count) override
    {
        (void)ids;
        (void)count;
        // B채널은 HW acceptance filter를 재설치하지 않는다.
        // TWAI는 accept-all로 유지하고 nagKillerTask의 SW 필터에서 ID를 걸러야
        // 실행 중 stop/uninstall로 RX가 끊기거나 A/B 통합 폴링이 흔들리지 않는다.
    }

    bool enableInterrupt(void (* /*onReady*/)()) override { return false; }

    bool read(CanFrame &frame) override
    {
        if (!driverOK_)
        {
            tryRecover();
            return false;
        }

        if (recoveryInProgress_)
        {
            updateRecoveryProgress();
            return false;
        }

        twai_message_t msg;
        if (twai_receive(&msg, 0) != ESP_OK)
        {
            if (!updateRecoveryProgress() && isBusOff())
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
        (void)sendCheck(frame);
    }

    bool sendCheck(const CanFrame &frame) override
    {
        if (txQuiesced_ || !driverOK_ || recoveryInProgress_) {
            txSuppressedCount_++;
            if (recoveryInProgress_) updateRecoveryProgress();
            return false;
        }

        twai_message_t msg = {};
        uint8_t dlc = (frame.dlc <= 8) ? frame.dlc : 8;
        msg.identifier = frame.id;
        msg.data_length_code = dlc;
        memcpy(msg.data, frame.data, dlc);

        // Short timeout (2ms): modified frames should not be dropped, but
        // long blocks (10ms) risk building up the RX queue.
        // At 500kbps, ~8 frames arrive in 2ms — 128-deep queue handles this fine.
        if (twai_transmit(&msg, pdMS_TO_TICKS(2)) != ESP_OK)
        {
            txSuppressedCount_++;
            if (!updateRecoveryProgress() && isBusOff())
                recoverWithCooldown();
            return false;
        }
        return true;
    }

    bool quiesceTransmit() override
    {
        if (txQuiesced_) return true;
        txQuiesced_ = true;
        (void)twai_clear_transmit_queue();
        const esp_err_t stopErr = twai_stop();
        driverOK_ = false;
        recoveryInProgress_ = false;
        return stopErr == ESP_OK || stopErr == ESP_ERR_INVALID_STATE;
    }

    // ── 진단 getters (web API, timeseries, BUS-OFF 이벤트 로그에서 사용) ──
    bool     isDriverOK()              const { return driverOK_; }
    bool     isBusOffState()                 { return isBusOff(); }
    uint8_t  getStateCode()                  { return stateCode(); }
    uint32_t getBusoffCount()          const { return busoffCount_; }
    uint32_t getRecoveryAttemptCount() const { return recoveryAttemptCount_; }
    uint32_t getRecoverySuccessCount() const { return recoverySuccessCount_; }
    uint32_t getRecoveryFailCount()    const { return recoveryFailCount_; }
    uint32_t getLastRecoveryDurationMs() const { return lastRecoveryDurationMs_; }
    uint32_t getMaxRecoveryDurationMs()  const { return maxRecoveryDurationMs_; }
    uint32_t getLastBusOffMs()         const { return lastBusOffMs_; }
    uint32_t getBusOffTec()            const { return busOffTec_; }
    uint32_t getBusOffRec()            const { return busOffRec_; }
    uint32_t getTxSuppressedCount()    const { return txSuppressedCount_; }
    uint32_t getCooldownMs()           const { return cooldownMs_; }
    void     setCooldownMs(uint32_t ms)      { cooldownMs_ = ms; }
    int      getLastInstallErr()       const { return (int)lastInstallErr_; }
    int      getLastStartErr()         const { return (int)lastStartErr_; }

    bool     getSoftRecovery()             const { return true; }
    uint32_t getSoftRecoveryFallbackCount() const { return softRecoveryFallbackCount_; }

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
    bool getStatus(twai_status_info_t &status)
    {
        return twai_get_status_info(&status) == ESP_OK;
    }

    uint8_t stateCode()
    {
        twai_status_info_t status;
        if (!getStatus(status))
            return driverOK_ ? 1 : 0;
        switch (status.state) {
        case TWAI_STATE_RUNNING: return 1;
        case TWAI_STATE_BUS_OFF: return 2;
        case TWAI_STATE_RECOVERING: return 3;
        case TWAI_STATE_STOPPED: return recoveryInProgress_ ? 3 : 0;
        default: return 0;
        }
    }

    bool isBusOff()
    {
        twai_status_info_t status;
        if (!getStatus(status))
            return false;
        return status.state == TWAI_STATE_BUS_OFF;
    }

    // BUS-OFF 진입 시 TEC/REC 캡처 후 카운터 갱신
    void captureBusOffSnapshot()
    {
        twai_status_info_t st;
        if (getStatus(st)) {
            busOffTec_ = st.tx_error_counter;
            busOffRec_ = st.rx_error_counter;
        }
        lastBusOffMs_ = millis();
        busoffCount_++;
    }

    void recordRecoverySuccess(uint32_t startMs)
    {
        driverOK_ = true;
        recoveryInProgress_ = false;
        recoverySuccessCount_++;
        uint32_t dur = millis() - startMs;
        lastRecoveryDurationMs_ = dur;
        if (dur > maxRecoveryDurationMs_) maxRecoveryDurationMs_ = dur;
    }

    bool hardReinstall(uint32_t startMs, bool countSoftFallback)
    {
        twai_status_info_t status;
        if (getStatus(status)) {
            if (status.state == TWAI_STATE_RUNNING) {
                twai_stop();
            }
            if (status.state != TWAI_STATE_RECOVERING) {
                twai_driver_uninstall();
            }
        }

        esp_err_t installErr = twai_driver_install(&g_config_, &t_config_, &f_config_);
        lastInstallErr_ = installErr;
        esp_err_t startErr = (installErr == ESP_OK) ? twai_start() : installErr;
        lastStartErr_ = startErr;
        if (installErr != ESP_OK || startErr != ESP_OK)
        {
            if (installErr == ESP_OK) {
                twai_driver_uninstall();
            }
            driverOK_ = false;
            recoveryInProgress_ = false;
            recoveryFailCount_++;
            return false;
        }

        if (countSoftFallback) softRecoveryFallbackCount_++;
        recordRecoverySuccess(startMs);
        return true;
    }

    bool updateRecoveryProgress()
    {
        twai_status_info_t status;
        if (!getStatus(status)) {
            driverOK_ = false;
            recoveryInProgress_ = false;
            return false;
        }

        if (status.state == TWAI_STATE_RUNNING) {
            driverOK_ = true;
            recoveryInProgress_ = false;
            return true;
        }

        if (status.state == TWAI_STATE_RECOVERING) {
            return false;
        }

        if (status.state == TWAI_STATE_STOPPED && recoveryInProgress_) {
            uint32_t startMs = recoveryStartMs_ ? recoveryStartMs_ : millis();
            if (twai_start() == ESP_OK) {
                recordRecoverySuccess(startMs);
                return true;
            }
            hardReinstall(startMs, true);
            return false;
        }

        if (status.state == TWAI_STATE_BUS_OFF) {
            recoverWithCooldown();
            return false;
        }

        return false;
    }

    void recoverWithCooldown()
    {
        uint32_t now = millis();
        if (now - lastRecovery_ < cooldownMs_)
            return;
        lastRecovery_ = now;

        twai_status_info_t status;
        if (!getStatus(status)) {
            driverOK_ = false;
            recoveryInProgress_ = false;
            return;
        }

        if (status.state == TWAI_STATE_RECOVERING ||
            (status.state == TWAI_STATE_STOPPED && recoveryInProgress_)) {
            updateRecoveryProgress();
            return;
        }

        if (status.state != TWAI_STATE_BUS_OFF) {
            if (status.state == TWAI_STATE_RUNNING) driverOK_ = true;
            return;
        }

        captureBusOffSnapshot();
        recoveryAttemptCount_++;
        recoveryStartMs_ = millis();

        if (twai_initiate_recovery() == ESP_OK) {
            recoveryInProgress_ = true;
            driverOK_ = true;
        } else {
            hardReinstall(recoveryStartMs_, true);
        }
    }

    void tryRecover()
    {
        if (txQuiesced_) return;
        uint32_t now = millis();
        if (now - lastRecovery_ < cooldownMs_ * 10)
            return;
        lastRecovery_ = now;

        recoveryAttemptCount_++;
        uint32_t startMs = millis();
        hardReinstall(startMs, false);
    }

    gpio_num_t txPin_;
    gpio_num_t rxPin_;
    twai_general_config_t g_config_;
    twai_timing_config_t t_config_;
    twai_filter_config_t f_config_;
    bool     driverOK_ = false;
    bool     txQuiesced_ = false;
    bool     recoveryInProgress_ = false;
    uint32_t lastRecovery_ = 0;
    uint32_t recoveryStartMs_ = 0;
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
    uint32_t txSuppressedCount_ = 0;
    uint32_t softRecoveryFallbackCount_ = 0;
    esp_err_t lastInstallErr_ = ESP_OK;
    esp_err_t lastStartErr_ = ESP_OK;
};
