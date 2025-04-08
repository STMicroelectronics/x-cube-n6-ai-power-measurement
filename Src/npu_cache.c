 /**
 ******************************************************************************
 * @file    npu_cache.c
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

#include "npu_cache.h"
#include "stm32n6xx_hal.h"

static CACHEAXI_HandleTypeDef hcacheaxi_s;

void HAL_CACHEAXI_MspInit(CACHEAXI_HandleTypeDef *hcacheaxi)
{
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_ENABLE();
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_CACHEAXI_CLK_ENABLE();
  __HAL_RCC_CACHEAXI_CLK_SLEEP_ENABLE();
  __HAL_RCC_CACHEAXI_FORCE_RESET();
  __HAL_RCC_CACHEAXI_RELEASE_RESET();
}

void HAL_CACHEAXI_MspDeInit(CACHEAXI_HandleTypeDef *hcacheaxi)
{
  __HAL_RCC_CACHEAXI_FORCE_RESET();
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_DISABLE();
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_SLEEP_DISABLE();
  __HAL_RCC_CACHEAXI_CLK_DISABLE();
  __HAL_RCC_CACHEAXI_CLK_SLEEP_DISABLE();
}

void npu_cache_init(void)
{
  hcacheaxi_s.Instance = CACHEAXI;
  HAL_CACHEAXI_Init(&hcacheaxi_s);
}

void npu_cache_deinit(void)
{
  HAL_CACHEAXI_DeInit(&hcacheaxi_s);
}

void npu_cache_enable(void)
{
  HAL_CACHEAXI_Enable(&hcacheaxi_s);
}

void npu_cache_disable(void)
{
  HAL_CACHEAXI_Disable(&hcacheaxi_s);
}

void npu_cache_invalidate(void)
{
  HAL_CACHEAXI_Invalidate(&hcacheaxi_s);
}

void npu_cache_clean_range(uint32_t start_addr, uint32_t end_addr)
{
  HAL_CACHEAXI_CleanByAddr(&hcacheaxi_s, (uint32_t*)start_addr, end_addr-start_addr);
}

void npu_cache_clean_invalidate_range(uint32_t start_addr, uint32_t end_addr)
{
  HAL_CACHEAXI_CleanInvalidByAddr(&hcacheaxi_s, (uint32_t*)start_addr, end_addr-start_addr);
}

void NPU_CACHE_IRQHandler(void)
{
  __NOP();
}
