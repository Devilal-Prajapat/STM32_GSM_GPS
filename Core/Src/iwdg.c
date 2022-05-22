/***************************************************************************
 * 
 *  @brief  watchdog implementation
 *  @file   iwdg.c
 *  @date   17.03.2022 
 *  @author Devilal Prajapat 
 *
 *****************************************************************************/
 
#include "main.h"
#include "iwdg.h"

IWDG_HandleTypeDef hiwdg;

/*
 *  Init IWDG
 */
void IWDG_Init(void)
{
	hiwdg.Instance = IWDG;
	hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
	hiwdg.Init.Reload = 4095;
	hiwdg.Init.Window = 4095;
	HAL_IWDG_Init(&hiwdg);
}

/*
 *  Refresh iwdg 
 */
void IWDG_Refresh(void)
{
	HAL_IWDG_Refresh(&hiwdg);
}

void IWDG_Freeze(void)
{
	
}
