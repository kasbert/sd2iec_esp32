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


   system.c: ESP32 specific routines

*/

#include "config.h"

#include <esp_check.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <string.h>

#include "espfs.h"
#include "cbmdirent.h"
#include "iec-bus.h"
#include "diskio.h"

static const char *TAG = "system";

int32_t arch_timeout;

// Own "watchdog"
volatile uint64_t last_system_sleep;

volatile static char interrupt_happens;

static TaskHandle_t system_task_handle;
static TimerHandle_t led_timer;

#define SYSTEM_STACK_SIZE 4096 * 4
StaticTask_t xTaskBuffer;
StackType_t xStack[SYSTEM_STACK_SIZE];

extern int main();

bool sd2iec_system_init() {
  system_task_handle = xTaskCreateStaticPinnedToCore(
      main, "system", SYSTEM_STACK_SIZE, 0, 24, xStack, &xTaskBuffer, 1);
  return true;
}

static void led_timer_callback(TimerHandle_t arg) {
  if (led_state & LED_ERROR)
    toggle_dirty_led();
  // We don't want to disturb iec running in core 1
  assert(xPortGetCoreID() == 0);
}

void timer_init(void) {
  // No need for systick timer, just blinking leds
  led_timer = xTimerCreate("LedTimer", pdMS_TO_TICKS(200), pdTRUE, (void *)0,
                           led_timer_callback);
  xTimerStart(led_timer, 0);
}


IRAM_ATTR
void disable_interrupts(void) {
  portDISABLE_INTERRUPTS();
}

IRAM_ATTR
void enable_interrupts(void) {
  portENABLE_INTERRUPTS();
}

/* Early system initialisation */
void system_init_early(void) {
  // disable_interrupts();
  return;
}

/* Late initialisation */
void system_init_late(void) {}

/* Reset MCU */
void system_reset(void) {
  ESP_LOGI(TAG, "system_reset");
  uart_flush();
  esp_restart();
  while (1)
    ;
}

void uart_init(void) {
}

void disk_init(void) {
  ESP_LOGI(TAG, "Initialize FS");
  esp32fs_create();
#if CONFIG_SD2IEC_USE_SPI_PARTITION
  ESP_LOGI(TAG, "Mount SPI flash");
  esp32fs_spiflash_mount(SPIMOUNT_POINT);
  // esp32fs_filetest(SPIMOUNT_POINT, "FLASH");
  esp32fs_list_files(SPIMOUNT_POINT);
#endif

#if CONFIG_SD2IEC_USE_SDCARD
  ESP_LOGI(TAG, "Mount SD card");
  // esp32fs_sdcard_init();
  // esp32fs_sdcard_unmount(SDMOUNT_POINT);
  esp32fs_sdcard_mount(SDMOUNT_POINT);
  esp32fs_list_files(SDMOUNT_POINT);
#endif
}

void i2c_init(void) {
  ESP_LOGI(TAG, "No i2c_init"); // Not used here
}

void set_changelist(path_t *path, uint8_t *filename) {
  ESP_LOGE(TAG, "FIXME set_changelist");
}

void change_init(void) {
  ESP_LOGE(TAG, "FIXME change_init");
}

void change_disk(void) {
  ESP_LOGE(TAG, "FIXME change_disk");
}

volatile enum diskstates disk_state = DISK_OK;

/**
 * sd_getinfo - read card information
 * @drv   : drive
 * @page  : information page
 * @buffer: target buffer
 *
 * This function returns the requested information page @page
 * for card @drv in the buffer @buffer. Currently only page
 * 0 is supported which is the diskinfo0_t structure defined
 * in diskio.h. Returns a DRESULT to indicate success/failure.
 */
DRESULT esp32_getinfo(uint8_t drv, uint8_t page, void *buffer) {
#if 1
  uint32_t capacity;
  // TODO
  capacity = 1;
#else
  uint8_t buf[18];
  uint8_t res;
  uint32_t capacity;

  if (drv >= MAX_CARDS)
    return RES_NOTRDY;

  if (sd_status(drv) & STA_NODISK)
    return RES_NOTRDY;

  if (page != 0)
    return RES_ERROR;

  /* Try to calculate the total number of sectors on the card */
  if (send_command(drv, SEND_CSD, 0) != 0) {
    deselect_card();
    return RES_ERROR;
  }

  /* Wait for data token */
  // FIXME: Timeout?
  do {
    res = spi_rx_byte();
  } while (res == 0);

  spi_rx_block(buf, 18);
  deselect_card();

  if (cardtype[drv] & CARD_SDHC) {
    /* Special CSD for SDHC cards */
    capacity = (1 + getbits(buf,127-69,22)) * 1024;
  } else {
    /* Assume that MMC-CSD 1.0/1.1/1.2 and SD-CSD 1.1 are the same... */
    uint8_t exponent = 2 + getbits(buf, 127-49, 3);
    capacity = 1 + getbits(buf, 127-73, 12);
    exponent += getbits(buf, 127-83,4) - 9;
    while (exponent--) capacity *= 2;
  }

#endif
  diskinfo0_t *di = buffer;
  di->validbytes  = sizeof(diskinfo0_t);
  di->disktype    = DISK_TYPE_SD;
  di->sectorsize  = 2; // TODO
  di->sectorcount = capacity;
  return RES_OK;
}
DRESULT disk_getinfo(BYTE drv, BYTE page, void *buffer) __attribute__ ((weak, alias("esp32_getinfo")));



IRAM_ATTR
void system_pin_intr_handler() {
  // Called from ISR
  interrupt_happens++;
  //uart_putc('&');
  // Notify mainloop in system_sleep()
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveIndexedFromISR(system_task_handle, 0, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}


void system_sleep(void) {
  /* TODO check keys
        while (!key_pressed(KEY_SLEEP))
        while (IEC_ATN) {
  */
  uart_putc('<');
  last_system_sleep = esp_timer_get_time();
  while (!interrupt_happens && IEC_ATN) {
    // Wait for gpio interrupt
    uint32_t ulNotificationValue =
        ulTaskNotifyTakeIndexed(0, pdTRUE, pdMS_TO_TICKS(1000));
    last_system_sleep = esp_timer_get_time();
    if (ulNotificationValue) {
      break;
    }
  }
  uart_putc('>');
  // uart_putcrlf();
  interrupt_happens = 0;
  return;
}

// test utility

#include <dirent.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#define EXAMPLE_MAX_CHAR_SIZE    64

static esp_err_t s_example_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

void esp32fs_filetest(char *mount_point, const char *txt) {
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    char file_hello[256];
    snprintf(file_hello, sizeof(file_hello), "%s/%s", mount_point, "/hello.txt");
    char data[EXAMPLE_MAX_CHAR_SIZE];
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", txt);
    esp_err_t ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK) {
        return;
    }

    char file_foo[256];
    snprintf(file_foo, sizeof(file_foo), "%s/%s", mount_point, "/foo.txt");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // Delete it if it exists
        unlink(file_foo);
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    ret = s_example_read_file(file_foo);
    if (ret != ESP_OK) {
        return;
    }

    if (stat(file_foo, &st) == 0) {
        ESP_LOGI(TAG, "file still exists");
        //return;
    } else {
        ESP_LOGI(TAG, "file doesnt exist, format done");
    }

    char file_nihao[256];
    snprintf(file_nihao, sizeof(file_nihao), "%s/%s", mount_point, "/nihao.txt");
    memset(data, 0, EXAMPLE_MAX_CHAR_SIZE);
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", txt);
    ret = s_example_write_file(file_nihao, data);
    if (ret != ESP_OK) {
        return;
    }

    //Open file for reading
    ret = s_example_read_file(file_nihao);
    if (ret != ESP_OK) {
        return;
    }

    esp32fs_list_files(mount_point);
}

void esp32fs_list_files(char *path) {
  DIR *dir;
  struct dirent *ent;
  ESP_LOGI(TAG, "DIR------------ %s ", path);
  if ((dir = opendir(path)) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL) {
      ESP_LOGI(TAG, " %s", ent->d_name);
    }
    closedir(dir);
  } else {
    ESP_LOGE(TAG, "Cannot open %s", path);
    // return EXIT_FAILURE;
  }
  ESP_LOGI(TAG, "------------");
}

////

