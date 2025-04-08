 /**
 ******************************************************************************
 * @file    main.c
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

#include "cmw_camera.h"
#include "stm32n6570_discovery_bus.h"
#include "stm32n6570_discovery_lcd.h"
#include "stm32n6570_discovery_xspi.h"
#include "stm32n6570_discovery.h"
#include "stm32_lcd.h"
#include "app_fuseprogramming.h"
#include "stm32_lcd_ex.h"
#include "app_postprocess.h"
#include "ll_aton_runtime.h"
#include "app_cam.h"
#include "main.h"
#include "stm32n6xx_hal_rif.h"
#include "app_config.h"
#include "pwr_timestamp.h"
#include "system_clock.h"


/* clock configuration during inference when NPU_FRQ_SCALING enabled */
const FrequencyStep frequencySteps[] =
{
  { /* overdrive, npu@1GHz, cpu@800Mhz */
  	.pll2Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 125, .PLLM = 6, .PLLP1 = 1, .PLLP2 = 1, .PLLFractional = 0},
  	.pll3Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 75, .PLLM = 4, .PLLP1 = 1, .PLLP2 = 1, .PLLFractional = 0},
  	.npufreq = 1000,
  	.cpuClkSrc = RCC_ICCLKSOURCE_PLL1,
  	.npuClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.npuRamsClkSrc = RCC_ICCLKSOURCE_PLL3,
  	.stepName = "nn_inference_1GHz"
  },
  { /* nominal, npu@800Mhz, cpu@600Mhz */
  	.pll3Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 2, .PLLP1 = 2, .PLLP2 = 1, .PLLFractional = 0},
  	.pll2Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 3, .PLLP1 = 1, .PLLP2 = 1, .PLLFractional = 0},
  	.npufreq = 800,
  	.cpuClkSrc = RCC_ICCLKSOURCE_PLL3,
  	.npuClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.npuRamsClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.stepName = "nn_inference_800MHz"
  },
  { /* nominal, npu@600Mhz, cpu@600Mhz */
  	.pll3Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 2, .PLLP1 = 2, .PLLP2 = 1, .PLLFractional = 0},
  	.pll2Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 2, .PLLP1 = 2, .PLLP2 = 1, .PLLFractional = 0},
  	.npufreq = 600,
  	.cpuClkSrc = RCC_ICCLKSOURCE_PLL3,
  	.npuClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.npuRamsClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.stepName = "nn_inference_600MHz"
  },
  { /* nominal, npu@400Mhz, cpu@600Mhz */
  	.pll3Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 2, .PLLP1 = 2, .PLLP2 = 1, .PLLFractional = 0},
  	.pll2Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 3, .PLLP1 = 2, .PLLP2 = 1, .PLLFractional = 0},
  	.npufreq = 400,
  	.cpuClkSrc = RCC_ICCLKSOURCE_PLL3,
  	.npuClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.npuRamsClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.stepName = "nn_inference_400MHz"
  },
  { /* nominal, npu@200Mhz, cpu@600Mhz */
  	.pll3Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 2, .PLLP1 = 2, .PLLP2 = 1, .PLLFractional = 0},
  	.pll2Cfg = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 3, .PLLP1 = 4, .PLLP2 = 1, .PLLFractional = 0},
  	.npufreq = 200,
  	.cpuClkSrc = RCC_ICCLKSOURCE_PLL3,
  	.npuClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.npuRamsClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.stepName = "nn_inference_200MHz"
  },
  {  /* nominal, npu@100Mhz, cpu@600Mhz */
  	.pll3Cfg= {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 2, .PLLP1 = 2, .PLLP2 = 1, .PLLFractional = 0},
  	.pll2Cfg= {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE, .PLLN = 50, .PLLM = 3, .PLLP1 = 4, .PLLP2 = 2, .PLLFractional = 0},
  	.npufreq = 100,
  	.cpuClkSrc = RCC_ICCLKSOURCE_PLL3,
  	.npuClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.npuRamsClkSrc = RCC_ICCLKSOURCE_PLL2,
  	.stepName = "nn_inference_100MHz"
  }
};

CLASSES_TABLE;

#define MAX_NUMBER_OUTPUT 5

#if POSTPROCESS_TYPE == POSTPROCESS_OD_YOLO_V2_UF
 yolov2_pp_static_param_t pp_params;
#elif POSTPROCESS_TYPE == POSTPROCESS_OD_YOLO_V5_UU
 yolov5_pp_static_param_t pp_params;
#elif POSTPROCESS_TYPE == POSTPROCESS_OD_YOLO_V8_UF
 yolov8_pp_static_param_t pp_params;
#elif POSTPROCESS_TYPE == POSTPROCESS_OD_YOLO_V8_UI
yolov8_pp_static_param_t pp_params;
#else
  #error "PostProcessing type not supported"
#endif

#define STLINKPWR_TGI_PORT  GPIOG
#define STLINKPWR_TGI_PORT_CLK_ENABLE()  __HAL_RCC_GPIOG_CLK_ENABLE()
#define STLINKPWR_TGI_PORT_CLK_SLEEP_ENABLE() __HAL_RCC_GPIOG_CLK_SLEEP_ENABLE()
#define STLINKPWR_TGI_PIN   GPIO_PIN_5

__attribute__ ((aligned (32)))
uint8_t nn_in_buffer[NN_WIDTH*NN_HEIGHT*NN_BPP];

volatile int32_t cameraFrameReceived;

const LL_Buffer_InfoTypeDef *nn_in_info;
const LL_Buffer_InfoTypeDef *nn_out_info;
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Default);
int number_output = 0;

od_pp_out_t pp_output;
CACHEAXI_HandleTypeDef hcacheaxi;
UART_HandleTypeDef huart1;

float32_t *nn_out[MAX_NUMBER_OUTPUT];
int32_t nn_out_len[MAX_NUMBER_OUTPUT];

static void NPURam_enable(void);
static void NPURam_disable(void);
static void NPUCache_enable(void);
static void NPUCache_disable(void);
static void Security_Config(void);
static void IAC_Config(void);
static void GPIO_Config(void);
static void Console_Config(void);
static void waitForUserTrigger(void);
static void cameraInit(void);
static void startStlinkPwr(void);
static void cameraCapture(void);
static void cameraDeInit(void);
static void nn_inference(void);
static void postProcessing(void);
static void sendTimestamp(void);
static void deInitIPs(void);

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* enable VDDA18ADC isolation */
  PWR->SVMCR3 |= PWR_SVMCR3_ASV;
  PWR->SVMCR3 |= PWR_SVMCR3_AVMEN;
  /* Power on ICACHE */
  MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_ICACTIVE_Msk;
  NPURam_disable();

  /* disable unused IPs  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_AHBSRAM1_MEM_CLK_DISABLE();
  __HAL_RCC_AHBSRAM2_MEM_CLK_DISABLE();
  __HAL_RCC_BKPSRAM_MEM_CLK_DISABLE();
  __HAL_RCC_RTCAPB_CLK_DISABLE();
  __HAL_RCC_RTC_CLK_DISABLE();
  __HAL_RCC_RNG_CLK_DISABLE();
  HAL_PWR_DisableBkUpAccess();

  /* Set back system and CPU clock source to HSI */
  __HAL_RCC_CPUCLK_CONFIG(RCC_CPUCLKSOURCE_HSI);
  __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);

  GPIO_Config();

  HAL_Init();

  SCB_EnableICache();

#if defined(USE_DCACHE)
  /* Power on DCACHE */
  MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_DCACTIVE_Msk;
  SCB_EnableDCache();
#endif

  sysclk_SystemClockConfig();

  Fuse_Programming();

  /* Set all required IPs as secure privileged */
  Security_Config();
  IAC_Config();

  nn_out_info = LL_ATON_Output_Buffers_Info_Default();

  /* Count number of outputs */
  while (nn_out_info[number_output].name != NULL)
  {
    number_output++;
  }
  assert(number_output <= MAX_NUMBER_OUTPUT);

  for (int i = 0; i < number_output; i++)
  {
    nn_out[i] = (float32_t *) LL_Buffer_addr_start(&nn_out_info[i]);
    nn_out_len[i] = LL_Buffer_len(&nn_out_info[i]);
  }

  app_postprocess_init(&pp_params);

  /*** App Loop ***************************************************************/
  while (1)
  {
    /* Wait for USER1 trigger */
    waitForUserTrigger();
    
    /* Start STLINKPWR */
    startStlinkPwr();

    /* Camera initialization */
    cameraInit();

    /* Camera capture */
    cameraCapture();
    
    /* Camera de-initialization */
    cameraDeInit();
    
    /* Inference */
    nn_inference();
    
    /* Post-processing */
    postProcessing();
    
    /* Send timestamps */
    sendTimestamp();
    
    /* De-initialize IPs */
    deInitIPs();
  }
}

/**
  * @brief  Wait USER1 push button
  * @param  None
  * @retval None
  */
static void waitForUserTrigger(void)
{
  /* reset TGI pin and clear any pending interrupt before going into sleep mode */
  HAL_GPIO_WritePin(STLINKPWR_TGI_PORT, STLINKPWR_TGI_PIN, GPIO_PIN_RESET);
  HAL_NVIC_ClearPendingIRQ(EXTI13_IRQn);
  HAL_NVIC_DisableIRQ(CSI_IRQn);
  /* enter in sleep mode and wait wake-up by USER1 button interrupt */
  HAL_SuspendTick();
  HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();
  pwr_timestamp_init();
}

/**
  * @brief  init camera sensor and camera pipline (Csi and Dcmipp)
  * @param  None
  * @retval None
  */
static void cameraInit(void)
{
  CAM_Init();
  pwr_timestamp_log("CAM init");
}

/**
  * @brief  send a signal to trigger the STLinkPwr capture
  * @param  None
  * @retval None
  */
static void startStlinkPwr(void)
{
  /* trigger power capture */
  HAL_GPIO_WritePin(STLINKPWR_TGI_PORT, STLINKPWR_TGI_PIN, GPIO_PIN_SET);
  pwr_timestamp_start();
  pwr_timestamp_log("start timestamp");
}

/**
  * @brief  trigger camera capture and wait frame reception
  * @param  None
  * @retval None
  */
static void cameraCapture(void)
{
  cameraFrameReceived = 0;

  /* enable low power clocks */
  __HAL_RCC_DCMIPP_CLK_SLEEP_ENABLE();
  __HAL_RCC_CSI_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM1_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM2_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_FLEXRAM_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_TIM2_CLK_SLEEP_ENABLE();
  __HAL_RCC_I2C1_CLK_SLEEP_ENABLE();
  __HAL_RCC_I2C2_CLK_SLEEP_ENABLE();
  
  /* Start NN camera single capture Snapshot */
  CAM_NNPipe_Start(nn_in_buffer, CMW_MODE_SNAPSHOT);
  pwr_timestamp_log("camera started");

  HAL_SuspendTick();
  while (cameraFrameReceived == 0)
  {
	/* sleep during capture */
    HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
  }
  HAL_ResumeTick();
  
  pwr_timestamp_log("wait frame");

  CAM_IspUpdate();
  pwr_timestamp_log("ISP update");
}

/**
  * @brief  Deinit camera sensor and camera pipeline (Csi and Dcmipp)
  * @param  None
  * @retval None
  */
static void cameraDeInit(void)
{
  CAM_DeInit();
  pwr_timestamp_log("camera de-init");
}


/**
  * @brief  configure PSRAM and Flash memory in memory mapped mode
  * @param  None
  * @retval None
  */
void externMem_config(void)
{
  BSP_XSPI_NOR_Init_t NOR_Init;

  /* External RAM */
  #if (USE_PSRAM == 1)
  	BSP_XSPI_RAM_Init(0);
  	BSP_XSPI_RAM_EnableMemoryMappedMode(0);
    __HAL_RCC_XSPI1_CLK_SLEEP_ENABLE();
    pwr_timestamp_log("External RAM init");
  #endif /* USE_PSRAM */

  /* For NN weights */
  NOR_Init.InterfaceMode = BSP_XSPI_NOR_OPI_MODE;
  NOR_Init.TransferRate = BSP_XSPI_NOR_DTR_TRANSFER;
  BSP_XSPI_NOR_Init(0, &NOR_Init);
  BSP_XSPI_NOR_EnableMemoryMappedMode(0);
  __HAL_RCC_XSPI2_CLK_SLEEP_ENABLE();
  pwr_timestamp_log("NOR flash init");
}


#if(NPU_FRQ_SCALING == 1)
/**
  * @brief  configure clocks and run inferences for NPU frequency scaling mode
  * @param  None
  * @retval None
  */
static void runInference_freqScaling(void)
{
  for (int i = 0; i < sizeof(frequencySteps) / sizeof(frequencySteps[0]); i++)
  {
    sysclk_NpuFreqScaling(&frequencySteps[i]);
    /* invalidate all caches before next inferences */
    npu_cache_invalidate();
    SCB_CleanInvalidateDCache();
    SCB_InvalidateICache();
    /* log config timestamp */
    pwr_timestamp_log("config npu clock scaling");

    HAL_SuspendTick();
    LL_ATON_RT_Main(&NN_Instance_Default);
    HAL_ResumeTick();
    pwr_timestamp_log(frequencySteps[i].stepName);
  }
}
#endif /* NPU_FRQ_SCALING */

/**
  * @brief  configures NPU and NPU memories and run inference
  * @param  None
  * @retval None
  */
static void nn_inference(void)
{
  int ret;

  sysclk_NpuClockConfig();
  sysclk_NpuClockEnable();
  sysclk_CpuClockConfig();
#if (CPU_FRQ_SCALE_DOWN == 1)
  sysclk_SetCpuMaxFreq();
#endif

  /* Enable NPU ram and npu cache (axicache) */
  NPURam_enable();
  NPUCache_enable();

  /* config External flash in memory mapped mode,
   * config External PSRAM only if needed
   */
  externMem_config();

  /* use capture buffer as nn_input buffer */
  nn_in_info = LL_ATON_Input_Buffers_Info_Default();
  uint32_t nn_in_len = LL_Buffer_len(&nn_in_info[0]);
  /* Note that we don't need to clean/invalidate those input buffers since they are only access in hardware */
  ret = LL_ATON_Set_User_Input_Buffer_Default(0, nn_in_buffer, nn_in_len);
  assert(ret == LL_ATON_User_IO_NOERROR);
  pwr_timestamp_log("NPU and NPU Rams config");

#if(NPU_FRQ_SCALING == 0)
  /* run NN inference (dry run)*/
  HAL_SuspendTick();
  LL_ATON_RT_Main(&NN_Instance_Default);
  HAL_ResumeTick();
  pwr_timestamp_log("nn inference (dry run)");

  /* run NN inference */
  HAL_SuspendTick();
  LL_ATON_RT_Main(&NN_Instance_Default);
  HAL_ResumeTick();
  pwr_timestamp_log("nn inference");
#else
  /* npu clock scaling, run one inference per npu freq config */
  runInference_freqScaling();
#endif
  sysclk_NpuClockDisable();
  BSP_XSPI_NOR_DeInit(0);
#if (USE_PSRAM == 1)
  BSP_XSPI_RAM_DeInit(0);
#endif /* USE_PSRAM */
  __HAL_RCC_XSPIM_CLK_DISABLE();
}


/**
  * @brief  run post-processing
  * @param  None
  * @retval None
  */
static void postProcessing(void)
{
#if(POWER_OVERDRIVE == 1)
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
  sysclk_NpuOverDrivePllDeinit(&RCC_ClkInitStruct);
#endif

  int32_t error = app_postprocess_run((void **) nn_out, number_output, &pp_output, &pp_params);
  UNUSED(error);
  pwr_timestamp_log("post processing");
  pwr_timestamp_stop();

  /* Discard nn_out region (used by pp_input and pp_outputs variables) to avoid Dcache evictions during nn inference */
  for (int i = 0; i < number_output; i++)
  {
    float32_t *tmp = nn_out[i];
    SCB_InvalidateDCache_by_Addr(tmp, nn_out_len[i]);
  }
#if(POWER_OVERDRIVE == 1)
  sysclk_NpuRamsOverDriveClockDeinit(&RCC_ClkInitStruct);
#endif

#if (NPU_FRQ_SCALING == 1)
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 200;
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 200;
  int ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct);
  assert(ret == HAL_OK);

#if (CPU_FRQ_SCALE_DOWN == 1)
// switch cpu to HSE, switch-off PLL3
  sysclk_SetCpuMinFreq();

  {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_OFF;
    RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_OFF;
    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    assert(ret == HAL_OK);
  }
#else
#endif /* CPU_FRQ_SCALE_DOWN */
 /* switch off pll2, keep cpu@pll3-600MHz */
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_OFF;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  assert(ret == HAL_OK);
#endif /* NPU_FRQ_SCALING */
}

/**
  * @brief  config USART1 and send all timestamps and logged data at end of main loop
  * @param  None
  * @retval None
  */
static void sendTimestamp(void)
{
  Console_Config();
  pwr_timestamp_sendOverUart();
}

/**
  * @brief  disable all used IPs and be ready for next capture
  * @param  None
  * @retval None
  */
static void deInitIPs(void)
{
  HAL_GPIO_WritePin(STLINKPWR_TGI_PORT, STLINKPWR_TGI_PIN, GPIO_PIN_RESET);
  NPURam_disable();
  NPUCache_disable();

#if (CPU_FRQ_SCALE_DOWN != 0)
  sysclk_SetCpuMinFreq();
#endif
  /* deinit Console */
  HAL_UART_DeInit(&huart1);
  HAL_GPIO_DeInit(GPIOE, GPIO_PIN_5 | GPIO_PIN_6);
  __HAL_RCC_USART1_CLK_DISABLE();
  __HAL_RCC_GPIOE_CLK_DISABLE();
}

/**
  * @brief  config and enable NPU RAMS
  * @param  None
  * @retval None
  */
static void NPURam_enable(void)
{
  __HAL_RCC_AXISRAM3_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_SLEEP_ENABLE();
  /* Enable NPU RAMs (4x448KB) */
  __HAL_RCC_AXISRAM3_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_ENABLE();
  __HAL_RCC_RAMCFG_CLK_ENABLE();
  __HAL_RCC_RAMCFG_CLK_SLEEP_ENABLE();
  __HAL_RCC_RAMCFG_FORCE_RESET();
  __HAL_RCC_RAMCFG_RELEASE_RESET();

  RAMCFG_HandleTypeDef hramcfg = {0};
  hramcfg.Instance =  RAMCFG_SRAM3_AXI;
  HAL_RAMCFG_EnableAXISRAM(&hramcfg);
  hramcfg.Instance =  RAMCFG_SRAM4_AXI;
  HAL_RAMCFG_EnableAXISRAM(&hramcfg);
  hramcfg.Instance =  RAMCFG_SRAM5_AXI;
  HAL_RAMCFG_EnableAXISRAM(&hramcfg);
  hramcfg.Instance =  RAMCFG_SRAM6_AXI;
  HAL_RAMCFG_EnableAXISRAM(&hramcfg);
}

/**
  * @brief  disable NPU RAMS
  * @param  None
  * @retval None
  */
static void NPURam_disable(void)
{
  RAMCFG_HandleTypeDef hramcfg = {0};
  hramcfg.Instance =  RAMCFG_SRAM3_AXI;
  HAL_RAMCFG_DisableAXISRAM(&hramcfg);
  hramcfg.Instance =  RAMCFG_SRAM4_AXI;
  HAL_RAMCFG_DisableAXISRAM(&hramcfg);
  hramcfg.Instance =  RAMCFG_SRAM5_AXI;
  HAL_RAMCFG_DisableAXISRAM(&hramcfg);
  hramcfg.Instance =  RAMCFG_SRAM6_AXI;
  HAL_RAMCFG_DisableAXISRAM(&hramcfg);

  __HAL_RCC_NPU_FORCE_RESET();
  __HAL_RCC_NPU_RELEASE_RESET();
  __HAL_RCC_NPU_CLK_DISABLE();
  __HAL_RCC_NPU_CLK_SLEEP_DISABLE();

  /* Disable NPU RAMs (4x448KB) */
  __HAL_RCC_AXISRAM3_MEM_CLK_DISABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_DISABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_DISABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_DISABLE();

  __HAL_RCC_AXISRAM3_MEM_CLK_SLEEP_DISABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_SLEEP_DISABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_SLEEP_DISABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_SLEEP_DISABLE();

  __HAL_RCC_RAMCFG_CLK_DISABLE();
  __HAL_RCC_RAMCFG_CLK_SLEEP_DISABLE();

}


/**
  * @brief  configure axi-cache memory and enable cache mode
  * @param  None
  * @retval None
  */
static void NPUCache_enable(void)
{
  npu_cache_init();
  npu_cache_enable();
}

/**
  * @brief  disable axi-cache memory
  * @param  None
  * @retval None
  */
static void NPUCache_disable(void)
{
  npu_cache_disable();
  npu_cache_deinit();
}


/**
  * @brief  RIF configuration
  * @param  None
  * @retval None
  */
static void Security_Config(void)
{
  __HAL_RCC_RIFSC_CLK_ENABLE();
  RIMC_MasterConfig_t RIMC_master = {0};
  RIMC_master.MasterCID = RIF_CID_1;
  RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_NPU, &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DMA2D, &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DCMIPP, &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC1 , &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC2 , &RIMC_master);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_NPU , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DMA2D , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_CSI    , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DCMIPP , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDC   , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL1 , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL2 , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

/**
  * @brief  enable and reset IAC
  * @param  None
  * @retval None
  */
static void IAC_Config(void)
{
/* Configure IAC to trap illegal access events */
  __HAL_RCC_IAC_CLK_ENABLE();
  __HAL_RCC_IAC_FORCE_RESET();
  __HAL_RCC_IAC_RELEASE_RESET();
}

/**
  * @brief  IAC handler
  * @param  None
  * @retval None
  */
void IAC_IRQHandler(void)
{
  while (1)
  {
  }
}


/**
  * @brief  config USER1 button and GPIO to trigger STLinkPwr capture
  * @param  None
  * @retval None
  */
void GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_SLEEP_ENABLE();

  STLINKPWR_TGI_PORT_CLK_ENABLE();
  STLINKPWR_TGI_PORT_CLK_SLEEP_ENABLE();

  HAL_NVIC_SetPriority(EXTI13_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI13_IRQn);

  /* Configure GPIO pin : BUTTON_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* GPIO for STLINKPWR trigger */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Pin = STLINKPWR_TGI_PIN;
  HAL_GPIO_Init(STLINKPWR_TGI_PORT, &GPIO_InitStruct);
}

/* Define the function for GCC */
#ifdef __GNUC__
int _write(int fd, char * ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, ~0);
  return len;
}
#endif

/* Define the function for IAR */
#ifdef __ICCARM__
int __write(int fd, char * ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, ~0);
  return len;
}
#endif

/**
  * @brief  USART1 config to send data over virtual COM port
  * @param  None
  * @retval None
  */
static void Console_Config(void)
{
  GPIO_InitTypeDef gpio_init;

  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

 /* DISCO & NUCLEO USART1 (PE5/PE6) */
  gpio_init.Mode      = GPIO_MODE_AF_PP;
  gpio_init.Pull      = GPIO_PULLUP;
  gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio_init.Pin       = GPIO_PIN_5 | GPIO_PIN_6;
  gpio_init.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOE, &gpio_init);

  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = 115200;
  huart1.Init.Mode         = UART_MODE_TX_RX;
  huart1.Init.Parity       = UART_PARITY_NONE;
  huart1.Init.WordLength   = UART_WORDLENGTH_8B;
  huart1.Init.StopBits     = UART_STOPBITS_1;
  huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_8;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    while (1);
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  UNUSED(file);
  UNUSED(line);
  __BKPT(0);
  while (1)
  {
  }
}
#endif
