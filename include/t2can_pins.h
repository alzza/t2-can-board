#pragma once

#ifdef BOARD_T2CAN
    // A채널 (MCP2515) - 13/14번 핀 연결용
    #define T2CAN_RST_PIN 9
    #define T2CAN_CS      10
    #define T2CAN_SCK     12
    #define T2CAN_MISO    13
    #define T2CAN_MOSI    11

    // B채널 (TWAI) - 2/3번 핀 연결용 (X179)
    #define T2CAN_TX      7
    #define T2CAN_RX      6
#endif