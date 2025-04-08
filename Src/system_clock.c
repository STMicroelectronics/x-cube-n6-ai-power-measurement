 /**
 ******************************************************************************
 * @file  system_clock.c
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

#include <stdio.h>

#include "system_clock.h"
#include "assert.h"

#include "app_config.h"
#include "stm32n6570_discovery.h"
#include "main.h"

/**
 * Clock initialization function based on the selected power mode.
 * HSE is used to avoid clock drifting when logging timestamps with TIM2
 * If we are in overdrive mode (POWER_OVERDRIVE=1), we choose 3 PLLs because we have 3 different frequencies:
 * 800MHz, 900MHz, and 1000MHz. PLL2 and PLL3 are only enabled for NPU inference.
 *
 * Overdrive mode
 *   HSE (48 MHz)
 *    ├── PLL1 (800 MHz)
 *    │   ├── CPU (800 MHz)
 *    │   ├── AXI (400 MHz)
 *    │   ├── CSI (20 MHz)
 *    │   └── DCMIPP (800/3=260 MHz)
 *    ├── PLL2 (1000 MHz)
 *    │   └── NPU (1000 MHz)
 *    └── PLL3 (900 MHz)
 *        └── AXISRAM3/4/5/6 (900 MHz)
 *
 * For nominal mode (POWER_OVERDRIVE=0), we have only 2 frequencies: 800MHz and 600MHz, so we use only 2 PLLs.
 * Both PLLs are always on, but the NPU and NPU RAM clocks are only activated for NPU inference.
 *
 * Nominal mode
 *   HSE (48 MHz)
 *    ├── PLL1 (800 MHz)
 *    │   ├── AXI (400 MHz)
 *    │   ├── CSI (20 MHz)
 *    │   ├── DCMIPP (800/3=260 MHz)
 *    │   ├── NPU (800 MHz)
 *    │   └── AXISRAM3/4/5/6 (800 MHz)
 *    └── PLL3 (600 MHz)
 *        └── CPU (600 MHz)
 *
 * When the CPU_FRQ_SCALE_DOWN mode is enabled, the CPU is clocked directly by the HSI and not by the PLL of the mode.
 * When the NPU_FRQ_SCALING mode is enabled, the POWER_OVERDRIVE mode is disabled in the code, and a different clock scheme is used during inference.
 * In this mode, several power consumption measurements are performed at different NPU/NPU RAM frequencies and power modes.
 * Therefore, we prefer to keep the same clock tree structure to compare results on the same basis and with the same configuration.
 *
 * Overdrive mode
 *   HSE (48 MHz)
 *    ├── PLL1 (800 MHz)
 *    │   ├── CPU (800 MHz)
 *    │   ├── AXI (400 MHz)
 *    │   ├── CSI (20 MHz)
 *    │   └── DCMIPP (800/3=260 MHz)
 *    ├── PLL2 (1000 MHz)
 *    │   └── NPU (1000 MHz)
 *    └── PLL3 (900 MHz)
 *        └── AXISRAM3/4/5/6 (900 MHz)
 *
 * Nominal mode
 *   HSE (48 MHz)
 *    ├── PLL1 (800 MHz)
 *    │   ├── AXI (400 MHz)
 *    │   ├── CSI (20 MHz)
 *    │   └── DCMIPP (800/3=260 MHz)
 *    ├── PLL2 (xx MHz)
 *    │   ├── NPU (step freq)
 *    │   └── AXISRAM3/4/5/6 (step freq)
 *    └── PLL3 (600 MHz)
 *        └── CPU (600 MHz)
 *
 * If the CPU_FRQ_SCALE_DOWN mode is enabled, the CPU frequency switches to the maximum of the mode (600 or 800MHz)
 * before inference and during post-processing. However, it switches back to 48MHz during the NPU hardware epochs.
 * We assume that during hardware inference, the CPU remains in sleep mode, so to reduce power consumption, we also reduce its frequency to 48MHz
*/


/**
  * @brief Congigure external SMPS power mode
  * @param SMPSVoltage_TypeDef voltMode nominal or overdrive
  * @retval None
  */
static void configPowerMode(SMPSVoltage_TypeDef voltMode)
{
  BSP_SMPS_Init(voltMode);
  HAL_Delay(10);
}

/**
  * @brief Switch clocks to PLL1 for NPU frequency scaling
  * @retval None
  * @note this function is used to switch NPU and NPU Rams and CPU to PLL1, to be able
  * to configure their PLLs and clock sources used in next inference
  */
static void npuFrqScaling_switchClocksToPll1(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK;

  /* NPU Clock (sysc_ck) */
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 200;
  /* AXISRAM3/4/5/6 Clock (sysd_ck) */
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 200;
  /* CPU clock */
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 2;

  int ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct);
  assert(ret == HAL_OK);
}

/**
  * @brief Configure PLLs for NPU frequency scaling
  * @param frequencySteps Pointer to FrequencyStep structure
  * @retval None
  */
static void npuFrqScaling_configurePlls(FrequencyStep *frequencySteps)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_OscInitStruct.PLL2 = frequencySteps->pll2Cfg;
  RCC_OscInitStruct.PLL3 = frequencySteps->pll3Cfg;
  int ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  assert(ret == HAL_OK);

  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);

  /* CPU clock */
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = frequencySteps->cpuClkSrc;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 1;

  /* NPU Clock (sysc_ck) */
  RCC_ClkInitStruct.IC6Selection.ClockSelection = frequencySteps->npuClkSrc;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 1;
  /* AXISRAM3/4/5/6 Clock (sysd_ck) */
  RCC_ClkInitStruct.IC11Selection.ClockSelection = frequencySteps->npuRamsClkSrc;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 1;

  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct);
  assert(ret == HAL_OK);
}

/**
  * @brief System clock configuration for NPU frequency scaling
  * @param frequencySteps Pointer to FrequencyStep structure
  * @retval None
  */
void sysclk_NpuFreqScaling(FrequencyStep *frequencySteps)
{
  /* switch CPU, NPU, NPURams clock source to PLL1 before modifying it */
  npuFrqScaling_switchClocksToPll1();

  /* if overdrive, increase VddCore before switching to freq max */
  if(frequencySteps->npufreq == 1000)
  {
    configPowerMode(SMPS_VOLTAGE_OVERDRIVE);
  }

  /* configure CPU and NPU and NPURams plls according to step config*/
  npuFrqScaling_configurePlls(frequencySteps);

  /* if nominal mode, decrease VddCore only after switching to nominal mode frequencies */
  if(frequencySteps->npufreq < 1000)
  {
    configPowerMode(SMPS_VOLTAGE_NOMINAL);
  }
}

/**
  * @brief Configure NPU clock for overdrive mode
  * @param pRCC_ClkInitStruct Pointer to RCC_ClkInitTypeDef structure
  * @retval None
  */
void sysclk_NpuOverDriveClockConfig(RCC_ClkInitTypeDef *pRCC_ClkInitStruct)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* PLL2 = 48 * 125 / 6 = 1000MHz */
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL2.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL2.PLLM = 6;
  RCC_OscInitStruct.PLL2.PLLFractional = 0;
  RCC_OscInitStruct.PLL2.PLLN = 125;
  RCC_OscInitStruct.PLL2.PLLP1 = 1;
  RCC_OscInitStruct.PLL2.PLLP2 = 1;

  int ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  assert(ret == HAL_OK);

  /* NPU Clock (sysc_ck) = ic6_ck = PLL2 output/ic6_divider = 1000/1 = 1000 MHz */
  pRCC_ClkInitStruct->IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL2;
  pRCC_ClkInitStruct->IC6Selection.ClockDivider = 1;

  ret = HAL_RCC_ClockConfig(pRCC_ClkInitStruct);
  assert(ret == HAL_OK);
}

/**
  * @brief Configure NPU RAMs clock for overdrive mode
  * @param pRCC_ClkInitStruct Pointer to RCC_ClkInitTypeDef structure
  * @retval None
  */
void sysclk_NpuRamsOverDriveClockConfig(RCC_ClkInitTypeDef *pRCC_ClkInitStruct)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* PLL3 = 48 x 75 / 4 = 900MHz */
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL3.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL3.PLLM = 4;
  RCC_OscInitStruct.PLL3.PLLN = 75;
  RCC_OscInitStruct.PLL3.PLLFractional = 0;
  RCC_OscInitStruct.PLL3.PLLP1 = 1;
  RCC_OscInitStruct.PLL3.PLLP2 = 1;

  int ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  assert(ret == HAL_OK);

  /* AXISRAM3/4/5/6 Clock (sysd_ck) = ic11_ck = PLL4 output/ic11_divider = 900 MHz */
  pRCC_ClkInitStruct->IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL3;
  pRCC_ClkInitStruct->IC11Selection.ClockDivider = 1;

  ret = HAL_RCC_ClockConfig(pRCC_ClkInitStruct);
  assert(ret == HAL_OK);
}

/**
  * @brief Deinitialize NPU overdrive PLL
  * @param pRCC_ClkInitStruct Pointer to RCC_ClkInitTypeDef structure
  * @retval None
  */
void sysclk_NpuOverDrivePllDeinit(RCC_ClkInitTypeDef *pRCC_ClkInitStruct)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  pRCC_ClkInitStruct->IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  pRCC_ClkInitStruct->IC6Selection.ClockDivider = 200;

  pRCC_ClkInitStruct->ClockType = (RCC_CLOCKTYPE_SYSCLK);
  pRCC_ClkInitStruct->SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  int ret = HAL_RCC_ClockConfig(pRCC_ClkInitStruct);
  assert(ret == HAL_OK);

  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_OFF;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  assert(ret == HAL_OK);
}

/**
  * @brief Deinitialize NPU RAMs overdrive clock
  * @param pRCC_ClkInitStruct Pointer to RCC_ClkInitTypeDef structure
  * @retval None
  */
void sysclk_NpuRamsOverDriveClockDeinit(RCC_ClkInitTypeDef *pRCC_ClkInitStruct)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  pRCC_ClkInitStruct->IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  pRCC_ClkInitStruct->IC11Selection.ClockDivider = 200;
  pRCC_ClkInitStruct->ClockType = (RCC_CLOCKTYPE_SYSCLK);
  pRCC_ClkInitStruct->SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  int ret = HAL_RCC_ClockConfig(pRCC_ClkInitStruct);
  assert(ret == HAL_OK);

  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_OFF;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  assert(ret == HAL_OK);
}

/**
  * @brief Enable NPU clock and reset IP
  * @retval None
  */
void sysclk_NpuClockEnable(void)
{
  __HAL_RCC_NPU_CLK_ENABLE();
  __HAL_RCC_NPU_CLK_SLEEP_ENABLE();

  __HAL_RCC_NPU_FORCE_RESET();
  __HAL_RCC_NPU_RELEASE_RESET();
}

/**
  * @brief Disable NPU clock
  * @retval None
  */
void sysclk_NpuClockDisable(void)
{
  __HAL_RCC_NPU_CLK_DISABLE();
  __HAL_RCC_NPU_CLK_SLEEP_DISABLE();
}

/**
  * @brief Configure NPU clock
  * @retval None
  */
void sysclk_NpuClockConfig(void)
{
 /* configure PLL for NPU and NPURams for overdrive mode
  * in case of nominal mode, the NPU and NPURams will use the PLL1 already configured */
#if(POWER_OVERDRIVE == 1)
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
  sysclk_NpuOverDriveClockConfig(&RCC_ClkInitStruct);
  sysclk_NpuRamsOverDriveClockConfig(&RCC_ClkInitStruct);
#endif /*POWER_OVERDRIVE*/
}

/**
  * @brief Configure CPU clock
  * @retval None
  */
void sysclk_CpuClockConfig(void)
{
#if(POWER_OVERDRIVE == 0)
#if(CPU_FRQ_SCALE_DOWN == 1)
  /* use Pll3 for cpu during epoch programming or for post proc */
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  /* PLL3 = 48 x 50 / 2 / 2 = 600MHz */
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL3.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL3.PLLM = 2;
  RCC_OscInitStruct.PLL3.PLLN = 50;
  RCC_OscInitStruct.PLL3.PLLFractional = 0;
  RCC_OscInitStruct.PLL3.PLLP1 = 2;
  RCC_OscInitStruct.PLL3.PLLP2 = 1;

  int ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  assert(ret == HAL_OK);

#endif /* CPU_FRQ_SCALE_DOWN */
#endif /*POWER_OVERDRIVE*/
}

/**
  * @brief Set CPU to maximum frequency
  * @retval None
  */
void sysclk_SetCpuMaxFreq(void)
{
#if (CPU_FRQ_SCALE_DOWN == 1)
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  int ret;

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;

#if(POWER_OVERDRIVE == 1)
  /* CPU @Sysa_ck = 800MHz */
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 1;
#else
  /* CPU CLock (sysa_ck) = ic1_ck = PLL3 output/ic1_divider = 600 MHz */
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL3;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 1;
#endif /* POWER_OVERDRIVE */

  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct);
  assert(ret == HAL_OK);
#endif /* CPU_FRQ_SCALE_DOWN */
}

/**
  * @brief Set CPU to minimum frequency
  * @retval None
  */
void sysclk_SetCpuMinFreq(void)
{
#if (CPU_FRQ_SCALE_DOWN == 1)
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  int ret;

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSE;

  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct);
  assert(ret == HAL_OK);
#endif /* CPU_FRQ_SCALE_DOWN */
}

#if(POWER_OVERDRIVE == 1)
/**
  * @brief System clock configuration for overdrive mode
  * @retval None
  */
static void sysclk_SystemClockConfig_Overdrive(void)
{
  int ret;

  /* configure external SMPS to deliver overdrive power 0.89vols */
  configPowerMode(SMPS_VOLTAGE_OVERDRIVE);

  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 0;

  /* PLL1 = 48 * 50 / 3 = 800MHz */
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL1.PLLM = 3;
  RCC_OscInitStruct.PLL1.PLLN = 50;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL1.PLLP1 = 1;
  RCC_OscInitStruct.PLL1.PLLP2 = 1;

  /* PLL2 = 48 * 125 / 6 = 1000MHz */
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_OFF;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_OFF;

  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  assert(ret == HAL_OK);

  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK |
                   RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                   RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK4 |
                   RCC_CLOCKTYPE_PCLK5);

#if(CPU_FRQ_SCALE_DOWN == 0)
  /* CPU @Sysa_ck = 800MHz */
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 1;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
#else
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
#endif /* CPU_FRQ_SCALE_DOWN */

  /* AXI Clock (sysb_ck) = ic2_ck = PLL1 output/ic2_divider = 800/2 = 400 MHz */
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider = 2;

  /* NPU Clock (sysc_ck) = ic6_ck = PLL2 output/ic6_divider = 1000/1 = 1000 MHz */
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 200;

  /* AXISRAM3/4/5/6 Clock (sysd_ck) = ic11_ck = PLL4 output/ic11_divider = 900 MHz */
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 200;

  /* HCLK = sysb_ck / HCLK divider = 200 MHz */
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;

  /* PCLKx = HCLK / PCLKx divider = 200 MHz */
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;

  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct);
  assert(ret == HAL_OK);

  RCC_PeriphCLKInitStruct.PeriphClockSelection = 0;

  /* XSPI1 kernel clock (ck_ker_xspi1) = HCLK = 200MHz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_XSPI1;
  RCC_PeriphCLKInitStruct.Xspi1ClockSelection = RCC_XSPI1CLKSOURCE_HCLK;

  /* XSPI2 kernel clock (ck_ker_xspi1) = HCLK =  200MHz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_XSPI2;
  RCC_PeriphCLKInitStruct.Xspi2ClockSelection = RCC_XSPI2CLKSOURCE_HCLK;

  ret = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
  assert(ret == HAL_OK);
}
#endif /* POWER_OVERDRIVE */

#if(POWER_OVERDRIVE == 0)
/**
  * @brief System clock configuration for nominal mode
  * @retval None
  */
static void sysclk_SystemClockConfig_Nominal(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 0;

  /* PLL1 = 48 * 50 / 3 = 800MHz */
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL1.PLLM = 3;
  RCC_OscInitStruct.PLL1.PLLN = 50;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL1.PLLP1 = 1;
  RCC_OscInitStruct.PLL1.PLLP2 = 1;

  /* PLL2 = 48 * 125 / 6 = 1000MHz */
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_OFF;

#if(CPU_FRQ_SCALE_DOWN == 1)
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_OFF;
#else
  /* PLL3 = 48 x 50 / 2 / 2 = 600MHz */
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL3.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL3.PLLM = 2;
  RCC_OscInitStruct.PLL3.PLLN = 50;
  RCC_OscInitStruct.PLL3.PLLFractional = 0;
  RCC_OscInitStruct.PLL3.PLLP1 = 2;
  RCC_OscInitStruct.PLL3.PLLP2 = 1;
#endif /* CPU_FRQ_SCALE_DOWN */

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while(1);
  }

  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK |
                   RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                   RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK4 |
                   RCC_CLOCKTYPE_PCLK5);

#if(CPU_FRQ_SCALE_DOWN == 0)
  /* CPU CLock (sysa_ck) = ic1_ck = PLL3 output/ic1_divider = 600 MHz */
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL3;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 1;
#else
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
#endif /* CPU_FRQ_SCALE_DOWN */

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  /* AXI Clock (sysb_ck) = ic2_ck = PLL1 output/ic2_divider = 400 MHz */
  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider = 2;

  /* NPU Clock (sysc_ck) = ic6_ck = PLL1 output/ic6_divider = 800 MHz */
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 1;

  /* AXISRAM3/4/5/6 Clock (sysd_ck) = ic11_ck = PLL1 output/ic11_divider = 800 MHz */
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 1;

  /* HCLK = sysb_ck / HCLK divider = 200 MHz */
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;

  /* PCLKx = HCLK / PCLKx divider = 200 MHz */
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
  {
    while(1);
  }

  RCC_PeriphCLKInitStruct.PeriphClockSelection = 0;

  /* XSPI1 kernel clock (ck_ker_xspi1) = HCLK = 200MHz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_XSPI1;
  RCC_PeriphCLKInitStruct.Xspi1ClockSelection = RCC_XSPI1CLKSOURCE_HCLK;

  /* XSPI2 kernel clock (ck_ker_xspi1) = HCLK =  200MHz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_XSPI2;
  RCC_PeriphCLKInitStruct.Xspi2ClockSelection = RCC_XSPI2CLKSOURCE_HCLK;

  if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
  {
    while (1);
  }
}
#endif /* POWER_OVERDRIVE */

/**
  * @brief System clock configuration
  * @retval None
  */
void sysclk_SystemClockConfig(void)
{
#if(POWER_OVERDRIVE == 1)
  sysclk_SystemClockConfig_Overdrive();
#else
  sysclk_SystemClockConfig_Nominal();
#endif
}

/**
  * @brief  DCMIPP Clock Config for DCMIPP.
  * @param  hcsi  DCMIPP Handle
  *     Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp)
{
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};
  HAL_StatusTypeDef ret = HAL_OK;

  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DCMIPP;
  RCC_PeriphCLKInitStruct.DcmippClockSelection = RCC_DCMIPPCLKSOURCE_IC17;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockDivider = 3;/* 800/3=260*/
  ret = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
  if (ret)
  {
    return ret;
  }

  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CSI;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockDivider = 40; /* 800/40=20Mhz */
  ret = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
  if (ret)
  {
    return ret;
  }

  return ret;
}

