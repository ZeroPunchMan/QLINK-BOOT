#include "dwin_protocol.h"
#include "systime.h"
#include "usart.h"
#include "string.h"
#include "cl_serialize.h"
#include "cl_log.h"
#include "cl_event_system.h"
#include "crc.h"

const uint8_t packetHead[] = {0x5a, 0xa5};

typedef enum
{
    PS_Head,
    PS_DataLen,
    PS_Cmd,
    PS_Data,
    PS_Crc,
} ParseStatus_t;

typedef struct
{
    ParseStatus_t parseStatus;
    int parseCount;
    DwinPacket_t recvPacket;
    uint32_t lastRecvTime;
} DwinChannelParam_t;

DwinChannelParam_t channelParams[ChanIdx_Max];

void DwinProtocol_Init(void)
{
    for (int i = 0; i < ChanIdx_Max; i++)
    {
        channelParams[i].parseStatus = PS_Head;
        channelParams[i].parseCount = 0;
        channelParams[i].lastRecvTime = 0;
    }
}

static inline bool ParseByte(DwinChannelParam_t *channel, uint8_t b)
{
    // 超时判断,超过时间,认为是新的包
    if (SysTimeSpan(channel->lastRecvTime) >= DWIN_PACKET_TIMEOUT)
    {
        channel->parseCount = 0;
        channel->parseStatus = PS_Head;
    }
    channel->lastRecvTime = GetSysTime();

    switch (channel->parseStatus)
    {
    case PS_Head:
        if (b == packetHead[channel->parseCount])
            channel->parseCount++;
        else
            channel->parseCount = 0;

        if (channel->parseCount >= sizeof(packetHead))
            channel->parseStatus = PS_DataLen;
        break;
    case PS_DataLen:
        channel->recvPacket.length = b;
        if (channel->recvPacket.length < 3 || channel->recvPacket.length > sizeof(channel->recvPacket.data))
        { // length error
            channel->parseCount = 0;
            channel->parseStatus = PS_Head;
        }
        else
        {
            channel->parseCount = 0;
            channel->parseStatus = PS_Cmd;
        }
        break;
    case PS_Cmd:
        channel->recvPacket.cmd = b;
        channel->parseCount = 0;
        if (channel->recvPacket.length > 3)
            channel->parseStatus = PS_Data;
        else
            channel->parseStatus = PS_Crc;
        // Log("cmd");
        break;
    case PS_Data:
        channel->recvPacket.data[channel->parseCount++] = b;
        // Log(string.Format("data: {0}, 0x{1:x}", parseCount, b));
        if (channel->parseCount >= channel->recvPacket.length - 3)
        {
            channel->parseCount = 0;
            channel->parseStatus = PS_Crc;
            // Log("data ok");
        }
        break;
    case PS_Crc:
        channel->recvPacket.crc[channel->parseCount++] = b;
        if (channel->parseCount >= sizeof(channel->recvPacket.crc))
        { // CRC接收完成,计算, 初值0xffff
            uint16_t crcCalc = Modbus_CRC16((uint8_t *)&channel->recvPacket.cmd, channel->recvPacket.length - 2);
            uint16_t crcRecv = CL_BytesToUint16(channel->recvPacket.crc, CL_LittleEndian);

            // CL_LOG_LINE("crc: 0x%x, 0x%x", crcCalc, crcRecv);

            channel->parseCount = 0;
            channel->parseStatus = PS_Head;

            if (crcRecv == crcCalc)
                return true;
        }
        break;
    }
    return false;
}

void DwinProtocol_Process(void)
{
    uint8_t data;
    for (int i = 0; i < 100; i++)
    {
        if (Usart2_PollRecvByte(&data) == CL_ResSuccess)
        {
            if (ParseByte(&channelParams[ChanIdx_ToMainBoard], data))
            {
                CL_EventSysRaise(CL_Event_RecvDwinPack, ChanIdx_ToMainBoard, &channelParams[ChanIdx_ToMainBoard].recvPacket);
            }
        }
        else
        {
            break;
        }
    }
}

void DwinProtocol_SendPack(DwinProtoChanIdx_t chan, const DwinPacket_t *packet)
{
    USART_TypeDef *uart;
    switch (chan)
    {
    case ChanIdx_ToMainBoard:
        uart = USART2;
        break;
    default:
        return;
    }

    Usartx_Send(uart, packetHead, 0, sizeof(packetHead));

    Usartx_Send(uart, (uint8_t *)packet, 0, packet->length - 1);

    uint16_t crc = Modbus_CRC16((uint8_t *)&packet->cmd, packet->length - 2);
    uint8_t data[2];
    CL_Uint16ToBytes(crc, data, CL_LittleEndian);
    Usartx_Send(uart, data, 0, sizeof(data));
}
