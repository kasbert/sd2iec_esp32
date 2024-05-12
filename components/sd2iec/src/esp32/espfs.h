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


   espfs.c: ESP32 VFS low level handling

*/

#ifndef ESPFS_H
#define ESPFS_H

#include "sdkconfig.h"


bool esp32fs_create(void);

#if CONFIG_SD2IEC_USE_SDCARD
#define SDMOUNT_POINT "/sdcard"
bool esp32fs_sdcard_init();
void esp32fs_sdcard_del();
bool esp32fs_sdcard_mount( char *mount_point);
void esp32fs_sdcard_unmount( char *mount_point);
bool esp32fs_sdcard_format( char *mount_point);
bool esp32fs_sdcard_ismounted();
const char *esp32fs_sdcard_get_type();
const char *esp32fs_sdcard_get_name();
uint64_t esp32fs_sdcard_get_size();
#endif

#if CONFIG_SD2IEC_USE_SPI_PARTITION
#define SPIMOUNT_POINT "/flash"
bool esp32fs_spiflash_mount( char *mount_point);
void esp32fs_spiflash_unmount( char *mount_point);
#endif

uint64_t esp32fs_get_bytes_free(const char *mount_point);
uint64_t esp32fs_get_bytes_used(const char *mount_point);

void esp32fs_list_files(char *path);
void esp32fs_filetest(char *mount_point, const char *txt);

#endif