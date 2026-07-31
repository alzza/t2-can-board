#pragma once

#define FIRMWARE_VERSION "1.3.7"
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 3
#define FIRMWARE_VERSION_PATCH 7

#ifndef FIRMWARE_BUILD_ID
#define FIRMWARE_BUILD_ID "FW137-dev"
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
