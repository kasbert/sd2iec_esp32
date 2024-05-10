/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2021 Jarkko Sonninen <kasper@iki.fi>
   Copyright (C) 2007-2017  Ingo Korb <ingo@akana.de>

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


   nvs-conf.c: Persistent configuration storage

*/

#include <string.h>
#include <esp_log.h>
#include "config.h"
#include <nvs_flash.h>
#include <nvs.h>
#include "flags.h"
#include "eeprom-conf.h"
#include "diskio.h"
#include "bus.h"

static void write_config_block(void *srcptr, unsigned int length);
static void read_config_block(void *destptr, unsigned int length);

#define STORAGE_NAMESPACE "sd2iec"

static const char *TAG = "nvs-conf";

uint8_t rom_filename[ROM_NAME_LENGTH+1];

/**
 * struct storedconfig - in-eeprom data structure
 * @dummy      : EEPROM position 0 is unused
 * @checksum   : Checksum over the EEPROM contents
 * @structsize : size of the eeprom structure
 * @unused     : unused byte kept for structure compatibility
 * @globalflags: subset of the globalflags variable
 * @address    : device address set by software
 * @hardaddress: device address set by jumpers
 * @fileexts   : file extension mapping mode
 * @drvflags0  : 16 bits of drv mappings, organized as 4 nybbles.
 * @drvflags1  : 16 bits of drv mappings, organized as 4 nybbles.
 * @imagedirs  : Disk images-as-directory mode
 * @romname    : M-R rom emulation file name (zero-padded, but not terminated)
 *
 * This is the data structure for the contents of the EEPROM.
 *
 * Do not remove any fields!
 * Only add fields at the end!
 */
static struct {
  uint8_t  dummy;
  uint8_t  checksum;
  uint16_t structsize;
  uint8_t  unused;
  uint8_t  global_flags;
  uint8_t  address;
  uint8_t  hardaddress;
  uint8_t  fileexts;
  uint16_t drvconfig0;
  uint16_t drvconfig1;
  uint8_t  imagedirs;
  uint8_t  romname[ROM_NAME_LENGTH];
} __attribute__((packed)) storedconfig;

#define CONFIG_MEMBER_ADDRESS(member) ((uint8_t*)(member)-(uint8_t*)&storedconfig)

/**
 * read_configuration - reads configuration from EEPROM
 *
 * This function reads the stored configuration values from the EEPROM.
 * If the stored checksum doesn't match the calculated one nothing will
 * be changed.
 */
void read_configuration(void) {
  uint_fast16_t i;
  uint8_t checksum, tmp;

  /* Set default values */
  globalflags         |= POSTMATCH;            /* Post-* matching enabled */
  file_extension_mode  = 1;                    /* Store x00 extensions except for PRG */
  set_drive_config(get_default_driveconfig()); /* Set the default drive configuration */
  memset(rom_filename, 0, sizeof(rom_filename));

#if _FIXME
  /* Use the NEXT button to skip reading the EEPROM configuration */
  if (!(buttons_read() & BUTTON_NEXT)) {
    ignore_keys();
    return;
  }
#endif

  read_config_block(&storedconfig, sizeof(storedconfig));
  ESP_LOG_BUFFER_HEXDUMP(TAG, &storedconfig, sizeof(storedconfig), ESP_LOG_INFO);
  /* abort if the size bytes are not set */
  if (storedconfig.structsize != sizeof(storedconfig)) {
    return;
  }
  uint8_t *p;
  for (checksum = 0, p = (uint8_t *)&storedconfig, i=2;i<sizeof(storedconfig);i++) {
    checksum += p[i];
  }
  if (storedconfig.checksum != checksum) {
    ESP_LOGE(TAG, "Checksum mismatch %x %x ", checksum, storedconfig.checksum);
    return;
  }

  tmp = storedconfig.global_flags;
  globalflags &= (uint8_t)~(POSTMATCH | EXTENSION_HIDING);
  globalflags |= tmp;

  if (storedconfig.hardaddress == device_hw_address())
    device_address = storedconfig.hardaddress;

  file_extension_mode = storedconfig.fileexts;

#ifdef NEED_DISKMUX
  set_drive_config(storedconfig.drvconfig0 | storedconfig.drvconfig1 << 16);

  /* sanity check.  If the user has truly turned off all drives, turn the
   * defaults back on
   */
  if(drive_config == 0xffffffff)
    set_drive_config(get_default_driveconfig());
#endif

  image_as_dir = storedconfig.imagedirs;
  strcpy((char*)rom_filename, (char*)&storedconfig.romname);
}

/**
 * write_configuration - stores configuration data to EEPROM
 *
 * This function stores the current configuration values to the EEPROM.
 */
void write_configuration(void) {
  uint_fast16_t i;
  uint8_t checksum;

  uint8_t *p;
  storedconfig.structsize = sizeof(storedconfig);
  storedconfig.global_flags = globalflags & (POSTMATCH | EXTENSION_HIDING);
  storedconfig.address = device_address;
  storedconfig.hardaddress = device_hw_address();
  storedconfig.fileexts = file_extension_mode;
#ifdef NEED_DISKMUX
  storedconfig.drvconfig0 = drive_config;
  storedconfig.drvconfig1 = drive_config >> 16;
#endif
  storedconfig.imagedirs = image_as_dir;
  memset(&storedconfig.romname, 0, sizeof(storedconfig.romname));
  strncpy((char*)&storedconfig.romname, (char*)rom_filename, sizeof(storedconfig.romname));
  for (checksum = 0, p = (uint8_t *)&storedconfig, i=2;i<sizeof(storedconfig);i++) {
    checksum += p[i];
  }
  storedconfig.checksum = checksum;
  write_config_block(&storedconfig, sizeof(storedconfig));
  ESP_LOG_BUFFER_HEXDUMP(TAG, &storedconfig, sizeof(storedconfig), ESP_LOG_INFO);
}

static int initialized;

static void read_config_block(void *destptr, unsigned int length) {
  nvs_handle_t my_handle;
  esp_err_t err;

  if (!initialized) {
    nvs_flash_init();
    initialized = 1;
  }
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open nvs");
    return;
  }
  // Read run time blob
  size_t required_size = length;  // value will default to 0, if not set yet in NVS
  // obtain required memory space to store blob being read from NVS
  err = nvs_get_blob(my_handle, "config", destptr, &required_size);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "No config block");
    goto cleanup;
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read nvs");
    goto cleanup;
  }
  cleanup:
  nvs_close(my_handle);
}

static void write_config_block(void *srcptr, unsigned int length) {
  nvs_handle_t my_handle;
  esp_err_t err;

  if (!initialized) {
    nvs_flash_init();
    initialized = 1;
  }
  // Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open nvs");
    return;
  }

  err = nvs_set_blob(my_handle, "config", srcptr, length);
  if (err != ESP_OK ) {
    ESP_LOGE(TAG, "Failed to write nvs");
    goto cleanup;
  }
  err = nvs_commit(my_handle);
  if (err != ESP_OK ) {
    ESP_LOGE(TAG, "Failed to commit nvs");
    goto cleanup;
  }
  cleanup:
  nvs_close(my_handle);
}
