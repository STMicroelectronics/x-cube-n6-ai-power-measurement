/**
  ******************************************************************************
  * @file    trace_gpio.c
  * @author  MCD Application Team
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

#include "trace_gpio.h"

#include "main.h"

#define TRACE_GPIO_0 GPIOG
#define TRACE_PIN_0 GPIO_PIN_2
#define TRACE_GPIO_1 GPIOH
#define TRACE_PIN_1 GPIO_PIN_8
#define TRACE_GPIO_2 GPIOE
#define TRACE_PIN_2 GPIO_PIN_15
#define TRACE_GPIO_3 GPIOC
#define TRACE_PIN_3 GPIO_PIN_6


static int trace_state;

void trace_gpio_enable(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  HAL_PWREx_EnableVddIO4();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  GPIO_InitStruct.Pin = TRACE_PIN_0;
  HAL_GPIO_Init(TRACE_GPIO_0, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TRACE_PIN_1;
  HAL_GPIO_Init(TRACE_GPIO_1, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TRACE_PIN_2;
  HAL_GPIO_Init(TRACE_GPIO_2, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TRACE_PIN_3;
  HAL_GPIO_Init(TRACE_GPIO_3, &GPIO_InitStruct);

  trace_state = 0;
  HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TRACE_GPIO_1, TRACE_PIN_1, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TRACE_GPIO_2, TRACE_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TRACE_GPIO_3, TRACE_PIN_3, GPIO_PIN_RESET);
}

void trace_gpio_next_state(void)
{
  switch(trace_state + 1) {
  case 1:
    HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_SET);
    break;
  case 2:
    HAL_GPIO_WritePin(TRACE_GPIO_1, TRACE_PIN_1, GPIO_PIN_SET);
    break;
  case 3:
    HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_RESET);
    break;
  case 4:
    HAL_GPIO_WritePin(TRACE_GPIO_2, TRACE_PIN_2, GPIO_PIN_SET);
    break;
  case 5:
    HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_SET);
    break;
  case 6:
    HAL_GPIO_WritePin(TRACE_GPIO_1, TRACE_PIN_1, GPIO_PIN_RESET);
    break;
  case 7:
    HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_RESET);
    break;
  case 8:
    HAL_GPIO_WritePin(TRACE_GPIO_3, TRACE_PIN_3, GPIO_PIN_SET);
    break;
  case 9:
    HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_SET);
    break;
  case 10:
    HAL_GPIO_WritePin(TRACE_GPIO_1, TRACE_PIN_1, GPIO_PIN_SET);
    break;
  case 11:
    HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_RESET);
    break;
  case 12:
    HAL_GPIO_WritePin(TRACE_GPIO_2, TRACE_PIN_2, GPIO_PIN_RESET);
    break;
  case 13:
    HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_SET);
    break;
  case 14:
    HAL_GPIO_WritePin(TRACE_GPIO_1, TRACE_PIN_1, GPIO_PIN_RESET);
    break;
  case 15:
    HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_RESET);
    break;
  default:
    break;
  }
  trace_state++;
}


void trace_reset_state(void)
{
  trace_state = 0;
  HAL_GPIO_WritePin(TRACE_GPIO_0, TRACE_PIN_0, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TRACE_GPIO_1, TRACE_PIN_1, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TRACE_GPIO_2, TRACE_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TRACE_GPIO_3, TRACE_PIN_3, GPIO_PIN_RESET);
}
