/**
  ******************************************************************************
  * @file    trace_gpio.h
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

#ifndef TRACE_GPIO_H
#define TRACE_GPIO_H

void trace_gpio_enable(void);
void trace_gpio_next_state(void);
void trace_reset_state(void);

void timestamp_init(void);

#endif
