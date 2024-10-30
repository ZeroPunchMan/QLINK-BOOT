/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
CL_QUEUE_DEF_INIT(usart1RecvQueue, 256, uint8_t, );
CL_QUEUE_DEF_INIT(usart1SendQueue, 256, uint8_t, );

CL_QUEUE_DEF_INIT(usart2RecvQueue, 256, uint8_t, );
CL_QUEUE_DEF_INIT(usart2SendQueue, 256, uint8_t, );
/* USER CODE END 0 */

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  /**USART1 GPIO Configuration
  PB6   ------> USART1_TX
  PB7   ------> USART1_RX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USART1 interrupt Init */
  NVIC_SetPriority(USART1_IRQn, 0);
  NVIC_EnableIRQ(USART1_IRQn);

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  USART_InitStruct.BaudRate = 115200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);
  LL_USART_DisableIT_CTS(USART1);
  LL_USART_ConfigAsyncMode(USART1);
  LL_USART_Enable(USART1);
  /* USER CODE BEGIN USART1_Init 2 */
  LL_USART_EnableIT_RXNE(USART1);

  /* USER CODE END USART1_Init 2 */

}

/* USER CODE BEGIN 1 */
void Usart_Exit(USART_TypeDef *Usartx)
{
    LL_USART_DisableIT_TXE(Usartx);
    LL_USART_DisableIT_RXNE(Usartx);
    LL_USART_Disable(Usartx);
    LL_USART_DeInit(Usartx);
    
    if (Usartx == USART2)
    {
        LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART2);
        NVIC_DisableIRQ(USART2_IRQn);
    }
    else if (Usartx == USART1)
    {
        LL_APB1_GRP2_DisableClock(LL_APB1_GRP2_PERIPH_USART1);
        NVIC_DisableIRQ(USART1_IRQn);
    }
}

static inline void EnableTxe(USART_TypeDef *Usartx)
{
    LL_USART_EnableIT_TXE(Usartx);
}

CL_Result_t Usartx_Send(USART_TypeDef *Usartx, const uint8_t *data, uint16_t offset, uint16_t len)
{
    CL_QueueInfo_t *queue;
    if (Usartx == USART2)
        queue = &usart2SendQueue;
    else
        return CL_ResInvalidParam;

    if (CL_QueueFreeSpace(queue) < len)
        return CL_ResFailed;

    for (uint16_t i = 0; i < len; i++)
        CL_QueueAdd(queue, (void *)&data[i + offset]);

    EnableTxe(Usartx);

    return CL_ResSuccess;
}

//--------------------------------------

// #include "stdio.h"
// #pragma import(__use_no_semihosting)

// struct __FILE
// {
//     int handle;
// };

// FILE __stdout;

// void _sys_exit(int x)
// {
//     x = x;
// }

// int fputc(int ch, FILE *f)
// {
//     uint8_t data = ch;
//     CL_QueueAdd(&usart1SendQueue, &data);
//     EnableTxe(USART1);
//     return ch;
// }
/* USER CODE END 1 */
