
/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2024 Jarkko Sonninen <kasper@iki.fi>

   Inspired by MMC2IEC by Lars Pontoppidan et al.

   FAT filesystem access based on code from ChaN and Jim Brain, see ff.c|h.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License only.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


   autoconf.h: Architecture-specific system definitions

*/
// Emulates the original build system
#ifndef AUTOCONF_H
#define AUTOCONF_H
#include "sdkconfig.h"

#define CONFIG_ARCH esp
#define CONFIG_HARDWARE_NAME sd2iec-esp32
#define VERSION "1.0"
#define LONGVERSION "1.0-X"

#define CONFIG_ERROR_BUFFER_SIZE 100
#define CONFIG_COMMAND_BUFFER_SIZE 250
#define CONFIG_BUFFER_COUNT 15
#define CONFIG_MAX_PARTITIONS 4
#define HAVE_CLOCK_IRQ 1

#define CONFIG_HAVE_IEC CONFIG_SD2IEC_ENABLE_IEC

#define IEC_PIN_ATN   CONFIG_SD2IEC_PIN_ATN
#define IEC_PIN_CLOCK CONFIG_SD2IEC_PIN_CLK
#define IEC_PIN_DATA  CONFIG_SD2IEC_PIN_DATA

#define CONFIG_COMMAND_CHANNEL_DUMP
#define CONFIG_DISPLAY_BUFFER_SIZE 40
#define CONFIG_LOADER_AR6
#define CONFIG_LOADER_DREAMLOAD
#define CONFIG_LOADER_ELOAD1
#define CONFIG_LOADER_EPYXCART
#define CONFIG_LOADER_FC3
#define CONFIG_LOADER_GEOS
#define CONFIG_LOADER_GIJOE
#define CONFIG_LOADER_MMZAK
#define CONFIG_LOADER_N0SDOS
#define CONFIG_LOADER_NIPPON
#define CONFIG_LOADER_SAMSJOURNEY
#define CONFIG_LOADER_TURBODISK
#define CONFIG_LOADER_ULOAD3
#define CONFIG_LOADER_WHEELS
#define CONFIG_MCU esp32
#if CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160
#define CONFIG_MCU_FREQ 160000000
#endif
#define CONFIG_SD_AUTO_RETRIES 10
#define CONFIG_SD_BLOCKTRANSFER
#define CONFIG_SD_DATACRC
#define CONFIG_SPI_FLASH_SIZE_MAP 4
#define CONFIG_UART_BAUDRATE 115200

//#define CONFIG_REMOTE_DISPLAY 1
#define HAVE_I2C 1
//#define CONFIG_HAVE_FATFS
#define CONFIG_HAVE_VFS 1
#define CONFIG_HARDWARE_VARIANT 2
#define CONFIG_UART_DEBUG 1
#define CONFIG_ERROR_BUFFER_SIZE 100
#define CONFIG_COMMAND_BUFFER_SIZE 250
#define CONFIG_BUFFER_COUNT 15

#define CONFIG_DEBUG_VERBOSE 1

#endif
