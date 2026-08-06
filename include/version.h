#pragma once

#define FIRMWARE_VERSION "1.3.8"
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 3
#define FIRMWARE_VERSION_PATCH 8

// OTA/실차 테스트용 보관 bin 파일명에 사용할 당일 핵심 변경 요약.
// 예: 1.3.8_26-08-07_summon-txdiag.bin
#define FIRMWARE_ARTIFACT_NOTE "summon-txdiag"

#ifndef FIRMWARE_BUILD_ID
#define FIRMWARE_BUILD_ID "FW138-summon-txdiag"
#endif

#ifndef FIRMWARE_BUILD_AT
#define FIRMWARE_BUILD_AT __DATE__ " " __TIME__
#endif

#ifndef FIRMWARE_BUILD_ENV
#define FIRMWARE_BUILD_ENV "unknown"
#endif

#ifndef FIRMWARE_GIT_SHA
#define FIRMWARE_GIT_SHA "unknown"
#endif

#ifndef FIRMWARE_GIT_BRANCH
#define FIRMWARE_GIT_BRANCH "unknown"
#endif

#ifndef FIRMWARE_SOURCE_HASH
#define FIRMWARE_SOURCE_HASH "unknown"
#endif

#ifndef FIRMWARE_GIT_DIRTY
#define FIRMWARE_GIT_DIRTY 1
#endif
