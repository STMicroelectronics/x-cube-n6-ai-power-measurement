#!/bin/bash

stedgeai generate --no-inputs-allocation --model quantized_tiny_yolo_v2_224_.tflite --target stm32n6 --st-neural-art default@user_neuralart.json --quiet
cp st_ai_output/network.c .
cp st_ai_output/stai_network.c .
cp st_ai_output/stai_network.h .
cp st_ai_output/network_c_info.json .
cp st_ai_output/network_ecblobs.h .
cp st_ai_output/network_atonbuf.xSPI2.raw network_data.xSPI2.bin
arm-none-eabi-objcopy -I binary network_data.xSPI2.bin --change-addresses 0x70380000 -O ihex network_data.hex