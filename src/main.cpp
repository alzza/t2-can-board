#include <Arduino.h>
#include "app.h"
#include "t2can_pins.h"
// =========================================================
// [Target 1] A채널 (MCP2515) 선택 시 활성화되는 핀맵
// =========================================================
#if defined(DRIVER_MCP2515)
    #include <SPI.h>
    #include "drivers/mcp2515_driver.h"
    #define MCP2515_CS    10
    #define MCP2515_SCLK  12
    #define MCP2515_MOSI  11
    #define MCP2515_MISO  13
    #define MCP2515_RST   9
// =========================================================
// [Target 2] B채널 (TWAI) 선택 시 활성화되는 핀맵
// =========================================================
#elif defined(DRIVER_TWAI)
    #include "drivers/twai_driver.h"
    #define PIN_TWAI_TX GPIO_NUM_7
    #define PIN_TWAI_RX GPIO_NUM_6
#endif

void setup() {
    Serial.begin(115200);
    delay(4000);

    Serial.println("\n\n========================================");
    Serial.println("  Tesla Open CAN Mod - LilyGo T-2Can");

#if defined(DRIVER_MCP2515)
    // ----------------- A채널 초기화 로직 -----------------
    Serial.println("  [모드] Channel A (MCP2515) 선택됨");
    Serial.println("========================================\n");

    pinMode(MCP2515_RST, OUTPUT);
    digitalWrite(MCP2515_RST, LOW); delay(100);
    digitalWrite(MCP2515_RST, HIGH); delay(100);
    
    SPI.begin(MCP2515_SCLK, MCP2515_MISO, MCP2515_MOSI, -1);
    
    auto driver = std::make_unique<MCP2515Driver>(MCP2515_CS);
    if (!driver->init()) {
        Serial.println("[FAIL] A채널 MCP2515 칩 응답 없음!");
        while(1) delay(1000);
    }
    Serial.println("[SUCCESS] HW3 A-Channel Ready!");
    appSetup<MCP2515Driver>(std::move(driver), "--- A-Channel FSD Mod Running ---");

#elif defined(DRIVER_TWAI)
    // ----------------- B채널 초기화 로직 -----------------
    Serial.println("  [모드] Channel B (TWAI) 선택됨");
    Serial.println("========================================\n");

    auto driver = std::make_unique<TWAIDriver>(PIN_TWAI_TX, PIN_TWAI_RX);
    
    // [핵심] 우리가 여기서 직접 init() 하지 않고, appSetup이 알아서 켜도록 바로 던져줍니다!
    appSetup<TWAIDriver>(std::move(driver), "--- B-Channel FSD Mod Running ---");

#else

    Serial.println("========================================\n");
    Serial.println("[ERROR] 선택된 채널이 없습니다!");
    Serial.println("sketch_config.h 에서 DRIVER_MCP2515 또는 DRIVER_TWAI 주석을 해제하세요.");
#endif
}

void loop() {
    static unsigned long lastTick = 0;
    if (millis() - lastTick > 5000) {
        Serial.println("[System] 차량(HW3) 데이터 수신 대기 중...");
        lastTick = millis();
    }

#if defined(DRIVER_MCP2515)
    appLoop<MCP2515Driver>();
#elif defined(DRIVER_TWAI)
    appLoop<TWAIDriver>();
#endif
}