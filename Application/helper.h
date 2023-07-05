#pragma once

#include "cl_queue.h"


typedef struct
{
    CL_QueueInfo_t *queue;
    uint32_t count;
    bool status;
} StatusFilter_t;

#define SMOOTH_FILTER_DEF(name, length, modifier)       \
    CL_QUEUE_DEF_INIT(name##_q, length, bool, modifier) \
    modifier StatusFilter_t name = {.queue = &name##_q, .count = 0, .status = false};

bool StatusFilter(StatusFilter_t *filter, bool newStatus);


int16_t NtcAdcToTemp_10K(uint32_t adc);
bool Ntc10kTableCheck(void);
