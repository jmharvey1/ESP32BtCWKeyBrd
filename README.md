﻿
# ESP32 Bluetooth CW Keyboard & Decoder
___

A VS Code/PlatformIO project, Using ESPIDF's framework.
  
This ham radio project uses a generic Bluetooth Keyboard to send Morse Code (cw), and now using the audio out of your RX can decode CW too.
<p align="center">  
<img src="https://github.com/jmharvey1/ESP32BtCWKeyBrd/blob/main/MiscFiles/IMG_5733.JPG"  width="40%" height="40%">
</p>
  
 
<p dir="auto">For additional info, see YouTube video:
    <a href="https://youtu.be/Hb9m60LtSJw" rel="nofollow">https://youtu.be/Hb9m60LtSJw</a></p>
    
    
---
Main Hardware needed to replicate this project:
  
ESP32 WROOM dev board, (30 pin version)
  
3.5" TFT LCD Screen Display Module ILI9488 Board SPI Interface 480x320
  
Bluetooth Keyboard (Logitech K380, recommended)
  
___


Additional parts needed:
  
D4184 MOS FET (1)
  
HYDZ PIEZO Buzzer (1)

2n3904 (1)

2n3906 (1)

0.1ufd (3)

10ufd (1)
 
resistors 2.2K to 15K (4)
  
___
For those who want to bypass the source code, and just "flash" your ESP32, download the [ESPhomeFlasher](https://github.com/esphome/esphome-flasher/releases) and the [.bin file](https://github.com/jmharvey1/ESP32BtCWKeyBrd/blob/main/.pio/build/upesy_wroom/firmware.bin) found at these links.
  
Note: For me, using Linux Mint, the ESPhome-Flasher's 'Browse' button did NOT work. But placing the 'firmware.bin' file in the same folder as the flasher app, allowed me to just type the file name,"firmware.bin",
in the 'Firmware' cell.
  
A 2nd tip: Connect the ESP32 to your computer BEFORE launching the ESPhome-Flasher. 
___
A PCB for this project can be ordered from a board manufacturer, like [JLCPCB](https://jlcpcb.com/), using the gerber files found [Here](https://github.com/jmharvey1/ESP32BtCWKeyBrd/blob/main/MiscFiles/ESP32-BT-CW-KeyBrd_2023-03-16.zip).
  
A .PDF schematic for the PCB version of this project is [Here](https://github.com/jmharvey1/ESP32BtCWKeyBrd/blob/main/MiscFiles/ESP32-BT-CW-KeyBrd_Schematic.pdf)
  
If you have access to 3D printer, the [MiscFiles](https://github.com/jmharvey1/ESP32BtCWKeyBrd/tree/main/MiscFiles)
 folder contains “.stl” files for a simple case to host your completed project. 
___
Note: the PCB is currently not provisioned for the the decoder's audio input circuitry, nor is it documented in the MiscFiles folder.
But is super simple to add. It consists of pair of 10K voltage divider reisistors connected between gnd & the ESP32's 3.3V input, with the center tap connected to GPIO pin34 (ADC channel 6). A 0.1ufd DC blocking Capacitor is used to couple the external audio source (the RX's audio/speaker out) to GPIO pin 34. 