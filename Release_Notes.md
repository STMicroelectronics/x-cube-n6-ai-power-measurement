# Release Notes for x-cube-n6-ai-power-measurement

## Purpose

Computer Vision application to enable power measurement on STM32N6570-DK board.

This application is prebuilt with a people detection model "TinyYOLOv2".

## Key Features

- Sequential application flow
- NPU accelerated quantized AI model inference
- DCMIPP pipe
- DCMIPP crop, decimation, downscale
- DCMIPP ISP usage
- Dev mode
- Boot from External Flash
- Wake-up from sleep using USER1 button
- System requency scaling (switching betwing Overdrive and nominal modes)
- De-init of unused IPs
- CPU sleep during capture and during NPU HW epochs (ASYNC mode)
- CPU and NPU frequency scaling

## Software components

| Name                          | Version             | Release notes
|-----                          | -------             | -------------
| STM32Cube.AI runtime          | 10.1.0              | 
| Camera Middleware             | v1.4.2              | [release notes](Lib/Camera_Middleware/Release_Notes.html)
| lib_vision_models_pp Library  | v0.8.0              | [release notes](Lib/lib_vision_models_pp/lib_vision_models_pp/README.md)
| post process wrapper          | v1.0.2              | [release notes](Lib/ai-postprocessing-wrapper/Release_Notes.html)
| CMSIS                         | V5.9.0              | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Documentation/index.html)
| STM32N6xx CMSIS Device        | V1.1.0              | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Release_Notes.html)
| STM32N6xx HAL/LL Drivers      | V1.1.0              | [release notes](STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Release_Notes.html)
| STM32N6570-DK BSP Drivers     | V1.1.0              | [release notes](STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/Release_Notes.html)
| BSP Component aps256xx        | V1.0.6              | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/aps256xx/Release_Notes.html)
| BSP Component Common          | V7.3.0              | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/Common/Release_Notes.html)
| BSP Component mx66uw1g45g     | V1.1.0              | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/mx66uw1g45g/Release_Notes.html)
| Power measurement scripts     | V1.0.0              | [release notes](Utilities/pwr_scripts/Release_Notes.md)

## Update history

### V2.0.0 / May 2025

- Update submodules (Camera Middleware, STM32Cube.AI runtime, lib_vision_models_pp, STM32Cube_FW_N6)
- added post process wrapper

### V1.0.0 / December 2024

Initial Version
