/**
 ******************************************************************************
 * @file    system_clock.h
 * @author  GPM Application Team
 *
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

#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include "stm32n6xx_hal.h"

typedef struct
{
	RCC_PLLInitTypeDef pll2Cfg;
	RCC_PLLInitTypeDef pll3Cfg;
	uint32_t npufreq;
	uint32_t cpuClkSrc;
	uint32_t npuClkSrc;
	uint32_t npuRamsClkSrc;
    const char* stepName;
} FrequencyStep;



void sysclk_NpuFreqScaling(FrequencyStep *frequencySteps);
void sysclk_SystemClockConfig(void);
void sysclk_NpuOverDriveClockConfig(RCC_ClkInitTypeDef *pRCC_ClkInitStruct);
void sysclk_NpuRamsOverDriveClockConfig(RCC_ClkInitTypeDef *pRCC_ClkInitStruct);
void sysclk_NpuOverDrivePllDeinit(RCC_ClkInitTypeDef *pRCC_ClkInitStruct);
void sysclk_NpuRamsOverDriveClockDeinit(RCC_ClkInitTypeDef *pRCC_ClkInitStruct);
void sysclk_SetCpuMaxFreq(void);
void sysclk_SetCpuMinFreq(void);
void sysclk_NpuClockEnable(void);
void sysclk_NpuClockDisable(void);
void sysclk_NpuClockConfig(void);
void sysclk_CpuClockConfig(void);

#endif /* SYSTEM_CLOCK_H */
