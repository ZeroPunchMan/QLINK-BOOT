#include "cl_common.h"


typedef struct
{
    uint8_t head;
    uint8_t packNum, invPackNum;
    uint8_t data[1024];
    uint16_t crc;

    uint16_t dataLen;
} YmodemPacket_t;

const YmodemPacket_t* YmodemParseByte(uint8_t byte);
