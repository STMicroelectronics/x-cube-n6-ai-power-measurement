 /**
 ******************************************************************************
 * @file    cam_imx335.c
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
#ifdef USE_IMX335_SENSOR
#include "cam.h"
#include <assert.h>

#include "isp_api.h"
#include "mw-camera.h"
#include "app_config.h"
#include "cam_imx335.h"
extern uint32_t ISP_MainPipe_FrameCount;
extern uint32_t ISP_AncillaryPipe_FrameCount;
extern int32_t cameraFrameReceived;

ISP_HandleTypeDef hIsp;
static CAM_Conf_t current_conf;

static int CAM_IMX335_getRes(CAM_Conf_t *p_conf, uint32_t *res)
{
  int width = CAMERA_WIDTH;
  int height = CAMERA_HEIGHT;
  int fps = CAMERA_FPS;

  if ((width == 2592) && (height == 1944) && (fps == 30))
  {
    *res = CAMERA_R2592x1944;
  }
  else
  {
    return -1;
  }
  return 0;
}

static int CAM_GetMirrorFlip(int mirror_settings)
{
  int ret;
  switch (mirror_settings)
  {
    case CAMERA_FLIP_NONE:
      ret =  CAMERA_MIRRORFLIP_NONE;
      break;
    case CAMERA_FLIP_HFLIP:
      ret = CAMERA_MIRRORFLIP_MIRROR;
      break;
    case CAMERA_FLIP_VFLIP:
      ret = CAMERA_MIRRORFLIP_FLIP;
      break;
    case CAMERA_FLIP_HVFLIP:
      ret = CAMERA_MIRRORFLIP_FLIP_MIRROR;
      break;
    default:
        ret =  CAMERA_MIRRORFLIP_NONE;
      break;
  }
  return ret;
}

static void DCMIPP_PipeInitDisplay(DCMIPP_HandleTypeDef *hdcmipp, CAM_Conf_t *p_conf)
{
  DCMIPP_PipeConfTypeDef pipe_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CropConfTypeDef crop_conf = { 0 };
  DCMIPP_DownsizeTypeDef downsize_conf = { 0 };
  DCMIPP_DecimationConfTypeDef decim_conf = { 0 };
  float ratio_width;
  float ratio_height;
  int ret;

  csi_conf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  csi_conf.DataTypeIDA = DCMIPP_DT_RAW10;
  csi_conf.DataTypeIDB = 0;
  ret = HAL_DCMIPP_CSI_PIPE_SetConfig(hdcmipp, DCMIPP_PIPE1, &csi_conf);
  assert(ret == HAL_OK);

  /* Main pipe configuration : RGB565 for preview on LCD */
  pipe_conf.FrameRate = DCMIPP_FRAME_RATE_ALL;
  pipe_conf.PixelPipePitch = (p_conf->display_pipe_width - p_conf->display_pipe_width%16)* p_conf->display_pipe_bpp;
  pipe_conf.PixelPackerFormat = p_conf->display_pipe_format;
  ret = HAL_DCMIPP_PIPE_SetConfig(hdcmipp, DCMIPP_PIPE1, &pipe_conf);
  assert(ret == HAL_OK);
  assert(p_conf->display_pipe_width >= p_conf->display_pipe_height);

  if (p_conf->aspect_ratio_mode == ASPECT_RATIO_MODE_1)
  {
    /* Crop 2592x1944 --> 1944x1944 */
    crop_conf.VStart = 0;
    crop_conf.HStart = (CAMERA_WIDTH - CAMERA_HEIGHT)/2;
    crop_conf.VSize = CAMERA_HEIGHT;
    crop_conf.HSize = CAMERA_WIDTH;
    crop_conf.PipeArea = DCMIPP_POSITIVE_AREA;
    ret = HAL_DCMIPP_PIPE_SetCropConfig(hdcmipp, DCMIPP_PIPE1, &crop_conf);
    assert(ret == HAL_OK);
    ret = HAL_DCMIPP_PIPE_EnableCrop(hdcmipp, DCMIPP_PIPE1);
    assert(ret == HAL_OK);

    ratio_height = ((float)(CAMERA_HEIGHT/2)) / p_conf->display_pipe_height;
    ratio_width = ((float)(CAMERA_HEIGHT/2)) / p_conf->display_pipe_width;
    /* decimation 1944x1944 --X/2xY/2--> 972x972 */
    /* downzise 972x972 --> 480x480 */
  }
  else if(p_conf->aspect_ratio_mode == ASPECT_RATIO_MODE_2)
  {
    ratio_width = ((float)(CAMERA_WIDTH/2)) / p_conf->display_pipe_width;
    ratio_height = ((float)(CAMERA_HEIGHT/2)) / p_conf->display_pipe_height;
    /* decimation 2592x1944 --X/2xY/2--> 1296x972 */
    /* downzise 1296x972 --> 480x480 */
  }
  else
  {
    ratio_height = ((float)(CAMERA_HEIGHT/2)) / p_conf->display_pipe_height;
    ratio_width = (float) ratio_height;
    /* decimation 2592x1944 --X/2xY/2--> 1296x972 */
    /* downzise 1296x972 --> 640x480 */
  }

  decim_conf.VRatio = DCMIPP_VDEC_1_OUT_2;
  decim_conf.HRatio = DCMIPP_HDEC_1_OUT_2;
  ret = HAL_DCMIPP_PIPE_SetDecimationConfig(hdcmipp, DCMIPP_PIPE1, &decim_conf);
  assert(ret == HAL_OK);
  ret = HAL_DCMIPP_PIPE_EnableDecimation(hdcmipp, DCMIPP_PIPE1);
  assert(ret == HAL_OK);

  ret = HAL_DCMIPP_PIPE_EnableGammaConversion(hdcmipp, DCMIPP_PIPE1);
  assert(ret == HAL_OK);

  downsize_conf.HRatio = (uint32_t) (8192 * ratio_width);
  downsize_conf.VRatio = (uint32_t) (8192 * ratio_height);
  downsize_conf.HDivFactor = (1024 * 8192 - 1) / downsize_conf.HRatio;
  downsize_conf.VDivFactor = (1024 * 8192 - 1) / downsize_conf.VRatio;
  downsize_conf.HSize = p_conf->display_pipe_width;
  downsize_conf.VSize = p_conf->display_pipe_height;
  ret = HAL_DCMIPP_PIPE_SetDownsizeConfig(hdcmipp, DCMIPP_PIPE1, &downsize_conf);
  assert(ret == HAL_OK);
  ret = HAL_DCMIPP_PIPE_EnableDownsize(hdcmipp, DCMIPP_PIPE1);
  assert(ret == HAL_OK);
}

static void DCMIPP_PipeInitNn(DCMIPP_HandleTypeDef *hdcmipp, CAM_Conf_t *p_conf)
{
  float ratio_width;
  float ratio_height;
  DCMIPP_PipeConfTypeDef pipe_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CropConfTypeDef crop_conf = { 0 };
  DCMIPP_DownsizeTypeDef downsize_conf = { 0 };
  DCMIPP_DecimationConfTypeDef decim_conf = { 0 };
  int ret;

  /* Only Aspect ration 1:1 supported */
  assert(p_conf->nn_pipe_width == p_conf->nn_pipe_height);

  csi_conf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  csi_conf.DataTypeIDA = DCMIPP_DT_RAW10;
  csi_conf.DataTypeIDB = 0;
  ret = HAL_DCMIPP_CSI_PIPE_SetConfig(hdcmipp, DCMIPP_PIPE2, &csi_conf);
  assert(ret == HAL_OK);

  ret = HAL_DCMIPP_PIPE_CSI_EnableShare(hdcmipp, DCMIPP_PIPE2);
  assert(ret == HAL_OK);

  if (p_conf->aspect_ratio_mode == ASPECT_RATIO_MODE_1)
  {
    /* Crop 2592x1944 --> 1944x1944 */
    crop_conf.VStart = 0;
    crop_conf.HStart = (CAMERA_WIDTH - CAMERA_HEIGHT)/2;
    crop_conf.VSize = CAMERA_HEIGHT;
    crop_conf.HSize = CAMERA_HEIGHT;
    crop_conf.PipeArea = DCMIPP_POSITIVE_AREA;
    ret = HAL_DCMIPP_PIPE_SetCropConfig(hdcmipp, DCMIPP_PIPE2, &crop_conf);
    assert(ret == HAL_OK);
    ret = HAL_DCMIPP_PIPE_EnableCrop(hdcmipp, DCMIPP_PIPE2);
    assert(ret == HAL_OK);

    ratio_width = ((float)(CAMERA_HEIGHT/2)) / p_conf->nn_pipe_width;
    ratio_height = ((float)(CAMERA_HEIGHT/2)) / p_conf->nn_pipe_height;
    /* decimation 1944x1944 --X/2xY/2--> 970x970 */
  }
  else
  {
    ratio_width = ((float)(CAMERA_WIDTH/2)) / p_conf->nn_pipe_width;
    ratio_height = ((float)(CAMERA_HEIGHT/2)) / p_conf->nn_pipe_height;
    /* decimation 2592x1944 --X/2xY/2--> 1296x970 */
  }

  decim_conf.VRatio = DCMIPP_VDEC_1_OUT_2;
  decim_conf.HRatio = DCMIPP_HDEC_1_OUT_2;
  /* decimation 1944x1944 --X/4xY/4--> 486x486 */
  ret = HAL_DCMIPP_PIPE_SetDecimationConfig(hdcmipp, DCMIPP_PIPE2, &decim_conf);
  assert(ret == HAL_OK);
  ret = HAL_DCMIPP_PIPE_EnableDecimation(hdcmipp, DCMIPP_PIPE2);
  assert(ret == HAL_OK);

  downsize_conf.HRatio = (uint32_t) (8192 * ratio_width);
  downsize_conf.VRatio = (uint32_t) (8192 * ratio_height);
  downsize_conf.HDivFactor = (1024 * 8192 - 1) / downsize_conf.HRatio;
  downsize_conf.VDivFactor = (1024 * 8192 - 1) / downsize_conf.VRatio;
  downsize_conf.HSize = p_conf->nn_pipe_width;
  downsize_conf.VSize = p_conf->nn_pipe_height;
  ret = HAL_DCMIPP_PIPE_SetDownsizeConfig(hdcmipp, DCMIPP_PIPE2, &downsize_conf);
  assert(ret == HAL_OK);
  ret = HAL_DCMIPP_PIPE_EnableDownsize(hdcmipp, DCMIPP_PIPE2);
  assert(ret == HAL_OK);

  ret = HAL_DCMIPP_PIPE_EnableRedBlueSwap(hdcmipp, DCMIPP_PIPE2);
  assert(ret == HAL_OK);

  ret = HAL_DCMIPP_PIPE_EnableGammaConversion(hdcmipp, DCMIPP_PIPE2);
  assert(ret == HAL_OK);

  /* Config pipe */
  pipe_conf.FrameRate = DCMIPP_FRAME_RATE_ALL;
  pipe_conf.PixelPipePitch = p_conf->nn_pipe_width * p_conf->nn_pipe_bpp;
  pipe_conf.PixelPackerFormat = p_conf->nn_pipe_format;
  ret = HAL_DCMIPP_PIPE_SetConfig(hdcmipp, DCMIPP_PIPE2, &pipe_conf);
  assert(ret == HAL_OK);
}

static void DCMIPP_Init(DCMIPP_HandleTypeDef *hdcmipp)
{
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  int ret;

  hdcmipp->Instance = DCMIPP;
  ret = HAL_DCMIPP_Init(hdcmipp);
  assert(ret == HAL_OK);

  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_1600;
  ret = HAL_DCMIPP_CSI_SetConfig(hdcmipp, &csi_conf);
  assert(ret == HAL_OK);

  ret = HAL_DCMIPP_CSI_SetVCConfig(hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10);
  assert(ret == HAL_OK);
}

HAL_StatusTypeDef MX_DCMIPP_Init(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Instance)
{
    DCMIPP_Init(hdcmipp);
    DCMIPP_PipeInitDisplay(hdcmipp, &current_conf);
    DCMIPP_PipeInitNn(hdcmipp, &current_conf);
    return HAL_OK;
}

static ISP_StatusTypeDef CB_ISP_SetSensorGain(uint32_t camera_instance, int32_t gain)
{
  if (CMW_CAMERA_SetGain(camera_instance, gain) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORGAIN;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_GetSensorGain(uint32_t camera_instance, int32_t *gain)
{
  if (CMW_CAMERA_GetGain(camera_instance, gain) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORGAIN;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_SetSensorExposure(uint32_t camera_instance, int32_t exposure)
{
  if (CMW_CAMERA_SetExposure(camera_instance, exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_GetSensorExposure(uint32_t camera_instance, int32_t *exposure)
{
  if (CMW_CAMERA_GetExposure(camera_instance, exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}


void CAM_IMX335_Init(CAM_Conf_t *p_conf)
{
  ISP_StatAreaTypeDef isp_stat_area;
  uint32_t res;
  int ret;

  current_conf = *p_conf;

  ret = CAM_IMX335_getRes(p_conf, &res);
  assert(ret == 0);

  ret = CMW_CAMERA_Init(0, res, 0);
  assert(ret == CMW_ERROR_NONE);
  int mirror_flip = CAM_GetMirrorFlip(current_conf.cam_flip);
  ret = CMW_CAMERA_SetMirrorFlip(0, mirror_flip);
  assert(ret == 0);

  isp_stat_area.X0 = 0;
  isp_stat_area.Y0 = 0;
  isp_stat_area.XSize = 2592;
  isp_stat_area.YSize = 1944;

  ISP_AppliHelpersTypeDef appliHelpers = {
    .SetSensorGain = CB_ISP_SetSensorGain,
    .GetSensorGain = CB_ISP_GetSensorGain,
    .SetSensorExposure = CB_ISP_SetSensorExposure,
    .GetSensorExposure = CB_ISP_GetSensorExposure,
  };

  ret = ISP_Init(&hIsp, &hcamera_dcmipp, 0, &appliHelpers, &isp_stat_area);
  assert(ret == 0);
}

void CAM_IMX335_DisplayPipe_Start(uint32_t cam_mode)
{
  int ret;

  ret = ISP_Start(&hIsp);
  assert(ret == 0);

  ret = CMW_CAMERA_Start(0, DCMIPP_PIPE1, current_conf.display_pipe_dst, cam_mode);
  assert(ret == CMW_ERROR_NONE);
}

void CAM_IMX335_NNPipe_Start(uint32_t cam_mode)
{
  int ret;
  ret = CMW_CAMERA_Start(0, DCMIPP_PIPE2, current_conf.nn_pipe_dst, cam_mode);
  assert(ret == CMW_ERROR_NONE);
}

void CAM_IMX335_NNPipe_Stop(void)
{
  int ret;
  ret = CMW_CAMERA_Stop(0);
  assert(ret == CMW_ERROR_NONE);

//  ret = HAL_CSI_DeInit(&hcamera_csi);                        <======== TO BE UNCOMMENTED
//  assert(ret == HAL_OK);

  ret = HAL_DCMIPP_DeInit(&hcamera_dcmipp);
  assert(ret == HAL_OK);
}

void CAM_IMX335_Sensor_Start(void)
{
//  CMW_CAMERA_Sensor_Start();                                <======== TO BE UNCOMMENTED
}

void CAM_IMX335_Sensor_Stop(void)
{
 // CMW_CAMERA_Sensor_Stop();                                   <======== TO BE UNCOMMENTED
}

void CAM_IMX335_IspUpdate(void)
{
  int ret;
  ret = ISP_BackgroundProcess(&hIsp);
  assert(ret == 0);
}

/**
 * @brief  Vsync Event callback on pipe
 * @param  hdcmipp DCMIPP device handle
 *         Pipe    Pipe receiving the callback
 * @retval None
 */
void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);
  if (Pipe == DCMIPP_PIPE1 )
  {
    /* Call the IPS statistics handler */
    ISP_GatherStatistics(&hIsp);
  }
}

/**
 * @brief  Frame Event callback on pipe
 * @param  hdcmipp DCMIPP device handle
 *         Pipe    Pipe receiving the callback
 * @retval None
 */
void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);
  switch (Pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&hIsp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&hIsp);
      break;
    case DCMIPP_PIPE2 :
      cameraFrameReceived++;
      ISP_IncAncillaryFrameId(&hIsp);
      break;
  }
}

/**
  * @brief  DCMIPP Clock Config for DCMIPP.
  * @param  hcsi  DCMIPP Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp)
{
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};
  HAL_StatusTypeDef ret = HAL_OK;

  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DCMIPP;
  RCC_PeriphCLKInitStruct.DcmippClockSelection = RCC_DCMIPPCLKSOURCE_IC17;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockSelection = RCC_ICCLKSOURCE_PLL2;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockDivider = 3;
  ret = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
  if (ret)
  {
    return ret;
  }

  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CSI;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockDivider = 40;
  ret = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
  if (ret)
  {
    return ret;
  }

  return ret;
}


#endif
