#pragma once

#include "cl_common.h"
#include "systime.h"

#define TARGET_PRODUCT_ID_0 (0x00)
#define TARGET_PRODUCT_ID_1 (0x15)

#define SOG_RESEND_TIME  (SYSTIME_SECOND(10)) //包间超时时间

void SogYmodem_Init(void);
void SogYmodem_Process(void);


