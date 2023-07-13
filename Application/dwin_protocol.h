#pragma once

#include "stdint.h"
#include "stdbool.h"


typedef enum
{
    ChanIdx_TouchPad = 0,
    ChanIdx_HandHeld,
    ChanIdx_Max,
} DwinProtoChanIdx_t;

typedef struct
{
    uint8_t length;
    uint8_t cmd;
    uint8_t data[249];
    uint8_t crc[2];
} DwinPacket_t; //包结构


#define DWIN_PACKET_TIMEOUT  (2500)  //包超时时间

void DwinProtocol_Init(void);

void DwinProtocol_Process(void);
void DwinProtocol_SendPack(DwinProtoChanIdx_t chan, const DwinPacket_t *packet);

