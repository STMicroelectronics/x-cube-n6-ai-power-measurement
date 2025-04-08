/**
 ******************************************************************************
 * @file    ll_aton_osal.h
 * @author  SRA Artificial Intelligence & Embedded Architectures
 * @brief   Header file for defining an abstraction of RTOS differences
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

#ifndef __LL_ATON_OSAL_USER_H
#define __LL_ATON_OSAL_USER_H


#include "ll_aton_platform.h"

extern void sysclk_SetCpuMaxFreq(void);
extern void sysclk_SetCpuMinFreq(void);
extern void HAL_PWR_EnterSLEEPMode(uint32_t Regulator, uint8_t SLEEPEntry);

/* Macros for (de-)initialization of OSAL layer */
#define LL_ATON_OSAL_INIT()
#define LL_ATON_OSAL_DEINIT()

/* Wait for / signal event from ATON runtime HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFE);  */
#define LL_ATON_OSAL_WFE()  do { sysclk_SetCpuMinFreq();\
	                             __WFE();\
	                             sysclk_SetCpuMaxFreq();\
	                           } while(0)

#define LL_ATON_OSAL_SIGNAL_EVENT()

#endif // __LL_ATON_OSAL_USER_H
