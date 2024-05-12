sd2iec - a controller/interface adapting storage devices to the CBM serial bus

This is an ESP32 only port.

Copyright (C) 2007-2022  Ingo Korb <ingo@akana.de>  
Parts based on code from others, see comments in main.c for details.  
JiffyDos send based on code by M.Kiesel  
Fat LFN support and lots of other ideas+code by Jim Brain  
Final Cartridge III fastloader support by Thomas Giesel  
IEEE488 support by Nils Eilers  
ESP32 port by Jarkko Sonninen  

Free software under GPL version 2 ONLY, see comments in main.c and
COPYING for details.

# Note #

This is an unofficial clone of the original repository.  
The original repository is available on [https://www.sd2iec.de](https://www.sd2iec.de)

See the original [README](components/sd2iec/README)

# I/O

Pins in my ESP32S3 board. Use menuconfig to change the values
| CONFIG_SD2IEC_SD_PIN_MOSI | 47 |
| CONFIG_SD2IEC_SD_PIN_MISO | 41 |
| CONFIG_SD2IEC_SD_PIN_CLK | 48 |
| CONFIG_SD2IEC_SD_PIN_CS | 42 |
| CONFIG_SD2IEC_PIN_CLK | 1 |
| CONFIG_SD2IEC_PIN_DATA | 2 |
| CONFIG_SD2IEC_PIN_ATN | 40 |
| CONFIG_SD2IEC_PIN_SRQ | -1 |
| CONFIG_SD2IEC_PIN_LED_BUSY | -1 |
| CONFIG_SD2IEC_PIN_LED_DIRTY | -1 |

Use a level shifter for CLK,DATA and ATN pins.

# Building
Use [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html) to compile.
```
idf.py menuconfig
idf.py -p /dev/ttyUSB0 build flash monitor
```

# Storage
SD2IEC have a concept of partitions. These are not FAT or SD card partitions.
Partition 0 is the FAT file system in SDCARD.
Partition 1 is a part of board flash as a FAT file system. You can use it for storing utilities like file browsers and fast loaders.