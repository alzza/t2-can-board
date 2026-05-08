#pragma once

#ifdef BOARD_T2CAN
    // =========================================================
    // A채널 (MCP2515) - 13/14번 CAN (FSD + ASS + Enhanced Autopilot)
    // =========================================================
    #define T2CAN_RST_PIN  9
    #define T2CAN_CS       10
    #define T2CAN_SCK      12
    #define T2CAN_MISO     13
    #define T2CAN_MOSI     11

    // MCP2515 SPI clock (Hz): 기본 10MHz, 웹 UI에서 8MHz 요청 가능
    #ifndef T2CAN_SPI_FREQ_HZ
    #define T2CAN_SPI_FREQ_HZ 10000000
    #endif

    // MCP2515 crystal MHz (8 or 16)
    #ifndef MCP2515_CRYSTAL_MHZ
    #define MCP2515_CRYSTAL_MHZ 16
    #endif

    // =========================================================
    // B채널 (TWAI) - 2/3번 CAN (Nag Killer 전용, X179)
    // =========================================================
    #define T2CAN_TX       7
    #define T2CAN_RX       6
#endif