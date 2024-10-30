#pragma once
#include "main.h"

//*************蓝牙模块****************
#define BLE_CONN_PORT (GPIOA)
#define BLE_CONN_PIN (LL_GPIO_PIN_12)

/**************************
 * led 端口配置
 *************************/
#define MCU_STA_LED_PORT (GPIOB)
#define MCU_STA_LED_PIN (LL_GPIO_PIN_13)

//***************buzzer**********************
#define BUZZ_PORT (GPIOC)
#define BUZZ_PIN (LL_GPIO_PIN_13)

//***************chan1**********************
#define CHAN1_PWR_PORT  (GPIOA)
#define CHAN1_PWR_PIN  (LL_GPIO_PIN_4)

#define CHAN1_FREQ_PORT  (GPIOB)
#define CHAN1_FREQ_PIN  (LL_GPIO_PIN_1)

#define CHAN1_M1_PORT  (GPIOA)
#define CHAN1_M1_PIN  (LL_GPIO_PIN_6)

#define CHAN1_M2_PORT  (GPIOA)
#define CHAN1_M2_PIN  (LL_GPIO_PIN_5)

//***************chan2**********************
#define CHAN2_PWR_PORT  (GPIOB)
#define CHAN2_PWR_PIN  (LL_GPIO_PIN_9)

#define CHAN2_FREQ_PORT  (GPIOA)
#define CHAN2_FREQ_PIN  (LL_GPIO_PIN_8)

#define CHAN2_M1_PORT  (GPIOA)
#define CHAN2_M1_PIN  (LL_GPIO_PIN_0)

#define CHAN2_M2_PORT  (GPIOA)
#define CHAN2_M2_PIN  (LL_GPIO_PIN_1)

//***************chan3**********************
#define CHAN3_PWR_PORT  (GPIOB)
#define CHAN3_PWR_PIN  (LL_GPIO_PIN_8)

#define CHAN3_FREQ_PORT  (GPIOB)
#define CHAN3_FREQ_PIN  (LL_GPIO_PIN_5)

#define CHAN3_M1_PORT  (GPIOA)
#define CHAN3_M1_PIN  (LL_GPIO_PIN_2)

#define CHAN3_M2_PORT  (GPIOA)
#define CHAN3_M2_PIN  (LL_GPIO_PIN_3)

//*************heat****************
#define HEAT_PORT  (GPIOA)
#define HEAT_PIN  (LL_GPIO_PIN_10)
