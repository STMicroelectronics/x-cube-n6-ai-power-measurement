 /**
 ******************************************************************************
 * @file    app_postprocess_od_yolov8_ui.c
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


#include "app_postprocess.h"
#include "app_config.h"
#include <assert.h>

#if POSTPROCESS_TYPE == POSTPROCESS_OD_YOLO_V8_UI
static od_pp_outBuffer_t out_detections[AI_OD_YOLOV8_PP_TOTAL_BOXES];

int32_t app_postprocess_init(void *params_postprocess)
{
  int32_t error = AI_OD_POSTPROCESS_ERROR_NO;
  yolov8_pp_static_param_t *params = (yolov8_pp_static_param_t *) params_postprocess;
  params->nb_classes = AI_OD_YOLOV8_PP_NB_CLASSES;
  params->nb_total_boxes = AI_OD_YOLOV8_PP_TOTAL_BOXES;
  params->max_boxes_limit = AI_OD_YOLOV8_PP_MAX_BOXES_LIMIT;
  params->conf_threshold = AI_OD_YOLOV8_PP_CONF_THRESHOLD;
  params->iou_threshold = AI_OD_YOLOV8_PP_IOU_THRESHOLD;
  params->raw_output_scale = AI_OD_YOLOV8_PP_SCALE;
  params->raw_output_zero_point = AI_OD_YOLOV8_PP_ZERO_POINT;
  error = od_yolov8_pp_reset(params);

  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 1);
  int32_t error = AI_OD_POSTPROCESS_ERROR_NO;
  od_pp_out_t *pObjDetOutput = (od_pp_out_t *) pOutput;
  pObjDetOutput->pOutBuff = out_detections;
  yolov8_pp_in_centroid_int8_t pp_input = {
      .pRaw_detections = (int8_t *) pInput[0]
  };
  error = od_yolov8_pp_process_int8(&pp_input, pObjDetOutput,
                                    (yolov8_pp_static_param_t *) pInput_param);
  return error;
}
#endif
