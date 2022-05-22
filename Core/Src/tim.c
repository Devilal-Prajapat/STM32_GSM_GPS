/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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

/* Includes ------------------------------------------------------------------*/
#include "tim.h"

/* USER CODE BEGIN 0 */
#include "iwdg.h"
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

uint32_t one_sec_delay = ONE_SEC_DELAY;
uint32_t five_sec_delay = ONE_SEC_DELAY;
uint32_t ten_ms_delay = TEN_MILLISEC_DELAY;
uint32_t sim800_timeout = ONE_SEC_DELAY;
volatile uint32_t tim_counter;


volatile tim_flag_t tim_flag;
/* USER CODE END 0 */



/* TIM6 init function */
void MX_TIM6_Init(void)
{
	 /* USER CODE BEGIN */
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 31;   // timer frequency 1 MHz so tick every 1us
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999;    // timer period  1us * 1000 = 1 ms;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();		
  }
	HAL_TIM_Base_Start_IT(&htim6);
	 /* USER CODE END */

}
/* TIM7 init function */
void MX_TIM7_Init(void)
{
	
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 63999;   // tick at 2 ms
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 60000;                               // 2 minute
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM6)
  {
  /* USER CODE BEGIN TIM6_MspInit 0 */

  /* USER CODE END TIM6_MspInit 0 */
    /* TIM6 clock enable */
    __HAL_RCC_TIM6_CLK_ENABLE();

    /* TIM6 interrupt Init */
    HAL_NVIC_SetPriority(TIM6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);
  /* USER CODE BEGIN TIM6_MspInit 1 */

  /* USER CODE END TIM6_MspInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM7)
  {
  /* USER CODE BEGIN TIM7_MspInit 0 */

  /* USER CODE END TIM7_MspInit 0 */
    /* TIM7 clock enable */
    __HAL_RCC_TIM7_CLK_ENABLE();

    /* TIM7 interrupt Init */
    HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
  /* USER CODE BEGIN TIM7_MspInit 1 */

  /* USER CODE END TIM7_MspInit 1 */
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM6)
  {
  /* USER CODE BEGIN TIM6_MspDeInit 0 */

  /* USER CODE END TIM6_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM6_CLK_DISABLE();

    /* TIM6 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM6_IRQn);
  /* USER CODE BEGIN TIM6_MspDeInit 1 */

  /* USER CODE END TIM6_MspDeInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM7)
  {
  /* USER CODE BEGIN TIM7_MspDeInit 0 */

  /* USER CODE END TIM7_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM7_CLK_DISABLE();

    /* TIM7 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM7_IRQn);
  /* USER CODE BEGIN TIM7_MspDeInit 1 */

  /* USER CODE END TIM7_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM6)
	{
		if(tim_counter == ten_ms_delay)
		{
			tim_flag.tim_ten_ms_flag = 1;
			ten_ms_delay += TEN_MILLISEC_DELAY;
		}
		
		if(tim_counter == one_sec_delay)
		{
			IWDG_Refresh();
			tim_flag.tim_one_sec_flag = 1;
			one_sec_delay += ONE_SEC_DELAY;
		}
		
		if(tim_counter == five_sec_delay)
		{
			IWDG_Refresh();
			tim_flag.tim_five_sec_flag = 1;
			five_sec_delay += FIVE_SEC_DELAY;
		}
		
		if(tim_counter == sim800_timeout)
		{
			tim_flag.sim800_timeout_flag = 1;
			sim800_timeout += ONE_SEC_DELAY;
		}
			
		tim_flag.tim_one_ms_flag = 1;
		tim_counter++;
	}
  
  if(htim->Instance == TIM7)
	{
		tim_flag.tim_one_min_flag = 1;
	}
}

/*!
 *  brief 10ms over flow 
 *  param None
 *  retval true or false
 */
uint8_t tim_ten_ms_overflow(void)
{
	if(tim_flag.tim_ten_ms_flag)
	{
		tim_flag.tim_ten_ms_flag = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}

/*!
 *  brief 1 sec over flow 
 *  param None
 *  retval true or false
 */
uint8_t tim_one_sec_overflow(void)
{
	if(tim_flag.tim_one_sec_flag)
	{
		tim_flag.tim_one_sec_flag = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}


/*!
 *  brief 1 sec over flow 
 *  param None
 *  retval true or false
 */
uint8_t tim_five_sec_overflow(void)
{
	if(tim_flag.tim_five_sec_flag)
	{
		tim_flag.tim_five_sec_flag = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}


/*!
 *  brief sim800_timeout over flow 
 *  param None
 *  retval true or false
 */
uint8_t sim800_timer_overflow(void)
{
	if(tim_flag.sim800_timeout_flag)
	{
		tim_flag.sim800_timeout_flag = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
