#include "cl_common.h"

#define SOG_PACKET_TIMEOUT  (500)  //包内超时

typedef enum
{
    YMK_SOH = 0x01,
    YMK_STX = 0x02,
    YMK_EOT = 0x04,
    YMK_ACK = 0x06,
    YMK_NAK = 0x15,
    YMK_CAN = 0x18,
    YMK_C = 0x43,

    YMK_NULL = 0x00,
    YMK_CPMEOF = 0x1a,
} YModemKeyword_t;

uint16_t CalcYmodemCrc(const uint8_t *data, int offset, int len);
