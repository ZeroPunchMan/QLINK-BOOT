#pragma once

#include "cl_common.h"
#include "stdio.h"

#ifdef __cplusplus
extern "C"
{
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
#define CL_PRINTF printf

// #define CL_LOG_LEVEL_INFO
// #define CL_LOG_LEVEL_WARN
// #define CL_LOG_LEVEL_ERROR

#ifdef __cplusplus
}
#endif

