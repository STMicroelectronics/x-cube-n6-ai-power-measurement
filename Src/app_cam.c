 /**
 ******************************************************************************
 * @file    app_cam.c
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

#include <assert.h>
#include "cmw_camera.h"
#include "app_cam.h"
#include "app_config.h"

#if defined(USE_IMX335_SENSOR)
  #define GAMMA_CONVERSION 0
#elif defined(USE_VD66GY_SENSOR)
  #define GAMMA_CONVERSION 0
#elif defined(USE_VD55G1_SENSOR)
  #define GAMMA_CONVERSION 0
#endif

extern int32_t cameraFrameReceived;

static void DCMIPP_PipeInitDisplay(CMW_CameraInit_t *camConf)
{
  CMW_Aspect_Ratio_Mode_t aspect_ratio;
  CMW_DCMIPP_Conf_t dcmipp_conf;
  int ret;

  if (ASPECT_RATIO_MODE == ASPECT_RATIO_CROP)
  {
    aspect_ratio = CMW_Aspect_ratio_crop;
  }
  else if (ASPECT_RATIO_MODE == ASPECT_RATIO_FIT)
  {
    aspect_ratio = CMW_Aspect_ratio_fit;
  }
  else if (ASPECT_RATIO_MODE == ASPECT_RATIO_FULLSCREEN)
  {
    aspect_ratio = CMW_Aspect_ratio_fullscreen;
  }

  int lcd_bg_width;
  int lcd_bg_height;

  if (camConf->height <= 480)
  {
    lcd_bg_height = camConf->height;
  }
  else
  {
    lcd_bg_height = 480;
  }

#if ASPECT_RATIO_MODE == ASPECT_RATIO_FULLSCREEN
  lcd_bg_width = (((camConf->width*lcd_bg_height)/camConf->height) - ((camConf->width*lcd_bg_height)/camConf->height) % 16);
#else
  if (camConf->height <= 480)
  {
    lcd_bg_width = camConf->height;
  }
  else
  {
    lcd_bg_width = 480;
  }
#endif

  dcmipp_conf.output_width = lcd_bg_width;
  dcmipp_conf.output_height = lcd_bg_height;
  dcmipp_conf.output_format = CAPTURE_FORMAT;
  dcmipp_conf.output_bpp = CAPTURE_BPP;
  dcmipp_conf.mode = aspect_ratio;
  dcmipp_conf.enable_swap = 0;
  dcmipp_conf.enable_gamma_conversion = GAMMA_CONVERSION;
  uint32_t pitch;
  ret = CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &pitch);
  assert(ret == HAL_OK);
  if (dcmipp_conf.output_width*dcmipp_conf.output_bpp != pitch)
  {
    assert(1);
  }
}

static void DCMIPP_PipeInitNn(void)
{
  CMW_Aspect_Ratio_Mode_t aspect_ratio;
  CMW_DCMIPP_Conf_t dcmipp_conf;
  int ret;

  if (ASPECT_RATIO_MODE == ASPECT_RATIO_CROP)
  {
    aspect_ratio = CMW_Aspect_ratio_crop;
  }
  else if (ASPECT_RATIO_MODE == ASPECT_RATIO_FIT)
  {
    aspect_ratio = CMW_Aspect_ratio_fit;
  }
  else if (ASPECT_RATIO_MODE == ASPECT_RATIO_FULLSCREEN)
  {
    aspect_ratio = CMW_Aspect_ratio_fit;
  }

  dcmipp_conf.output_width = NN_WIDTH,
  dcmipp_conf.output_height = NN_HEIGHT,
  dcmipp_conf.output_format = NN_FORMAT,
  dcmipp_conf.output_bpp = NN_BPP,
  dcmipp_conf.mode = aspect_ratio;
  dcmipp_conf.enable_swap = 1;
  dcmipp_conf.enable_gamma_conversion = GAMMA_CONVERSION;
  uint32_t pitch;
  ret = CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE2, &dcmipp_conf, &pitch);
  assert(ret == HAL_OK);
  if (dcmipp_conf.output_width*NN_BPP != pitch)
  {
    //@Todo implement crop software
  }
}

void CAM_Init(void)
{
  int ret;
  CMW_CameraInit_t cam_conf;

  cam_conf.width = CAMERA_WIDTH;
  cam_conf.height = CAMERA_HEIGHT;
  cam_conf.fps = CAMERA_FPS;
  cam_conf.pixel_format = 0; /* Default; Not implemented yet */
  cam_conf.anti_flicker = 0;
  cam_conf.mirror_flip = CAMERA_FLIP;

  ret = CMW_CAMERA_Init(&cam_conf);
  assert(ret == CMW_ERROR_NONE);
  DCMIPP_PipeInitDisplay(&cam_conf);
  DCMIPP_PipeInitNn();
}

void CAM_DeInit(void)
{
  int ret;
  ret = CMW_CAMERA_DeInit();
  assert(ret == CMW_ERROR_NONE);
}

void CAM_DisplayPipe_Start(uint8_t *display_pipe_dst, uint32_t cam_mode)
{
  int ret;
  ret = CMW_CAMERA_Start(DCMIPP_PIPE1, display_pipe_dst, cam_mode);
  assert(ret == CMW_ERROR_NONE);
}

void CAM_NNPipe_Start(uint8_t *nn_pipe_dst, uint32_t cam_mode)
{
  int ret;

  ret = CMW_CAMERA_Start(DCMIPP_PIPE2, nn_pipe_dst, cam_mode);
  assert(ret == CMW_ERROR_NONE);
}

void CAM_DisplayPipe_Stop()
{
  int ret;
  ret = CMW_CAMERA_Suspend(DCMIPP_PIPE1);
  assert(ret == CMW_ERROR_NONE);
}

void CAM_IspUpdate(void)
{
  int ret = CMW_ERROR_NONE;
  ret = CMW_CAMERA_Run();
  assert(ret == CMW_ERROR_NONE);
}

/**
  * @brief  Frame event callback
  * @param  hdcmipp pointer to the DCMIPP handle
  * @retval None
  */
int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe)
{
  switch (pipe)
  {
    case DCMIPP_PIPE2 :
      cameraFrameReceived++;
      break;
  }
  return 0;
}
