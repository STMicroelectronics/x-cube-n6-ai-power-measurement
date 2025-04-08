# How to Program hex files

1. Making sure board is in dev mode (boot switch in dev mode position)
2. Open STM32CubeProgrammer
3. Select the Disco board through the "External loaders" tab.
4. ST-link configuration: mode set to "Hot plug"
5. Connect board
6. From the "Erasing & programming" tab, select the `Binary/ai_fsbl.hex` file.
7. Wait for flashing to complete.
8. From the "Erasing & programming" tab, select the `Binary/network_data.hex` file.
9. Wait for flashing to complete.
10. From the "Erasing & programming" tab select the ` Binary/x-cube-n6-ai-power-measurement.hex` file.
11. Wait for flashing to complete.

![Board Selection](../_htmresc/selectBoard.JPG)
![Flash the Hex file](../_htmresc/flashHex.JPG)
