/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;

/* USER CODE BEGIN Private defines */

#define 	TEN_MILLISEC_DELAY    10
#define   ONE_SEC_DELAY         1000
#define   TWO_SEC_DELAY         2000
#define   THREE_SEC_DELAY       3000
#define   FIVE_SEC_DELAY        5000

/*!
 * timer flags 
 */
typedef struct tim_flag
{
	uint8_t sim800_timeout_flag: 1;
	uint8_t tim_five_sec_flag  : 1;
	uint8_t tim_one_sec_flag   : 1;
	uint8_t tim_ten_ms_flag    : 1;
	uint8_t tim_one_ms_flag    : 1;
  uint8_t tim_one_min_flag   : 1;
	uint8_t time_reserved_flag : 3;
}tim_flag_t;

/* USER CODE END Private defines */

void MX_TIM6_Init(void);   // multiple time out timer
void MX_TIM7_Init(void);  // 1 minute timer

/* USER CODE BEGIN Prototypes */
uint8_t tim_five_sec_overflow(void);
uint8_t tim_one_sec_overflow(void);
uint8_t tim_ten_ms_overflow(void);
uint8_t sim800_timer_overflow(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
