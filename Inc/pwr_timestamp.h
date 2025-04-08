/**
  ******************************************************************************
  * @file    pwr_timestamp.h
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

#ifndef PWR_TIMESTAMP_H
#define PWR_TIMESTAMP_H

void pwr_timestamp_init(void);
void pwr_timestamp_log(const char *stepName);
void pwr_timestamp_sendOverUart(void);

void pwr_timestamp_stop(void);
void pwr_timestamp_start(void);

#endif
