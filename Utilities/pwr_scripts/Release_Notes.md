# Release Notes for power measurement scripts

## Purpose

Those scripts are developed to automatize the power measurment on STM32N6 DK board using STLINK-V3PWR.

## Key Features

- Support for the STM32N6570-DK board and STLINK-V3PWR only
- Script control the HW setup (STLinkPwr and N6 DK board)
- Retrieve the data produced by STLinkPwr and Timestamps log from N6 DK board
- Post-process the data and generate power report 


## Supported Devices and Boards

- STM32N6xx devices
- MB1939 STM32N6570-DK
- STLINK-V3PWR

## Update history

### V1.0.3 / January 2026

- Update user message in capture.py

### V1.0.2 / August 2025

- Update documents

### V1.0.1 / April 2025

- Update requirements file: Fixed PyQt version


### V1.0.0 / December 2024

- Initial release