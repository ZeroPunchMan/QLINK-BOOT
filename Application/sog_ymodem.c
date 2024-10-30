#include "sog_ymodem.h"
#include "systime.h"
#include "string.h"
#include "ymodem_helper.h"
#include "usart.h"
#include "ymodem_pack_parse.h"
#include "stdlib.h"
#include "flash_layout.h"
#include "cl_event_system.h"
#include "main.h"
#include "cl_log.h"
#include "dfu.h"
#include "board.h"
#include "firmware_info.h"
#include "cl_serialize.h"
#include "crc.h"
#include "dwin_protocol.h"

typedef enum
{
    OtaStatus_Idle, //
    OtaStatus_WaitMcuxx,
    OtaStatus_WaitFileName,
    OtaStatus_RecvFileData,
    OtaStatus_WaitEot,
    OtaStatus_Jump,
    OtaStatus_Error,
} OtaStatus_t;

typedef enum
{
    TargetMcu_Unkown,
    TargetMcu_1A,
    TargetMcu_2B,
    TargetMcu_3C,
} TargetMcu_t;

typedef struct
{
    OtaStatus_t status;
    TargetMcu_t targetMcu;
    uint8_t expectedPackNum;
    uint32_t timeoutTime;
    int32_t binFileSize;
    int32_t receivedSize;
    int32_t savedSize;
} OtaContext_t;

static OtaContext_t otaContext = {
    .status = OtaStatus_Idle,
    .targetMcu = TargetMcu_Unkown,
    .expectedPackNum = 0,
    .timeoutTime = 0,
    .binFileSize = 0,
    .receivedSize = 0,
    .savedSize = 0,
};

static const uint8_t encryptTable[] = {
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
    157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
    35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
    190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
    70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
    219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
    101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
    248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
    140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
    17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
    175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
    50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
    202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
    87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
    233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
    116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

static inline void SendAck(void)
{
    uint8_t cmd = YMK_ACK; // 发ACK
    Usartx_Send(USART2, &cmd, 0, 1);
    otaContext.timeoutTime = GetSysTime();
}

static inline void SendAckC(void)
{
    uint8_t cmd[2] = {YMK_ACK, YMK_C}; // 发CAN
    Usartx_Send(USART2, cmd, 0, sizeof(cmd));
    otaContext.timeoutTime = GetSysTime();
}

static inline void SendC(void)
{
    uint8_t cmd = YMK_C; // 发C
    Usartx_Send(USART2, &cmd, 0, 1);
    otaContext.timeoutTime = GetSysTime();
}

static inline void SendDoubleCAN(void)
{
    uint8_t cmd[2] = {YMK_CAN, YMK_CAN}; // 发CAN
    Usartx_Send(USART2, cmd, 0, sizeof(cmd));
}

// static inline void SendNak(void)
//{
//     uint8_t cmd = YMK_NAK; // 发C
//     Usartx_Send(USART2, &cmd, 0, 1);
//     otaContext.timeoutTime = GetSysTime();
// }

static inline TargetMcu_t PraseMcuxx(uint8_t byte)
{
    static uint8_t count = 0;
    static uint8_t mcuIdx[2]; // 1A or 2B or 3C, etc.
    if (count == 0)
    {
        (byte == 'M') ? (count++) : (count = 0);
    }
    else if (count == 1)
    {
        (byte == 'C') ? (count++) : (count = 0);
    }
    else if (count == 2)
    {
        (byte == 'U') ? (count++) : (count = 0);
    }
    else if (count == 3)
    {
        mcuIdx[0] = byte;
        count++;
    }
    else if (count == 4)
    {
        mcuIdx[1] = byte;
        // 判断
        if (mcuIdx[0] == '1' && mcuIdx[1] == 'A')
        {
            count = 0;
            return TargetMcu_1A;
        }
        else
        {
            count = 0; // error
        }
    }
    return TargetMcu_Unkown;
}

bool FwUpgradeCheck(uint8_t byte)
{
    const char cmd[] = "FWUPGRADE";
    static uint8_t count = 0;

    // static uint32_t lastTime = 0;
    // if(SysTimeSpan(lastTime) > SOG_PACKET_TIMEOUT)
    // {    //超时检测
    //     count = 0;
    // }
    // lastTime = GetSysTime();

    if (byte == cmd[count])
    {
        if (++count >= sizeof(cmd) - 1)
        {
            count = 0;
            return true;
        }
    }
    else
    {
        count = 0;
    }
    return false;
}

static inline bool ParseFileNamePack(const YmodemPacket_t *packet)
{ // 解析第一包,文件名和文件大小
    if (packet->packNum != 0)
        return false;

    char fileName[16], strSize[16];
    uint16_t offset = 0;
    for (int i = 0; i < sizeof(fileName) - 1; i++)
    { // 文件名最多15字节
        if (packet->data[offset] != YMK_NULL)
        {
            fileName[i] = packet->data[offset++];
        }
        else
        {
            fileName[i] = '\0';
            break;
        }
    }
    // 此时offset应该指向NULL
    offset++;
    for (int i = 0; i < sizeof(strSize) - 1; i++)
    { // size字符串最多15字节
        if (packet->data[offset] != YMK_NULL && packet->data[offset] >= '0' && packet->data[offset] <= '9')
        {
            strSize[i] = packet->data[offset++];
        }
        else
        {
            strSize[i] = '\0';
            break;
        }
    }

    if (packet->data[offset++] != ' ') // 文件大小后面要一个空格
    {

        // return false; //不判断空格
    }

    // 检查后续NULL
    for (; offset < packet->dataLen; offset++)
    {
        if (packet->data[offset] != 0)
            return false;
    }

    if (strcmp(fileName, "XXAPP.bin") != 0) // 判断文件名
        return false;

    int size = atoi(strSize);
    if (size <= 0 || size % 2 != 0 || size >= APP_SIZE) // 判断文件大小
        return false;

    otaContext.binFileSize = size; // 记录下bin文件大小

    CL_LOG_LINE("recv file name: %s, total size: %d", fileName, size);

    return true;
}

static inline void ToError(void)
{ // 发2个CAN取消
    SendDoubleCAN();

    otaContext.status = OtaStatus_Error;
}

static void OnRecvFileData(uint8_t *data, uint16_t size)
{
    // bin文件加密,还原数据
    uint32_t count = otaContext.savedSize;
    for (uint16_t i = 0; i < size; i++)
    {
        data[i] ^= encryptTable[count % 256];
        count++;
    }

    // 保存到备份区
    uint32_t addr = DFU_BAK_START_ADDR + otaContext.savedSize;
    WriteFlash(addr, data, size);

    // 记录已保存大小
    otaContext.savedSize += size;
}

static bool OnRecvFileDone(void)
{
    // 校验bak区内容有效性
    // 检查固件信息
    const FirmwareInfo_t *pFirmwareInfo = (const FirmwareInfo_t *)(DFU_BAK_START_ADDR + FIWMWARE_INFO_OFFSET);
    CL_LOG_LINE("check firmware info");
    if (pFirmwareInfo->productId[0] != ProductId_0 || pFirmwareInfo->productId[1] != ProductId_1)
        return false;

    if (!FirmwareCheck(pFirmwareInfo))
        return false;

    // 最后4字节是crc
    CL_LOG_LINE("check crc");
    uint32_t crcCalc = Ethernet_CRC32((const uint8_t *)DFU_BAK_START_ADDR, otaContext.binFileSize - 4);
    uint32_t crcRecv = CL_BytesToUint32((const uint8_t *)DFU_BAK_START_ADDR + otaContext.binFileSize - 4, CL_BigEndian);
    if (crcCalc != crcRecv)
        return false;

    CL_LOG_LINE("save app, size: %d, crc: %x--%x", otaContext.binFileSize - 4, crcCalc, crcRecv);
    if (SaveAppInfo(otaContext.binFileSize - 4, crcCalc) != CL_ResSuccess)
        return false; // 保存APP校验信息
    CL_LOG_LINE("copy to app");
    if (CopyOtaBakToApp() != CL_ResSuccess)
        return false; // 可用,bak覆盖到app

    return true;
}

static inline bool ParseFileDataPack(YmodemPacket_t *packet, bool *fileDone)
{ // 解析文件内容,需判断接收的文件大小,最后一包的CPMEOF
    fileDone[0] = false;

    int32_t remFileSize = otaContext.binFileSize - otaContext.receivedSize;
    if (remFileSize <= packet->dataLen)
    { // 接收的文件大小达到,应该完成了,检查CPMEOF
        for (int i = remFileSize; i < packet->dataLen; i++)
        {
            if (packet->data[i] != YMK_CPMEOF)
            {
                return false;
            }
        }

        otaContext.receivedSize += remFileSize;
        CL_LOG_LINE("recv last pack: %d", packet->packNum);
        OnRecvFileData(packet->data, remFileSize);
        fileDone[0] = true; // 接收完成了
    }
    else
    {
        otaContext.receivedSize += packet->dataLen;
        CL_LOG_LINE("recv pack: %d", packet->packNum);
        OnRecvFileData(packet->data, packet->dataLen);
    }

    return true;
}

static void RecvParser(void)
{
    volatile uint8_t byte;
    for (int i = 0; i < 100; i++)
    {
        if (Usart2_PollRecvByte(&byte) == CL_ResSuccess)
        {
            TargetMcu_t targetMcu = PraseMcuxx(byte);
            if (targetMcu != TargetMcu_Unkown)
            {
                CL_EventSysRaise(CL_Event_RecvMcuxx, 0, (void *)&targetMcu);
            }
            if (FwUpgradeCheck(byte))
            {
                CL_EventSysRaise(CL_Event_RecvFwupgrade, 0, CL_NULL);
            }

            const YmodemPacket_t *packet = YmodemParseByte(byte);
            if (packet != CL_NULL)
            {
                CL_EventSysRaise(CL_Event_RecvYmodemPack, 0, (void *)packet);
            }
        }
        else
        {
            break;
        }
    }
}

static void TimeoutCheck(void)
{
    if (SysTimeSpan(otaContext.timeoutTime) > SOG_RESEND_TIME)
    {
        if (otaContext.status >= OtaStatus_WaitMcuxx && otaContext.status <= OtaStatus_WaitEot)
        {
            ToError(); // 超时了,直接取消,不做重发
            CL_LOG_LINE("dfu time out");
        }
    }
}

bool OnRecvMcuxx(void *eventArg)
{
    TargetMcu_t *tm = (TargetMcu_t *)eventArg;
    if (otaContext.status == OtaStatus_WaitMcuxx)
    {
        if (tm[0] == TargetMcu_1A)
        {
            HAL_FLASH_Unlock(); // 需要升级,解锁flash
            EraseBakFlash();    // 擦除备份区

            otaContext.expectedPackNum = 0;
            SendC();
            otaContext.status = OtaStatus_WaitFileName; // 接收文件名
            CL_LOG_LINE("recv MCU2B");
        }
    }
    else if (otaContext.status == OtaStatus_WaitFileName)
    { // sender重发,响应即可
        SendC();
    }

    return true;
}

bool OnRecvFwupgrade(void *eventArg)
{
    if (otaContext.status == OtaStatus_WaitMcuxx)
    {
        SendAck();
        CL_LOG_LINE("recv fwupgrade");
    }

    return true;
}

bool OnRecvYmodemPack(void *eventArg)
{
    YmodemPacket_t *packet = (YmodemPacket_t *)eventArg;

    if (packet == CL_NULL)
        return false;

    switch (otaContext.status)
    {
    case OtaStatus_WaitFileName:
        if (ParseFileNamePack(packet))
        {                                               // 第一包正确
            otaContext.receivedSize = 0;                // 已接收文件大小清零
            otaContext.savedSize = 0;                   // 已保存的文件大小清零
            otaContext.expectedPackNum = 1;             // 包计数设置为1
            otaContext.status = OtaStatus_RecvFileData; // 进入接收文件内容状态

            SendAckC();
        }
        else
        { // 第一包错误
            CL_LOG_LINE("file name error");
            ToError();
        }
        break;
    case OtaStatus_RecvFileData:
        if (packet->packNum == 0 && otaContext.receivedSize == 0)
        { // 重发文件名,响应即可,不重新处理
            SendAckC();
            CL_LOG_LINE("resend file name");
        }
        else if (packet->packNum + 1 == otaContext.expectedPackNum)
        { // 重发文件数据包,响应即可,不重新处理
            SendAck();
            CL_LOG_LINE("resend pack: %d", packet->packNum);
        }
        else if (packet->packNum == otaContext.expectedPackNum)
        {
            bool fileRecvDone;
            if (ParseFileDataPack(packet, &fileRecvDone))
            {
                CL_LOG_LINE("recv pack: %d, total size: %d", otaContext.expectedPackNum, otaContext.receivedSize);
                otaContext.expectedPackNum++; // 包计数+1

                if (fileRecvDone)
                {
                    CL_LOG_LINE("recv done");
                    otaContext.status = OtaStatus_WaitEot; // 接收完成了,等EOT
                }

                SendAck();
            }
            else
            {
                CL_LOG_LINE("cpmeof error");
                ToError();
            }
        }
        else
        {
            CL_LOG_LINE("pack num error");
            ToError();
        }
        break;
    case OtaStatus_WaitEot:
        if (packet->head == YMK_EOT)
        { // 收到EOT,先完成升级,然后回复
            if (OnRecvFileDone())
            { // 接收完成处理
                SendAck();
                // otaContext.status = OtaStatus_Jump;
                CL_LOG_LINE("dfu done, reboot");
                DelayOnSysTime(200);
                NVIC_SystemReset();
            }
            else
            {
                CL_LOG_LINE("file verify error");
                ToError();
            }
        }
        else if (packet->head == YMK_SOH || packet->head == YMK_STX)
        { // 不是EOT,可能是重发
            if (packet->packNum + 1 == otaContext.expectedPackNum)
            {
                SendAck();
                CL_LOG_LINE("resend last pack");
            }
        }
        break;
    }

    return true;
}

static bool NeedOta(void)
{
    bool res = false;

    const uint32_t *pDfuFlag = (const uint32_t *)DFU_FLAG_ADDR;
    if (pDfuFlag[0] == 0x12345678 && pDfuFlag[1] == 0x87654321)
    {
        HAL_FLASH_Unlock();
        EraseFlash(DFU_FLAG_ADDR, 1);
        HAL_FLASH_Lock();
        return true;
    }

    if (!LL_GPIO_IsInputPinSet(KEY1_PORT, KEY1_PIN) &&
        !LL_GPIO_IsInputPinSet(KEY2_PORT, KEY2_PIN) &&
        !LL_GPIO_IsInputPinSet(KEY3_PORT, KEY3_PIN))
    {
        return true;
    }

    return res;
}

static void EnableBle(void)
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_GPIOB | LL_AHB1_GRP1_PERIPH_GPIOC | LL_AHB1_GRP1_PERIPH_GPIOD);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;

    LL_GPIO_SetOutputPin(BLE_RST_PORT, BLE_RST_PIN);
    GPIO_InitStruct.Pin = BLE_RST_PIN;
    LL_GPIO_Init(BLE_RST_PORT, &GPIO_InitStruct);

    LL_GPIO_SetOutputPin(BLE_PWR_PORT, BLE_PWR_PIN);
    GPIO_InitStruct.Pin = BLE_PWR_PIN;
    LL_GPIO_Init(BLE_PWR_PORT, &GPIO_InitStruct);

    LL_GPIO_ResetOutputPin(LED1_EN_PORT, LED1_EN_PIN);
    GPIO_InitStruct.Pin = LED1_EN_PIN;
    LL_GPIO_Init(LED1_EN_PORT, &GPIO_InitStruct);

    LL_GPIO_ResetOutputPin(LED2_EN_PORT, LED2_EN_PIN);
    GPIO_InitStruct.Pin = LED2_EN_PIN;
    LL_GPIO_Init(LED2_EN_PORT, &GPIO_InitStruct);

    LL_GPIO_ResetOutputPin(LED3_EN_PORT, LED3_EN_PIN);
    GPIO_InitStruct.Pin = LED3_EN_PIN;
    LL_GPIO_Init(LED3_EN_PORT, &GPIO_InitStruct);

    LL_GPIO_ResetOutputPin(LED4_EN_PORT, LED4_EN_PIN);
    GPIO_InitStruct.Pin = LED4_EN_PIN;
    LL_GPIO_Init(LED4_EN_PORT, &GPIO_InitStruct);
}

void SogYmodem_Process(void)
{
    if (otaContext.status == OtaStatus_Idle)
    {
        if (NeedOta())
        {
            EnableBle();
            DelayOnSysTime(300);
            otaContext.status = OtaStatus_WaitMcuxx;
            SendAck();

            CL_LOG_LINE("ready for dfu, erase bak");
        }
        else
        {
            otaContext.status = OtaStatus_Jump;
            CL_LOG_LINE("don't need ota");
        }
    }
    else if (otaContext.status >= OtaStatus_WaitMcuxx && otaContext.status <= OtaStatus_WaitEot)
    {
        RecvParser();   // 接收解析
        TimeoutCheck(); // 重发检测
    }
    else // jump or error
    {
        LoadAppInfo();
        bool appValid = IsAppValid();
        HAL_FLASH_Lock();
        if (appValid)
        { // app可用,直接跳转
            jumpToApp = true;
            CL_LOG_LINE("jump ready");
        }
        else
        { // app不可用,检查bak
            bool bakVaild = IsOtaBakValid();
            if (bakVaild)
            { // bak可用,复制到app
                HAL_FLASH_Unlock();
                CopyOtaBakToApp();
                HAL_FLASH_Lock();
                CL_LOG_LINE("bak valid, copy to app");
            }
            else
            { // bak也不可用,走升级流程
                EnableBle();
                otaContext.status = OtaStatus_WaitMcuxx;
                otaContext.targetMcu = TargetMcu_Unkown;
                otaContext.timeoutTime = GetSysTime();

                CL_LOG_LINE("no valid app, restart dfu");
            }
        }
        return;
    }
}

void SogYmodem_Init(void)
{
    CL_EventSysAddListener(OnRecvMcuxx, CL_Event_RecvMcuxx, 0);
    CL_EventSysAddListener(OnRecvFwupgrade, CL_Event_RecvFwupgrade, 0);
    CL_EventSysAddListener(OnRecvYmodemPack, CL_Event_RecvYmodemPack, 0);
}
