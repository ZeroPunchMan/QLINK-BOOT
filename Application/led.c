#include "led.h"
#include "board.h"
#include "systime.h"
#include "cl_log.h"

typedef void (*InitFunc)(void);
typedef void (*SwitchFunc)(bool on);

static void PwrBtnLed_Switch(bool on);
static void IndLed_Switch(bool on);
static void McuStaLed_Switch(bool on);

typedef struct
{
    InitFunc initFunc;
    SwitchFunc switchFunc;
} LedContext_t;

typedef enum
{
    LedIdx_McuStatus,
    LedIdx_Max,
} LedIndex_t;

const LedContext_t ledContext[LedIdx_Max] = {
    [LedIdx_McuStatus] = {.initFunc = NULL, .switchFunc = McuStaLed_Switch},
};

static void McuStaLed_Switch(bool on)
{
    if (on)
        LL_GPIO_SetOutputPin(MCU_STA_LED_PORT, MCU_STA_LED_PIN);
    else
        LL_GPIO_ResetOutputPin(MCU_STA_LED_PORT, MCU_STA_LED_PIN);
}

//------------------------------------------
static McuLedStyle_t mcuLedStyle = McuLedStyle_SlowBlink;

void McuLedProc(void)
{
    uint32_t blinkInterval = 180;

    if (mcuLedStyle == McuLedStyle_NormalBlink)
        blinkInterval = 600;
    else if (mcuLedStyle == McuLedStyle_SlowBlink)
        blinkInterval = 1500;

    static uint32_t lastTime = 0;
    static bool ledOn = false;
    if (SysTimeSpan(lastTime) >= blinkInterval)
    {
        lastTime = GetSysTime();

        ledOn = !ledOn;
        ledContext[LedIdx_McuStatus].switchFunc(ledOn);

        // CL_LOG_LINE("ble led: %d", ledOn);
    }
}

//------------------------------------------
void Led_Init(void)
{
    for (int i = 0; i < LedIdx_Max; i++)
    {
        if (ledContext[i].initFunc != NULL)
            ledContext[i].initFunc();
    }
}

void Led_Process(void)
{
    McuLedProc();
}

void SetPowerButtonLed(bool on)
{
    ledContext[LedIdx_PwrBtn].switchFunc(on);
}

void SetIndicationLed(bool on)
{
    ledContext[LedIdx_Indication].switchFunc(on);
}

void SetMcuLedStyle(McuLedStyle_t style)
{
    mcuLedStyle = style;
}
