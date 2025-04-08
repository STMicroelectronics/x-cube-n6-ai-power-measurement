 /**
 ******************************************************************************
 * @file    cam_imx335.h
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
#ifndef CAM_IMX335
#define CAM_IMX335

#include "cam.h"


#define CAMERA_FPS 30

void CAM_IMX335_Init(CAM_Conf_t *p_conf);
void CAM_IMX335_Start(void);
void CAM_IMX335_IspUpdate(void);
void CAM_IMX335_DisplayPipe_Start(uint32_t cam_mode);
void CAM_IMX335_NNPipe_Start(uint32_t cam_mode);

void CAM_IMX335_NNPipe_Stop(void);

void CAM_IMX335_Sensor_Start(void);
void CAM_IMX335_Sensor_Stop(void);

#endif
