/**
  ******************************************************************************
  * @file    stm32n6570_discovery_ts.c
  * @author  MCD Application Team
  * @brief   This file provides a set of firmware functions to communicate
  *          with  external devices available on STM32N6570-DK board (MB1939) from
  *          STMicroelectronics
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

/* File Info : -----------------------------------------------------------------
                                   User NOTES
1. How To use this driver:
--------------------------
   - This driver is used to drive the GT911 touch screen module
     mounted over TFT-LCD on the STM32N6570_DK board.

2. Driver description:
---------------------
  + Initialization steps:
     o Initialize the TS module using the BSP_TS_Init() function. This
       function includes the MSP layer hardware resources initialization and the
       communication layer configuration to start the TS use. The LCD size properties
       (x and y) are passed as parameters.
     o If TS interrupt mode is desired, you must configure the TS interrupt mode
       by calling the function BSP_TS_ITConfig(). The TS interrupt mode is generated
       as an external interrupt whenever a touch is detected.
       The interrupt mode internally uses the IO functionalities driver driven by
       the IO expander, to configure the IT line.

  + Touch screen use
     o The touch screen state is captured whenever the function BSP_TS_GetState() is
       used. This function returns information about the last LCD touch occurred
       in the TS_State_t structure.
     o The IT is handled using the corresponding external interrupt IRQ handler,
       the user IT callback treatment is implemented on the same external interrupt
       callback.

------------------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "stm32n6570_discovery_ts.h"
#include "stm32n6570_discovery_bus.h"
#include "../Components/Common/ts.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32N6570_DK
  * @{
  */

/** @defgroup STM32N6570_DK_TS TS
  * @{
  */

/** @defgroup STM32N6570_DK_TS_Private_Types_Definitions Private Types Definitions
  * @{
  */
/**
  * @}
  */

/** @defgroup STM32N6570_DK_TS_Private_Defines Private Types Defines
  * @{
  */
/**
  * @}
  */

/** @defgroup STM32N6570_DK_TS_Private_Macros Private Macros
  * @{
  */
#define TS_MIN(a,b) ((a > b) ? b : a)
/**
  * @}
  */
/** @defgroup STM32N6570_DK_TS_Private_Function_Prototypes Private Function Prototypes
  * @{
  */
static int32_t GT911_Probe(uint32_t Instance);
static void TS_EXTI_Callback(void);
/**
  * @}
  */

/** @defgroup STM32N6570_DK_TS_Privates_Variables Privates Variables
  * @{
  */
static TS_Drv_t   *Ts_Drv = NULL;
/**
  * @}
  */

/** @defgroup STM32N6570_DK_TS_Exported_Variables Exported Variables
  * @{
  */
EXTI_HandleTypeDef hts_exti[TS_INSTANCES_NBR] = {0};
void     *Ts_CompObj[TS_INSTANCES_NBR] = {0};
TS_Ctx_t  Ts_Ctx[TS_INSTANCES_NBR]     = {0};
/**
  * @}
  */

/** @defgroup STM32N6570_DK_TS_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief  Initializes and configures the touch screen functionalities and
  *         configures all necessary hardware resources (GPIOs, I2C, clocks..).
  * @param  Instance TS instance. Could be only 0.
  * @param  TS_Init  TS Init structure
  * @retval BSP status
  */
int32_t BSP_TS_Init(uint32_t Instance, TS_Init_t *TS_Init)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure = {0};

  if((Instance >= TS_INSTANCES_NBR) || (TS_Init->Width == 0U) ||( TS_Init->Width > TS_MAX_WIDTH) ||\
                         (TS_Init->Height == 0U) ||( TS_Init->Height > TS_MAX_HEIGHT) ||\
                         (TS_Init->Accuracy > TS_MIN((TS_Init->Width), (TS_Init->Height))))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Initialize NRST pin */
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Pin  = TS_NRST_PIN;
    HAL_GPIO_Init(TS_NRST_GPIO_PORT, &gpio_init_structure);

    /* Disable GT911 reset */
    HAL_GPIO_WritePin(TS_NRST_GPIO_PORT, TS_NRST_PIN, GPIO_PIN_SET);

    if(GT911_Probe(Instance) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_NO_INIT;
    }
    else
    {
      TS_Capabilities_t Capabilities;
      uint32_t i;
      /* Store parameters on TS context */
      Ts_Ctx[Instance].Width       = TS_Init->Width;
      Ts_Ctx[Instance].Height      = TS_Init->Height;
      Ts_Ctx[Instance].Orientation = TS_Init->Orientation;
      Ts_Ctx[Instance].Accuracy    = TS_Init->Accuracy;
      /* Get capabilities to retrieve maximum values of X and Y */
      if (Ts_Drv->GetCapabilities(Ts_CompObj[Instance], &Capabilities) < 0)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        /* Store maximum X and Y on context */
        Ts_Ctx[Instance].MaxX = Capabilities.MaxXl;
        Ts_Ctx[Instance].MaxY = Capabilities.MaxYl;
        /* Initialize previous position in order to always detect first touch */
        for(i = 0; i < TS_TOUCH_NBR; i++)
        {
          Ts_Ctx[Instance].PreviousX[i] = TS_Init->Width + TS_Init->Accuracy + 1U;
          Ts_Ctx[Instance].PreviousY[i] = TS_Init->Height + TS_Init->Accuracy + 1U;
        }
      }
    }
  }

  return ret;
}

/**
  * @brief  De-Initializes the touch screen functionalities
  * @param  Instance TS instance. Could be only 0.
  * @retval BSP status
  */
int32_t BSP_TS_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Ts_Drv->DeInit(Ts_CompObj[Instance]) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  /* Reset GT911 */
  HAL_GPIO_WritePin(TS_NRST_GPIO_PORT, TS_NRST_PIN, GPIO_PIN_RESET);

  /* Reset pin must be driven low for at least 100us for a proper reset */
  HAL_Delay(1U);

  /* DeInit reset GPIO */
  HAL_GPIO_DeInit(TS_NRST_GPIO_PORT, TS_NRST_PIN);

  return ret;
}

/**
  * @brief  Get Touch Screen instance capabilities
  * @param  Instance Touch Screen instance
  * @param  Capabilities pointer to Touch Screen capabilities
  * @retval BSP status
  */
int32_t BSP_TS_GetCapabilities(uint32_t Instance, TS_Capabilities_t *Capabilities)
{
  int32_t ret = BSP_ERROR_NONE;

  if((Instance >= TS_INSTANCES_NBR) || (Capabilities == NULL))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    (void)Ts_Drv->GetCapabilities(Ts_CompObj[Instance], Capabilities);
  }

  return ret;
}

/**
  * @brief  Configures and enables the touch screen interrupts.
  * @param  Instance TS instance. Could be only 0.
  * @retval BSP status
  */
int32_t BSP_TS_EnableIT(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure = {0};

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Configure Interrupt mode for TS_INT pin falling edge : when a new touch is available */
    /* TS_INT pin is active on low level on new touch available */
    TS_INT_GPIO_CLK_ENABLE();
    gpio_init_structure.Pin   = TS_INT_PIN;
    gpio_init_structure.Pull  = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init_structure.Mode  = GPIO_MODE_IT_RISING;

    HAL_GPIO_Init(TS_INT_GPIO_PORT, &gpio_init_structure);

    if(Ts_Drv->EnableIT(Ts_CompObj[Instance]) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      if(HAL_EXTI_GetHandle(&hts_exti[Instance], TS_EXTI_LINE) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else if(HAL_EXTI_RegisterCallback(&hts_exti[Instance],  HAL_EXTI_COMMON_CB_ID, TS_EXTI_Callback) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else
      {
        /* Enable and set the TS_INT EXTI Interrupt to an intermediate priority */
        HAL_NVIC_SetPriority((IRQn_Type)(TS_INT_EXTI_IRQn), BSP_TS_IT_PRIORITY, 0x00);
        HAL_NVIC_EnableIRQ((IRQn_Type)(TS_INT_EXTI_IRQn));
      }
    }
  }

  return ret;
}

/**
  * @brief  Disables the touch screen interrupts.
  * @param  Instance TS instance. Could be only 0.
  * @retval BSP status
  */
int32_t BSP_TS_DisableIT(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* De-init TS_INT_PIN     */
    gpio_init_structure.Pin  = TS_INT_PIN;
    HAL_GPIO_DeInit(TS_INT_GPIO_PORT, gpio_init_structure.Pin);

    /* Disable the TS in interrupt mode */
    /* In that case the INT output of GT911 when new touch is available */
    /* is active on low level and directed on EXTI */
    if(Ts_Drv->DisableIT(Ts_CompObj[Instance]) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  This function handles TS interrupt request.
  * @param  Instance TS instance
  * @retval None
  */
void BSP_TS_IRQHandler(uint32_t Instance)
{
  HAL_EXTI_IRQHandler(&hts_exti[Instance]);
}

/**
  * @brief  BSP TS Callback.
  * @param  Instance : TS instance
  * @retval None.
  */
__weak void BSP_TS_Callback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* This function should be implemented by the user application.
     It is called into this driver when an event on TS touch detection */
}

/**
  * @brief  Returns positions of a single touch screen.
  * @param  Instance  TS instance. Could be only 0.
  * @param  TS_State  Pointer to touch screen current state structure
  * @retval BSP status
  */
int32_t BSP_TS_GetState(uint32_t Instance, TS_State_t *TS_State)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t x_oriented;
  uint32_t y_oriented;
  uint32_t x_diff;
  uint32_t y_diff;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    GT911_State_t state;

    /* Get each touch coordinates */
    if(Ts_Drv->GetState(Ts_CompObj[Instance], &state) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Check and update the number of touches active detected */
    else if(state.TouchDetected != 0U)
    {
      x_oriented = state.TouchX;
      y_oriented = state.TouchY;

      if((Ts_Ctx[Instance].Orientation & TS_SWAP_XY) == TS_SWAP_XY)
      {
        x_oriented = state.TouchY;
        y_oriented = state.TouchX;
      }

      if((Ts_Ctx[Instance].Orientation & TS_SWAP_X) == TS_SWAP_X)
      {
        x_oriented = Ts_Ctx[Instance].MaxX - state.TouchX - 1UL;
      }

      if((Ts_Ctx[Instance].Orientation & TS_SWAP_Y) == TS_SWAP_Y)
      {
        y_oriented = Ts_Ctx[Instance].MaxY - state.TouchY;
      }

      /* Apply boundary */
      TS_State->TouchX = (x_oriented * Ts_Ctx[Instance].Width) / Ts_Ctx[Instance].MaxX;
      TS_State->TouchY = (y_oriented * Ts_Ctx[Instance].Height) / Ts_Ctx[Instance].MaxY;
      /* Store Current TS state */
      TS_State->TouchDetected = state.TouchDetected;

      /* Check accuracy */
      x_diff = (TS_State->TouchX > Ts_Ctx[Instance].PreviousX[0])?
        (TS_State->TouchX - Ts_Ctx[Instance].PreviousX[0]):
        (Ts_Ctx[Instance].PreviousX[0] - TS_State->TouchX);

      y_diff = (TS_State->TouchY > Ts_Ctx[Instance].PreviousY[0])?
        (TS_State->TouchY - Ts_Ctx[Instance].PreviousY[0]):
        (Ts_Ctx[Instance].PreviousY[0] - TS_State->TouchY);


      if ((x_diff > Ts_Ctx[Instance].Accuracy) || (y_diff > Ts_Ctx[Instance].Accuracy))
      {
        /* New touch detected */
        Ts_Ctx[Instance].PreviousX[0] = TS_State->TouchX;
        Ts_Ctx[Instance].PreviousY[0] = TS_State->TouchY;
      }
     else
     {
        TS_State->TouchX = Ts_Ctx[Instance].PreviousX[0];
        TS_State->TouchY = Ts_Ctx[Instance].PreviousY[0];
      }
    }
    else
    {
      TS_State->TouchDetected = 0U;
      TS_State->TouchX = Ts_Ctx[Instance].PreviousX[0];
      TS_State->TouchY = Ts_Ctx[Instance].PreviousY[0];
    }
  }

  return ret;
}

#if (USE_TS_MULTI_TOUCH > 0)
/**
  * @brief  Returns positions of multi touch screen.
  * @param  Instance  TS instance. Could be only 0.
  * @param  TS_State  Pointer to touch screen current state structure
  * @retval BSP status
  */
int32_t BSP_TS_Get_MultiTouchState(uint32_t Instance, TS_MultiTouch_State_t *TS_State)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t index;
  uint32_t x_oriented;
  uint32_t y_oriented;
  uint32_t x_diff;
  uint32_t y_diff;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    GT911_MultiTouch_State_t state;

    /* Get each touch coordinates */
    if(Ts_Drv->GetMultiTouchState(Ts_CompObj[Instance], &state) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(state.TouchDetected != 0U)
    {
      /* Check and update the number of touches active detected */
      for(index = 0; index < state.TouchDetected; index++)
      {
        x_oriented = state.TouchX[index];
        y_oriented = state.TouchY[index];

        if((Ts_Ctx[Instance].Orientation & TS_SWAP_XY) == TS_SWAP_XY)
        {
          x_oriented = state.TouchY[index];
          y_oriented = state.TouchX[index];
        }

        if((Ts_Ctx[Instance].Orientation & TS_SWAP_X) == TS_SWAP_X)
        {
          x_oriented = Ts_Ctx[Instance].MaxX - state.TouchX[index] - 1UL;
        }

        if((Ts_Ctx[Instance].Orientation & TS_SWAP_Y) == TS_SWAP_Y)
        {
          y_oriented = Ts_Ctx[Instance].MaxY - state.TouchY[index];
        }

        /* Apply boundary */
        TS_State->TouchX[index] = (x_oriented * Ts_Ctx[Instance].Width) / Ts_Ctx[Instance].MaxX;
        TS_State->TouchY[index] = (y_oriented * Ts_Ctx[Instance].Height) / Ts_Ctx[Instance].MaxY;
        /* Store Current TS state */
        TS_State->TouchDetected = state.TouchDetected;

        /* Check accuracy */
        x_diff = (TS_State->TouchX[index] > Ts_Ctx[Instance].PreviousX[index])?
                 (TS_State->TouchX[index] - Ts_Ctx[Instance].PreviousX[index]):
                 (Ts_Ctx[Instance].PreviousX[index] - TS_State->TouchX[index]);

        y_diff = (TS_State->TouchY[index] > Ts_Ctx[Instance].PreviousY[index])?
                 (TS_State->TouchY[index] - Ts_Ctx[Instance].PreviousY[index]):
                 (Ts_Ctx[Instance].PreviousY[index] - TS_State->TouchY[index]);

        if ((x_diff > Ts_Ctx[Instance].Accuracy) || (y_diff > Ts_Ctx[Instance].Accuracy))
        {
          /* New touch detected */
          Ts_Ctx[Instance].PreviousX[index] = TS_State->TouchX[index];
          Ts_Ctx[Instance].PreviousY[index] = TS_State->TouchY[index];
        }
        else
        {
          TS_State->TouchX[index] = Ts_Ctx[Instance].PreviousX[index];
          TS_State->TouchY[index] = Ts_Ctx[Instance].PreviousY[index];
        }
      }
    }
    else
    {
      TS_State->TouchDetected = 0U;
      for(index = 0; index < TS_TOUCH_NBR; index++)
      {
        TS_State->TouchX[index] = Ts_Ctx[Instance].PreviousX[index];
        TS_State->TouchY[index] = Ts_Ctx[Instance].PreviousY[index];
      }
    }
  }

  return ret;
}
#endif /* USE_TS_MULTI_TOUCH == 1 */

#if (USE_TS_GESTURE == 1)
/**
  * @brief  Update gesture Id following a touch detected.
  * @param  Instance      TS instance. Could be only 0.
  * @param  GestureConfig Pointer to gesture configuration structure
  * @retval BSP status
  */
int32_t BSP_TS_GestureConfig(uint32_t Instance, TS_Gesture_Config_t *GestureConfig)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Ts_Drv->GestureConfig(Ts_CompObj[Instance], GestureConfig) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Update gesture Id following a touch detected.
  * @param  Instance   TS instance. Could be only 0.
  * @param  GestureId  Pointer to gesture ID
  * @retval BSP status
  */
int32_t BSP_TS_GetGestureId(uint32_t Instance, uint32_t *GestureId)
{
  int32_t ret = BSP_ERROR_NONE;
  uint8_t tmp = 0;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Get gesture Id */
  else if(Ts_Drv->GetGesture(Ts_CompObj[Instance], &tmp)  < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Remap gesture Id to a TS_Gesture_Id_t value */
    switch(tmp)
    {
    case GT911_GEST_ID_NO_GESTURE :
      *GestureId = GESTURE_ID_NO_GESTURE;
      break;
    case GT911_GEST_ID_SWIPE_UP :
      *GestureId = GESTURE_ID_SWIPE_UP;
      break;
    case GT911_GEST_ID_SWIPE_RIGHT :
      *GestureId = GESTURE_ID_SWIPE_RIGHT;
      break;
    case GT911_GEST_ID_SWIPE_DOWN :
      *GestureId = GESTURE_ID_SWIPE_DOWN;
      break;
    case GT911_GEST_ID_SWIPE_LEFT :
      *GestureId = GESTURE_ID_SWIPE_LEFT;
      break;
    case GT911_GEST_ID_DOUBLE_TAP :
      *GestureId = GESTURE_ID_DOUBLE_TAP;
      break;
    default :
      *GestureId = GESTURE_ID_NO_GESTURE;
      break;
    }
  }

  return ret;
}
#endif /* USE_TS_GESTURE == 1 */

/**
  * @brief  Set TS orientation
  * @param  Instance TS instance. Could be only 0.
  * @param  Orientation Orientation to be set
  * @retval BSP status
  */
int32_t BSP_TS_Set_Orientation(uint32_t Instance, uint32_t Orientation)
{
  Ts_Ctx[Instance].Orientation = Orientation;
  return BSP_ERROR_NONE;
}

/**
  * @brief  Get TS orientation
  * @param  Instance TS instance. Could be only 0.
  * @param  Orientation Current Orientation to be returned
  * @retval BSP status
  */
int32_t BSP_TS_Get_Orientation(uint32_t Instance, uint32_t *Orientation)
{
  *Orientation = Ts_Ctx[Instance].Orientation;
  return BSP_ERROR_NONE;
}

/**
  * @}
  */
/** @defgroup STM32N6570_DK_TS_Private_Functions Private Functions
  * @{
  */

/**
  * @brief  Register Bus IOs if component ID is OK
  * @param  Instance TS instance. Could be only 0.
  * @retval BSP status
  */
static int32_t GT911_Probe(uint32_t Instance)
{
  int32_t ret              = BSP_ERROR_NONE;
  GT911_IO_t              IOCtx;
  static GT911_Object_t   GT911Obj;
  uint32_t gt911_id       = 0;

  /* Configure the touch screen driver */
  IOCtx.Address     = TS_I2C_ADDRESS;
  IOCtx.Init        = BSP_I2C2_Init;
  IOCtx.DeInit      = BSP_I2C2_DeInit;
  IOCtx.ReadReg     = BSP_I2C2_ReadReg16;
  IOCtx.WriteReg    = BSP_I2C2_WriteReg16;
  IOCtx.GetTick     = BSP_GetTick;

  if(GT911_RegisterBusIO (&GT911Obj, &IOCtx) != GT911_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }
  else if(GT911_ReadID(&GT911Obj, &gt911_id) != GT911_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else if(gt911_id != GT911_ID)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    Ts_CompObj[Instance] = &GT911Obj;
    Ts_Drv = (TS_Drv_t *) &GT911_TS_Driver;

    if(Ts_Drv->Init(Ts_CompObj[Instance]) != GT911_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  TS EXTI touch detection callbacks.
  * @retval None
  */
static void TS_EXTI_Callback(void)
{
  BSP_TS_Callback(0);

  /* Clear interrupt on TS driver */
  (void)Ts_Drv->ClearIT(Ts_CompObj[0]);
}

/**
  * @}
  */


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
