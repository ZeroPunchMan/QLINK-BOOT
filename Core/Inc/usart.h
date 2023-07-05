/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "cl_queue.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

CL_QUEUE_DECL(usart1RecvQueue);
CL_QUEUE_DECL(usart1SendQueue);

CL_QUEUE_DECL(usart2RecvQueue);
CL_QUEUE_DECL(usart2SendQueue);
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
//***************************usart1*******************************
//��usart1���Ͷ��л�ȡ�ֽ�
static inline CL_Result_t Usart1_PollSendByte(volatile uint8_t* out)
{
    return CL_QueuePoll(&usart1SendQueue, (void *)out);
}

//��usart1���ն��л�ȡ1�ֽ�
static inline CL_Result_t Usart1_PollRecvByte(volatile uint8_t* out)
{
    return CL_QueuePoll(&usart1RecvQueue, (void *)out);
}

//��usart1���ն�������1�ֽ�
static inline CL_Result_t Usart1_AddRecvByte(volatile uint8_t in)
{
    return CL_QueueAdd(&usart1RecvQueue, (void *)&in);
}

//************************usart2*****************************************
//��usart2���Ͷ��л�ȡ�ֽ�
static inline CL_Result_t Usart2_PollSendByte(volatile uint8_t* out)
{
    return CL_QueuePoll(&usart2SendQueue, (void *)out);
}

//��usart2���ն��л�ȡ1�ֽ�
static inline CL_Result_t Usart2_PollRecvByte(volatile uint8_t* out)
{
    return CL_QueuePoll(&usart2RecvQueue, (void *)out);
}

//��usart2���ն�������1�ֽ�
static inline CL_Result_t Usart2_AddRecvByte(volatile uint8_t in)
{
    return CL_QueueAdd(&usart2RecvQueue, (void *)&in);
}

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

