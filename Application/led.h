#pragma once

#include "cl_common.h"


void Led_Init(void);

void Led_Process(void);

void SetPowerButtonLed(bool on);
void SetIndicationLed(bool on);

typedef enum
{
    McuLedStyle_FastBlink,
    McuLedStyle_NormalBlink,
    McuLedStyle_SlowBlink,
} McuLedStyle_t;

void SetMcuLedStyle(McuLedStyle_t style);

