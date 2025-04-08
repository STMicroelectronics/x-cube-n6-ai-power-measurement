# Build Options

Some features are enabled using build options or using `app_config.h`:

- [Frequency scaling](#Frequency-scaling)
- [Cameras module](#cameras-module)
- [Camera Orientation](#camera-orientation)


This documentation explains those feature and how to modify them.

## Frequency-scaling

### Frequency and power modes
#### Overdrive Mode
The overdrive or performance mode is activated when the voltage scaling (VOS) is high (0.89 volts). This mode allows reaching high frequencies:
- **1 GHz** for the NPU
- **900 MHz** for the NPU RAMS (AXISRAM3/4/5/6)
- **800 MHz** for the CPU

#### Nominal Mode
The nominal mode is activated when the configured VOS is low (0.8 volts). The frequencies are then as follows:
- **800 MHz** for the NPU and NPU RAMS (AXISRAM3/4/5/6)
- **600 MHz** for the CPU

To change the mode, modify the value of `POWER_OVERDRIVE`:
- `1`: Overdrive mode
- `0`: Nominal mode


### CPU clock scaling
To enable scale down mode, where the CPU is clocked using HSE before inference, use the following values for `CPU_FRQ_SCALE_DOWN`:
- `1`: CPU is clocked by HSE (48 MHz) before inference and during NPU hardware epochs, but switches to maximum frequency during epoch configuration and post-processing.
- `0`: No CPU scaling; CPU is clocked at the maximum frequency of the configured mode (overdrive/nominal).

### NPU clock scaling
To enable NPU clock scaling mode, use the `NPU_FRQ_SCALING` definition:
- `1`: NPU clock scaling enabled; `POWER_OVERDRIVE` has no effect.
- `0`: NPU clock scaling disabled.

When this mode is enabled, the following measurements are automatically performed:

| Mode      | NPU Frequency | NPU RAMS Freq | CPU Frequency | Step Name            |
|-----------|---------------|---------------|---------------|----------------------|
| Overdrive | 1 GHz         | 900 MHz       | 800 MHz       | nn_inference_1GHz    |
| Nominal   | 800 MHz       | 800 MHz       | 600 MHz       | nn_inference_800MHz  |
| Nominal   | 600 MHz       | 600 MHz       | 600 MHz       | nn_inference_600MHz  |
| Nominal   | 400 MHz       | 400 MHz       | 600 MHz       | nn_inference_400MHz  |
| Nominal   | 200 MHz       | 200 MHz       | 600 MHz       | nn_inference_200MHz  |
| Nominal   | 100 MHz       | 100 MHz       | 600 MHz       | nn_inference_100MHz  |

## Using external PSRAM
By default, the external PSRAM is disabled to avoid unnecessary power consumption when it is not needed by the application. However, it may be required in certain cases, such as when the new model activations do not fit in the internal RAMs. In such scenarios, enable the configuration of the PSRAM using `USE_PSRAM`:
- `1`: external PSRAM enabled.
- `0`: external PSRAM disabled.

## Cameras module

The Application is compatible with 4 Cameras:

- MB1854B IMX335 (Default Camera provided with the MB1939 STM32N6570-DK board)
- ST VD66GY
- ST VD55G1

### Makefile

```bash
make -j8
```

### STM32CubeIDE

1. Right Click on the project -> Properties
2. Click on C/C++ General
3. Click on Path and Symbols
4. Click Symbols
5. Delete `USE_IMX335_SENSOR`
6. Add a Symbol following the pattern `USE_<CAMERA>_SENSOR`; Replace \<CAMERA\> by one of these IMX335; VD66GY; VD55G1
7. Click on Apply and Close



### IAR EWARM

1. Right Click on the project -> Options
2. Click on C/C++ Compiler
3. Click on Preprocessor
4. Scroll in Defined Symbols window
5. Modify `USE_IMX335_SENSOR` following the pattern `USE_<CAMERA>_SENSOR`; Replace \<CAMERA\> by one of these IMX335; VD66GY; VD55G1
6. Click on OK


## Camera Orientation

Cameras allows to flip the image along 2 axis.

- CAMERA_FLIP_HFLIP: Selfie mode
- CAMERA_FLIP_VFLIP: Flip upside down.
- CAMERA_FLIP_HVFLIP: Flip Both axis
- CAMERA_FLIP_NONE: Default

1. Open [app_config.h](../Inc/app_config.h)

2. Change CAMERA_FLIP define:
```C
/*Defines: CMW_MIRRORFLIP_NONE; CMW_MIRRORFLIP_FLIP; CMW_MIRRORFLIP_MIRROR; CMW_MIRRORFLIP_FLIP_MIRROR;*/
#define CAMERA_FLIP CMW_MIRRORFLIP_FLIP
```
