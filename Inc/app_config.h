 /**
 ******************************************************************************
 * @file    app_config.h
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
#ifndef APP_CONFIG
#define APP_CONFIG

#include "arm_math.h"

#ifndef POWER_OVERDRIVE
#define POWER_OVERDRIVE        1  /* 1: Overdrive mode, 0: Nominal mode */
#endif

#ifndef CPU_FRQ_SCALE_DOWN
#define CPU_FRQ_SCALE_DOWN     0  /* Scale down mode: CPU is clocked using HSE before inference */
#endif

#ifndef NPU_FRQ_SCALING
#define NPU_FRQ_SCALING        0  /* NPU frequency scaling (100MHz, 200MHz, 400MHz, 600MHz, 800MHz) and 1GHz if overdrive enabled */
#endif

#if ( NPU_FRQ_SCALING == 1 )
#undef POWER_OVERDRIVE
#define POWER_OVERDRIVE 0
#endif

#ifndef USE_PSRAM
#define USE_PSRAM              0  /* enable/disable using external RAM */
#endif


#define USE_DCACHE


/*Defines: CMW_MIRRORFLIP_NONE; CMW_MIRRORFLIP_FLIP; CMW_MIRRORFLIP_MIRROR; CMW_MIRRORFLIP_FLIP_MIRROR;*/
#define CAMERA_FLIP CMW_MIRRORFLIP_NONE

#define ASPECT_RATIO_CROP (1) /* Crop both pipes to nn input aspect ratio; Original aspect ratio kept */
#define ASPECT_RATIO_FIT (2) /* Resize both pipe to NN input aspect ratio; Original aspect ratio not kept */
#define ASPECT_RATIO_FULLSCREEN (3) /* Resize camera image to NN input size and display a fullscreen image */
#define ASPECT_RATIO_MODE ASPECT_RATIO_CROP

#define CAPTURE_FORMAT DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1
#define CAPTURE_BPP 2
/* Leave the driver use the default resolution */
#define CAMERA_WIDTH 0
#define CAMERA_HEIGHT 0

#define LCD_FG_WIDTH             800
#define LCD_FG_HEIGHT            480
#define LCD_FG_FRAMEBUFFER_SIZE  (LCD_FG_WIDTH * LCD_FG_HEIGHT * 2)

/* Model Related Info */
#define POSTPROCESS_TYPE    POSTPROCESS_OD_YOLO_V2_UF

#define NN_WIDTH 224
#define NN_HEIGHT 224
#define NN_FORMAT DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1
#define NN_BPP 3

#define NB_CLASSES 2
#define CLASSES_TABLE const char* classes_table[NB_CLASSES] = {\
"person",   "not_person"}

/* I/O configuration */
#define AI_OD_YOLOV2_PP_NB_CLASSES        (1)
#define AI_OD_YOLOV2_PP_NB_ANCHORS        (5)
#define AI_OD_YOLOV2_PP_GRID_WIDTH        (7)
#define AI_OD_YOLOV2_PP_GRID_HEIGHT       (7)
#define AI_OD_YOLOV2_PP_NB_INPUT_BOXES    (AI_OD_YOLOV2_PP_GRID_WIDTH * AI_OD_YOLOV2_PP_GRID_HEIGHT)

/* Anchor boxes */
static const float32_t AI_OD_YOLOV2_PP_ANCHORS[2*AI_OD_YOLOV2_PP_NB_ANCHORS] = {
    0.9883000000f,     3.3606000000f,
    2.1194000000f,     5.3759000000f,
    3.0520000000f,     9.1336000000f,
    5.5517000000f,     9.3066000000f,
    9.7260000000f,     11.1422000000f,
  };

/* --------  Tuning below can be modified by the application --------- */
#define AI_OD_YOLOV2_PP_CONF_THRESHOLD    (0.6f)
#define AI_OD_YOLOV2_PP_IOU_THRESHOLD     (0.3f)
#define AI_OD_YOLOV2_PP_MAX_BOXES_LIMIT   (10)

#endif
