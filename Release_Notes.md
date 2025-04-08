---
pagetitle: Release Notes for STM32N6 AI Power Measurement Application
lang: en
header-includes: <link rel="icon" type="image/x-icon" href="_htmresc/favicon.png" />
---

::: {.row}
::: {.col-sm-12 .col-lg-4}

<center>
# Release Notes for <mark>x-cube-n6-ai-power-measurement</mark>
Copyright &copy; 2024 STMicroelectronics\
    
[![ST logo](_htmresc/st_logo_2020.png)](https://www.st.com){.logo}
</center>

# Purpose

Computer Vision application to enable power consumtion measurement on STM32N6570-DK board.

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
:::

::: {.col-sm-12 .col-lg-8}
# Update history

::: {.collapse}
<input type="checkbox" id="collapse-section1" checked aria-hidden="true"/>
<label for="collapse-section1" aria-hidden="true">__V1.0.0 / December 2024__</label>
<div>

## Enhancements, new features and limitations

 Initial Release

## Software components

| Name                          | Version             | Release notes
|-----                          | -------             | -------------
| STM32Cube.AI runtime          | 10.0.0              | [release notes](Lib/AI_Runtime/README.md)
| Camera Middleware             | v1.4.1              | [release notes](Lib/Camera_Middleware/Release_Notes.html)
| lib_vision_models_pp Library  | v0.7.2              | [release notes](Lib/lib_vision_models_pp/lib_vision_models_pp/README.md)
| CMSIS                         | V5.9.0              | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Documentation/index.html)
| STM32N6xx CMSIS Device        | V1.0.0              | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Release_Notes.html)
| STM32N6xx HAL/LL Drivers      | V1.0.0              | [release notes](STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Release_Notes.html)
| STM32N6570-DK BSP Drivers     | V1.0.0              | [release notes](STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/Release_Notes.html)
| BSP Component aps256xx        | V1.0.6              | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/aps256xx/Release_Notes.html)
| BSP Component Common          | V7.3.0              | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/Common/Release_Notes.html)
| BSP Component mx66uw1g45g     | V1.1.0              | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/mx66uw1g45g/Release_Notes.html)
| Power measurement scripts     | V1.0.0              | [release notes](Utilities/pwr_scripts/Release_Notes.md)


## Available documentation
- README.md
- Doc/Application-Overview.md
- Doc/Deploy-your-tflite-Model.md
- Doc/Build-Options.md
- Doc/Program-Hex-Files-STM32CubeProgrammer.md

</div>
:::


:::
:::

<footer class="sticky">
::: {.columns}
::: {.column width="95%"}
For complete documentation on STM32,
visit: [www.st.com](http://www.st.com)

*This release note uses up to date web standards and, for this reason, should not be opened with Internet Explorer but preferably with popular browsers such as Google Chrome, Mozilla Firefox, Opera or Microsoft Edge.*
:::
::: {.column width="5%"}
<abbr title="Based on template cx566953 version 2.0">Info</abbr>
:::
:::
</footer>
