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


   espfs.c: ESP32 FS low level handling

*/

#include "config.h"
#include "esp_log.h"
#include "esp_check.h"
#include <stdio.h>
#include <string.h>

#include "espfs.h"

#include "esp_vfs_fat.h"
#if CONFIG_SD2IEC_USE_SDCARD
#include <sdmmc_cmd.h>
#include <driver/sdmmc_defs.h>
#endif
#if CONFIG_SD2IEC_USE_SPI_PARTITION
#include "driver/sdspi_host.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define TAG "espfs"
 // FIXME use CONFIG_
#define HOST_SLOT SPI2_HOST

#if CONFIG_SD2IEC_USE_SDCARD
static sdmmc_card_t *card; // = &sdmmc_card;
static int host_slot;
#endif
#if CONFIG_SD2IEC_USE_SPI_PARTITION
static wl_handle_t s_wl_handle;
#endif

// Pin assignments can be set in menuconfig, see "SD SPI Configuration" menu.

static void show_disk_free(const char *mount_point);


// Initialize context
bool esp32fs_create(void) {
#if CONFIG_SD2IEC_USE_SDCARD
  host_slot = HOST_SLOT;
  card = 0;
#endif
#if CONFIG_SD2IEC_USE_SPI_PARTITION
  s_wl_handle = WL_INVALID_HANDLE;
#endif
  return true;
}

#if CONFIG_SD2IEC_USE_SPI_PARTITION

bool esp32fs_spiflash_mount(char *mount_point) {
  ESP_LOGI(TAG, "Mounting SPIFLASH FAT filesystem to %s", mount_point);
  const esp_vfs_fat_mount_config_t mount_config = {
      .max_files = 4,
      .format_if_mount_failed = true,
      .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};
  esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(mount_point, "storage",
                                                   &mount_config, &s_wl_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount FAT (%s)", esp_err_to_name(err));
    return false;
  }
  ESP_LOGI(TAG, "Filesystem mounted");
  show_disk_free(mount_point);

  return true;
}

void esp32fs_spiflash_unmount(char *mount_point) {
  if(s_wl_handle) {
    esp_vfs_fat_spiflash_unmount_rw_wl(mount_point, s_wl_handle);
  }
  ESP_LOGI(TAG, "Flash unmounted");
}
#endif

static void show_disk_free(const char *mount_point) {
  uint64_t out_total_bytes;
  uint64_t out_free_bytes;
  int ret = esp_vfs_fat_info(mount_point, &out_total_bytes, &out_free_bytes);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to esp_vfs_fat_info.");
    return;
  }
  ESP_LOGI(TAG, "Total bytes: %lld, free bytes: %lld", out_total_bytes,
           out_free_bytes);
}


// sd card

#if 0
bool esp32fs_sdcard_init() {
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SPI peripheral");
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = CONFIG_SD2IEC_SD_PIN_MOSI,
        .miso_io_num = CONFIG_SD2IEC_SD_PIN_MISO,
        .sclk_io_num = CONFIG_SD2IEC_SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host_slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    ESP_GOTO_ON_ERROR(ret, cleanup, TAG, "Failed to initialize bus.");

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = CONFIG_SD2IEC_SD_PIN_CS;
    slot_config.host_id = host_slot;

    int card_handle = -1;   //uninitialized
    static sdmmc_card_t sdmmc_card;
    card = &sdmmc_card;

    ESP_LOGI(TAG, "init");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = host_slot;
    ret = host.init();
    ESP_GOTO_ON_ERROR(ret, cleanup_spi, TAG, "Host init failed");

    ESP_LOGI(TAG, "init_sdspi_host");

    ret = sdspi_host_init_device(&slot_config, &card_handle);
    ESP_GOTO_ON_ERROR(ret, cleanup, TAG, "sdspi_host_init_device failed.");

    // probe and initialize card
    ESP_LOGI(TAG, "sdmmc_card_init");
    ret = sdmmc_card_init(&host, card);
    ESP_GOTO_ON_ERROR(ret, cleanup_host, TAG, "sdmmc_card_init failed");

    sdmmc_card_print_info(stdout, card);
    return true;

cleanup_host:
    //call_host_deinit(host_config);
    if (host.flags & SDMMC_HOST_FLAG_DEINIT_ARG) {
        host.deinit_p(host.slot);
    } else {
        host.deinit();
    }
cleanup_spi:
    spi_bus_free(host_slot);

cleanup:
  return false;
}

#endif
void esp32fs_sdcard_del() {
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = host_slot;
    if (host.flags & SDMMC_HOST_FLAG_DEINIT_ARG) {
        host.deinit_p(host.slot);
    } else {
        host.deinit();
    }
    spi_bus_free(host_slot);
    card = 0;
}

bool esp32fs_sdcard_mount(char *mount_point) {
    esp_err_t ret;

    if (card) esp32fs_sdcard_del();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = CONFIG_SD2IEC_SD_PIN_MOSI,
        .miso_io_num = CONFIG_SD2IEC_SD_PIN_MISO,
        .sclk_io_num = CONFIG_SD2IEC_SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host_slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return false;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = CONFIG_SD2IEC_SD_PIN_CS;
    //slot_config.gpio_cd = PIN_NUM_CD;
    slot_config.host_id = host_slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_SD2IEC_SD_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = host_slot;
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_SD2IEC_SD_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return false;
    }
    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "Filesystem mounted");
    return true;
}

void esp32fs_sdcard_unmount(char *mount_point) {
  if (card) {
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");

    // deinitialize the bus after all devices are removed
    spi_bus_free(host_slot);
    card = 0;
  }
}

bool esp32fs_sdcard_format(char *mount_point) {
    esp_err_t ret;
    if (!card) {
        return false;
    }
    // Format FATFS
    ret = esp_vfs_fat_sdcard_format(mount_point, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(TAG, "Filesystem formatted");
    return true;
}

bool sdcard_ismounted() {
    return !!card;
}


const char *esp32fs_sdcard_get_type () {
  if (card->is_sdio) {
    return "SDIO";
  } else if (card->is_mmc) {
    return "MMC";
  } else {
    return (card->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC";
  }
}

const char *esp32fs_sdcard_get_name() {
  return card->cid.name;
}

uint64_t esp32fs_sdcard_get_size() {
  return ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024);
}

uint64_t esp32fs_get_bytes_free(const char *mount_point) {
    uint64_t out_total_bytes;
    uint64_t out_free_bytes;
    int ret = esp_vfs_fat_info(mount_point, &out_total_bytes, &out_free_bytes);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to esp_vfs_fat_info.");
      return 0;
    }
    return out_free_bytes;
}

uint64_t esp32fs_get_bytes_used(const char *mount_point) {
    uint64_t out_total_bytes;
    uint64_t out_free_bytes;
    int ret = esp_vfs_fat_info(mount_point, &out_total_bytes, &out_free_bytes);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to esp_vfs_fat_info.");
      return 0;
    }
    return out_total_bytes;
}
