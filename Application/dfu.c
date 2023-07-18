#include "dfu.h"
#include "flash_layout.h"
#include "string.h"
#include "main.h"
#include "stm32f0xx_hal_flash.h"
#include "crc.h"

typedef struct
{
    uint32_t size;
    uint32_t hash;
} AppVerifyInfo_t;

CL_Result_t EraseFlash(uint32_t addr, uint32_t pages);

static AppVerifyInfo_t appVerifyInfo;

inline static uint32_t CalcAppHash(void)
{
    uint32_t crc = Ethernet_CRC32((const uint8_t *)APP_START_ADDR, appVerifyInfo.size);
    return crc;
}

inline static uint32_t CalcOtaBakHash(void)
{
    uint32_t crc = Ethernet_CRC32((const uint8_t *)DFU_BAK_START_ADDR, appVerifyInfo.size);
    return crc;
}

bool IsAppValid(void)
{
    uint32_t appHash;

    uint32_t *pAppHead = (uint32_t *)APP_START_ADDR;
    if ((pAppHead[0] & 0xff000000) != 0x20000000 || (pAppHead[1] & 0xff000000) != 0x08000000)
        return false;

    if (appVerifyInfo.hash == 0xffffffff &&
        appVerifyInfo.size == 0xffffffff)
        return true;

    appHash = CalcAppHash();
    if (appHash != appVerifyInfo.hash)
        return false;

    return true;
}

bool IsOtaBakValid(void)
{
    uint32_t appHash;

    uint32_t *pAppHead = (uint32_t *)DFU_BAK_START_ADDR;
    if ((pAppHead[0] & 0xff000000) != 0x20000000 || (pAppHead[1] & 0xff000000) != 0x08000000)
        return false;

    appHash = CalcOtaBakHash();
    if (appHash != appVerifyInfo.hash)
        return false;

    return true;
}

//-------------------------flash操作------------------------------------------
CL_Result_t EraseAppFlash(void)
{
    return EraseFlash(APP_START_ADDR, APP_SIZE / FLASH_PAGE_SIZE);
}

CL_Result_t EraseBakFlash(void)
{
    return EraseFlash(DFU_BAK_START_ADDR, DFU_BAK_SIZE / FLASH_PAGE_SIZE);
}

CL_Result_t SaveAppInfo(uint32_t size, uint32_t hash)
{
    appVerifyInfo.size = size;
    appVerifyInfo.hash = hash;

    // 写入主存页
    if (EraseFlash(APP_INFO_ADDR, 1) != CL_ResSuccess)
        return CL_ResFailed;
    if (WriteFlash(APP_INFO_ADDR, (const uint8_t *)(&appVerifyInfo), sizeof(appVerifyInfo)) != CL_ResSuccess)
        return CL_ResFailed;

    return CL_ResSuccess;
}

CL_Result_t LoadAppInfo(void)
{
    memcpy(&appVerifyInfo, (const void *)APP_INFO_ADDR, sizeof(appVerifyInfo));
    return CL_ResSuccess;
}

CL_Result_t EraseFlash(uint32_t addr, uint32_t pages)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = addr;
    EraseInitStruct.NbPages = pages;

    uint32_t PAGEError = 0;
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        return CL_ResFailed;

    return CL_ResSuccess;
}

CL_Result_t WriteFlash(uint32_t addr, const uint8_t *buff, uint32_t length)
{
    uint32_t writeAddr, offset;
    uint32_t data;
    HAL_StatusTypeDef status;

    /* Clear All pending flags */
    // __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR | FLASH_FLAG_PROGERR);

    // little endian [0-0][0-1][0-2][0-3][1-0][1-1][1-2][1-3] -> 3210
    offset = 0;
    writeAddr = addr;
    while (offset < length)
    {
        data = 0;
        for (int i = 0; i < 4; i++)
        {
            if (offset < length)
                data |= (uint32_t)buff[offset++] << (i * 8);
            else
                break;
        }
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, writeAddr, data);
        if (status != HAL_OK)
            return CL_ResFailed;

        writeAddr += 4;
    }
    return CL_ResSuccess;
}

CL_Result_t CopyOtaBakToApp(void)
{
    if (EraseAppFlash() != CL_ResSuccess)
        return CL_ResFailed;

    return WriteFlash(APP_START_ADDR, (const uint8_t *)DFU_BAK_START_ADDR, appVerifyInfo.size);
}

CL_Result_t MarkNeedDfu(void)
{
    if (EraseFlash(DFU_FLAG_ADDR, 1) != CL_ResSuccess)
        return CL_ResFailed;

    uint32_t mark[2] = {0x12345678, 0x87654321};
    return WriteFlash(DFU_FLAG_ADDR, (const uint8_t *)mark, sizeof(mark));
}
//------------------------jump----------------------------------------
typedef void (*pFunction)(void);
pFunction JumpToAddrFunc;
void JumpToBootloader(void)
{
    uint32_t JumpAddress; // 跳转地址

    JumpAddress = *(volatile uint32_t *)(FLASH_START_ADDR + 4); // 获取复位地址
    JumpToAddrFunc = (pFunction)JumpAddress;                    // 函数指针指向复位地址
    __set_MSP(*(volatile uint32_t *)FLASH_START_ADDR);          // 设置主堆栈指针MSP指向升级机制IAP_ADDR
    JumpToAddrFunc();                                           // 跳转到升级代码处
}

void JumpToApplication(void)
{
    uint32_t JumpAddress; // 跳转地址

    JumpAddress = *(volatile uint32_t *)(APP_START_ADDR + 4); // 获取复位地址
    JumpToAddrFunc = (pFunction)JumpAddress;                  // 函数指针指向复位地址
    __set_MSP(*(volatile uint32_t *)APP_START_ADDR);          // 设置主堆栈指针MSP指向升级机制IAP_ADDR
    JumpToAddrFunc();                                         // 跳转到升级代码处
}
