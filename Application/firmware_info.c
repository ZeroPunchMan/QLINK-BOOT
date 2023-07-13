#include "firmware_info.h"
#include "flash_layout.h"

#define MAJOR_VER_NUMBER (0)
#define MINOR_VER_NUMBER (0)
#define PATCH_VER_NUMBER (1)

const FirmwareInfo_t firmwareInfo __attribute__((at(APP_START_ADDR + FIWMWARE_INFO_OFFSET))) = {
    .verMajor = MAJOR_VER_NUMBER,
    .verMinor = MINOR_VER_NUMBER,
    .verPatch = PATCH_VER_NUMBER,

    .productId[0] = ProductId_0,
    .productId[1] = ProductId_1,

    .check = FIRMWARE_CHECK_VALUE(MAJOR_VER_NUMBER, MINOR_VER_NUMBER, PATCH_VER_NUMBER, ProductId_0, ProductId_1),
};

// const int testData __attribute__((at(APP_START_ADDR + 38*1024))) = 0x12345;
