#pragma once
#include "main.h"

//*************蓝牙模块****************
#define BLE_RST_PORT (GPIOA)
#define BLE_RST_PIN (LL_GPIO_PIN_5)

#define BLE_PWR_PORT (GPIOB)
#define BLE_PWR_PIN (LL_GPIO_PIN_2)

#define BLE_CONN_PORT (GPIOB)
#define BLE_CONN_PIN (LL_GPIO_PIN_12)

/**************************
 * led 端口配置
 *************************/
#define MCU_STA_LED_PORT (GPIOA)
#define MCU_STA_LED_PIN (LL_GPIO_PIN_0)

//***************按键**********************
#define KEY1_PORT (GPIOB)
#define KEY1_PIN (LL_GPIO_PIN_7)

#define KEY2_PORT (GPIOB)
#define KEY2_PIN (LL_GPIO_PIN_6)

#define KEY3_PORT (GPIOB)
#define KEY3_PIN (LL_GPIO_PIN_5)

//******************LED*************************
#define LED1_EN_PORT (GPIOA)
#define LED1_EN_PIN (LL_GPIO_PIN_8)

#define LED2_EN_PORT (GPIOB)
#define LED2_EN_PIN (LL_GPIO_PIN_15)

#define LED3_EN_PORT (GPIOB)
#define LED3_EN_PIN (LL_GPIO_PIN_14)

#define LED4_EN_PORT (GPIOB)
#define LED4_EN_PIN (LL_GPIO_PIN_13)

#define MCU_STA_LED_PORT (GPIOA)
#define MCU_STA_LED_PIN (LL_GPIO_PIN_0)
