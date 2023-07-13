#include "button.h"
#include "board.h"
#include "main.h"
#include "systime.h"
#include "string.h"
#include "cl_log.h"
#include "comm.h"

typedef enum
{
    ButtonEvent_Down,      // 按下
    ButtonEvent_Click,     // 点击, 短按松开
    ButtonEvent_LongPress, // 长按
    ButtonEvent_LpUp,      // 长按松开
} ButtonEvent_t;

typedef void (*BtnEventCallback)(ButtonEvent_t event);

typedef struct
{
    GPIO_TypeDef *port;
    uint32_t pin;
    uint8_t press;
    BtnEventCallback evtCb;
} ButtonDef_t;

typedef enum
{
    BtnIdx_Light = 0,
    BtnIdx_Max,
} ButtonIdx_t;

void OnLightBtnEvent(ButtonEvent_t evt);

const ButtonDef_t buttonDef[BtnIdx_Max] =
    {
        [BtnIdx_Light] = {
            .port = LIGHT_BTN_PORT,
            .pin = LIGHT_BTN_PIN,
            .press = 0,
            .evtCb = OnLightBtnEvent,
        },
};

typedef enum
{
    BtnSta_Up,
    BtnSta_Press,
    BtnSta_LongPress,
} ButtonStatus_t;

typedef struct
{
    ButtonStatus_t status;
    uint32_t downTime;
    uint32_t upTime;
} ButtonContext_t;

ButtonContext_t buttonContext[BtnIdx_Max];

void Button_Init(void)
{
    for (int i = 0; i < BtnIdx_Max; i++)
    {
        buttonContext[i].status = BtnSta_Up;
        buttonContext[i].downTime = 0;
    }
}

static inline bool IsButtonDown(ButtonIdx_t butIdx)
{
    return LL_GPIO_IsInputPinSet(buttonDef[butIdx].port, buttonDef[butIdx].pin) == 0;
}

void Button_Process(void)
{
    static uint32_t lastTime = 0;
    if (lastTime == 0)
    {
        lastTime = GetSysTime();
    }
    else
    {
        uint32_t span = SysTimeSpan(lastTime);
        lastTime = GetSysTime();

        for (int i = 0; i < BtnIdx_Max; i++)
        {
            ButtonContext_t *bc = &buttonContext[i];
            if (bc->status == BtnSta_Up)
            { // 此时状态是未按下
                if (IsButtonDown((ButtonIdx_t)i))
                { // 持续按下
                    bc->downTime += span;
                }
                else
                { // 松开
                    bc->downTime = 0;
                }

                if (bc->downTime >= BUTTON_BOUNCE_TIME)
                { // 持续超过去抖时间,按键改为按下状态
                    bc->status = BtnSta_Press;
                    // CL_LOG_LINE("button down");
                    buttonDef[i].evtCb(ButtonEvent_Down);
                }
            }
            else if (bc->status == BtnSta_Press)
            { // 此时状态是已按下
                if (IsButtonDown((ButtonIdx_t)i))
                { // 持续按下
                    bc->downTime += span;
                }
                else
                {
                    bc->downTime = 0;
                    bc->status = BtnSta_Up;
                    // 短按事件
                    // CL_LOG_LINE("button %d click", i);
                    buttonDef[i].evtCb(ButtonEvent_Click);
                }

                if (bc->downTime >= BUTTON_LONG_PRESS_TIME)
                {
                    bc->status = BtnSta_LongPress;
                    // CL_LOG_LINE("button %d long press", i);

                    // 长按事件
                    buttonDef[i].evtCb(ButtonEvent_LongPress);
                }
            }
            else if (bc->status == BtnSta_LongPress)
            { // 此时是长按
                if (IsButtonDown((ButtonIdx_t)i))
                { // 持续按下
                    bc->downTime += span;
                }
                else
                { // 松开
                    bc->downTime = 0;
                    bc->status = BtnSta_Up;
                    // CL_LOG_LINE("button up");
                    buttonDef[i].evtCb(ButtonEvent_LpUp);
                }
            }
        }
    }
}

void OnLightBtnEvent(ButtonEvent_t evt)  
{
    switch (evt)
    {
    case ButtonEvent_Down:
        CL_LOG_LINE("light button down");
        Comm_SendKeyEvent();
        break;
    case ButtonEvent_Click:
        CL_LOG_LINE("light button click");
        break;
    case ButtonEvent_LongPress:
        CL_LOG_LINE("light button long press");
        break;
    case ButtonEvent_LpUp:
        CL_LOG_LINE("light button lpress up");
        break;
    }
}

