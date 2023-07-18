#pragma once

#include "cl_common.h"
#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------event type------------------
typedef enum
{
    CL_Event_RecvMcuxx = 0,
    CL_Event_RecvFwupgrade,
    CL_Event_RecvYmodemPack,
    CL_EventMax,
} CL_Event_t;

//---------------log-------------------------
#include "stdio.h"
#define USE_LDB_LOG
#define CL_PRINTF   printf

#ifdef __cplusplus
}
#endif
