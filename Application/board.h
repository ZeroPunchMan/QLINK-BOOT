#include "stm32f0xx_ll_gpio.h"

/**************************
 * 按键 端口配置
 *************************/
#define LIGHT_BTN_PORT (GPIOB)
#define LIGHT_BTN_PIN (LL_GPIO_PIN_0)

/**************************
 * led 端口配置
 *************************/
#define MCU_STA_LED_PORT (GPIOA)
#define MCU_STA_LED_PIN (LL_GPIO_PIN_0)