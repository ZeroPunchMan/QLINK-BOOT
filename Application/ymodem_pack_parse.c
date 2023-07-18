#include "ymodem_pack_parse.h"
#include "ymodem_helper.h"
#include "systime.h"
#include "cl_log.h"

typedef enum
{
    PPS_Head,       // SOH or STX
    PPS_PackNum,    // 包计数
    PPS_InvPackNum, // 包计数2 取反
    PPS_Data,       // 数据段
    PPS_CrcH,       // Crc高字节
    PPS_CrcL,       // Crc低字节
} PackParseStatus_t;

PackParseStatus_t packParseStatus = PPS_Head; // 包解析状态
YmodemPacket_t recvPacket = {
    .head = 0,
    .dataLen = 0,
};

static uint16_t GetDataBlockSize(uint8_t head) // 获取数据段大小
{
    if (head == YMK_SOH)
        return 128;
    else if (head == YMK_STX)
        return 1024;

    return 0; // error
}

const YmodemPacket_t *YmodemParseByte(uint8_t byte)
{
    static uint32_t lastTime = 0;
    if (SysTimeSpan(lastTime) >= SOG_PACKET_TIMEOUT) // 超时,重新解析head
        packParseStatus = PPS_Head;

    lastTime = GetSysTime();

    switch (packParseStatus)
    {
    case PPS_Head:
        if (byte == YMK_SOH || byte == YMK_STX)
        {                           // 收到SOH或STX,继续解析包计数
            recvPacket.head = byte; // SOH or STX
            recvPacket.dataLen = 0; // 数据长度清零

            packParseStatus = PPS_PackNum;
            // CL_LOG_LINE("recv head: %x", byte);
        }
        else if (byte == YMK_EOT)
        {
            // 收到EOT,这一包结束
            recvPacket.head = byte; // EOT
            recvPacket.dataLen = 0; // 数据长度清零

            packParseStatus = PPS_Head;
            return &recvPacket;
        }
        break;

    case PPS_PackNum:
        recvPacket.packNum = byte; // 包计数
        packParseStatus = PPS_InvPackNum;
        // CL_LOG_LINE("recv pack num: %x", byte);
        break;

    case PPS_InvPackNum:
        if (byte == (uint8_t)~recvPacket.packNum)
        {
            // 包计数反码正确,准备解析数据段
            recvPacket.invPackNum = byte;
            packParseStatus = PPS_Data;
            // CL_LOG_LINE("recv pack inv: %x", byte);
        }
        else
        {
            // 包计数反码错误,重新回到解析head状态
            packParseStatus = PPS_Head;
            // CL_LOG_LINE("pack inv illegal");
        }
        break;

    case PPS_Data:
        recvPacket.data[recvPacket.dataLen++] = byte;
        if (recvPacket.dataLen >= GetDataBlockSize(recvPacket.head))
        {
            packParseStatus = PPS_CrcH;
            // CL_LOG_LINE("recv data ok");
        }
        break;

    case PPS_CrcH:
        recvPacket.crc = (uint16_t)byte << 8;
        packParseStatus = PPS_CrcL;
        // CL_LOG_LINE("recv crc h");

        break;

    case PPS_CrcL:
        recvPacket.crc |= byte;
        // CL_LOG_LINE("recv crc l");

        uint16_t crc = CalcYmodemCrc(recvPacket.data, 0, GetDataBlockSize(recvPacket.head));
        if (recvPacket.crc == crc)
        {                               // 接收到的crc正确
            packParseStatus = PPS_Head; // 状态重设为解析head
            // CL_LOG_LINE("crc ok");
            return &recvPacket; // 返回packet
        }
        else
        { // crc错误,重新回到解析head状态
            // CL_LOG_LINE("crc error");
            packParseStatus = PPS_Head;
        }
        break;
    }

    return CL_NULL;
}
