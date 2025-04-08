/**
  ******************************************************************************
  * @file    pwr_timestamp.c
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
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "pwr_timestamp.h"
#include "main.h"

/* Timer handle declaration */
TIM_HandleTypeDef htim2;

/* Define the prescaler value for the timer */
#define PRESCALER_VALUE (uint32_t)(((400000000) / (1000000)) - 1)

/* Define the maximum number of log entries */
#define MAX_LOG_ENTRIES 100

/* structure for log entries */
typedef struct 
{
  const char* name;
  uint32_t timestamp;

      uint32_t divenr;
      uint32_t miscenr;
      uint32_t memenr;
      uint32_t ahb1enr;
      uint32_t ahb2enr;
      uint32_t ahb3enr;
      uint32_t ahb4enr;
      uint32_t ahb5enr;
      uint32_t apb1lenr;
      uint32_t apb1henr;
      uint32_t apb2enr;
      uint32_t apb3enr;
      uint32_t apb4lenr;
      uint32_t apb4henr;
      uint32_t apb5enr;

} LogEntry_t;

/* Buffer to store log entries */
__attribute__ ((aligned (32)))
LogEntry_t logBuffer[MAX_LOG_ENTRIES];

/* Static variables to keep track of timer state and log index */
static uint32_t tim_started = 0;
static uint32_t logIndex = 0;

/* Define constants for log formatting */
#define MAX_FORMATTED_RECORD_LENGTH 100
#define LOG_FORMAT "[SLP_SOL]%.30s:%lu:us:" \
                   "DIVENR=%lu:" \
                   "MISCENR=%lu:" \
                   "MEMENR=%lu:" \
                   "AHB1ENR=%lu:" \
                   "AHB2ENR=%lu:" \
                   "AHB3ENR=%lu:" \
                   "AHB4ENR=%lu:" \
                   "AHB5ENR=%lu:" \
                   "APB1LENR=%lu:" \
                   "APB1HENR=%lu:" \
                   "APB2ENR=%lu:" \
                   "APB3ENR=%lu:" \
                   "APB4LENR=%lu:" \
                   "APB4HENR=%lu:" \
                   "APB5ENR=%lu[SLP_EOL]"
#define END_OF_LOG "[SLP_SOL]END_OF_LOG[SLP_EOL]\0"

/**
  * @brief Function to initialize the timer
  * @retval None
  */
static void timer_init(void)
{
  /* USER CODE BEGIN TIM2_Init 0 */
  /* USER CODE END TIM2_Init 0 */
  
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  
  /* USER CODE BEGIN TIM2_Init 1 */
  __HAL_RCC_TIM2_CLK_ENABLE();
  /* USER CODE END TIM2_Init 1 */
  
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = PRESCALER_VALUE;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xffffffff;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    while(1);
  }
  
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    while(1);
  }
  
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    while(1);
  }
}

/**
  * @brief Function to start the timestamping
  * @retval None
  */
void pwr_timestamp_start(void) 
{
  if (tim_started == 0)
  {
    tim_started = 1;
    __HAL_TIM_SET_COUNTER(&htim2, 0);
  }
  /* Reset variables */
  logIndex = 0;
  
  /* Start timer */
  HAL_TIM_Base_Start(&htim2);
}

/**
  * @brief Function to stop the timestamping
  * @retval None
  */
void pwr_timestamp_stop(void)
{
  /* Stop timer */
  HAL_TIM_Base_Stop(&htim2);
  tim_started = 0;
}

/**
  * @brief Function to initialize the timestamping system
  * @retval None
  */
void pwr_timestamp_init(void)
{
  /* Configure timer */
  timer_init();
  
  /* Clear buffer */
  memset(logBuffer, 0, sizeof(logBuffer));
}

/**
  * @brief Function to log a timestamp with a step name and clocked IPs
  * @param stepName Name of the step to log
  * @retval None
  */
void pwr_timestamp_log(const char *stepName)
{
  assert(logIndex < MAX_LOG_ENTRIES);
  
  uint32_t timer_cnt_val = __HAL_TIM_GET_COUNTER(&htim2);
  
  logBuffer[logIndex].name = stepName;
  logBuffer[logIndex].timestamp = timer_cnt_val;
  logBuffer[logIndex].divenr = RCC->DIVENR;
  logBuffer[logIndex].miscenr = RCC->MISCENR;
  logBuffer[logIndex].memenr = RCC->MEMENR;
  logBuffer[logIndex].ahb1enr = RCC->AHB1ENR;
  logBuffer[logIndex].ahb2enr = RCC->AHB2ENR;
  logBuffer[logIndex].ahb3enr = RCC->AHB3ENR;
  logBuffer[logIndex].ahb4enr = RCC->AHB4ENR;
  logBuffer[logIndex].ahb5enr = RCC->AHB5ENR;
  logBuffer[logIndex].apb1lenr = RCC->APB1ENR1;
  logBuffer[logIndex].apb1henr = RCC->APB1ENR2;
  logBuffer[logIndex].apb2enr = RCC->APB2ENR;
  logBuffer[logIndex].apb3enr = RCC->APB3ENR;
  logBuffer[logIndex].apb4lenr = RCC->APB4ENR1;
  logBuffer[logIndex].apb4henr = RCC->APB4ENR2;
  logBuffer[logIndex].apb5enr = RCC->APB5ENR;
  logIndex++;
}

/**
  * @brief Function to send the logged timestamps over UART
  * @retval None
  */
void pwr_timestamp_sendOverUart(void)
{
  /* Stop timer */
  pwr_timestamp_stop();
  tim_started = 0;
  
  for (int i = 0; i < logIndex; i++) 
  {
      printf(LOG_FORMAT "\n", logBuffer[i].name, logBuffer[i].timestamp,
             logBuffer[i].divenr, logBuffer[i].miscenr, logBuffer[i].memenr,
             logBuffer[i].ahb1enr, logBuffer[i].ahb2enr, logBuffer[i].ahb3enr,
             logBuffer[i].ahb4enr, logBuffer[i].ahb5enr, logBuffer[i].apb1lenr,
             logBuffer[i].apb1henr, logBuffer[i].apb2enr, logBuffer[i].apb3enr,
             logBuffer[i].apb4lenr, logBuffer[i].apb4henr, logBuffer[i].apb5enr);
  }
  /* send end of log command */
  printf("%s\r\n", END_OF_LOG);
  
  logIndex = 0;
}
