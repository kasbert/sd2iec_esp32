
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/param.h>
#include <string.h>

#include "lvgl.h"
#include "examples/lv_examples.h"
#include "demos/lv_demos.h"

#include "sdkconfig.h"
#include "display.h"
#include "esp32/esp-display.h"
#include "ui.h"


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_display_t * hal_init(int32_t w, int32_t h);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

uint8_t globalflags;

/**********************
 *  STATIC PROTOTYPES
 **********************/

void display_send_prefixed(uint8_t cmd, uint8_t prefixbyte, uint8_t len, const uint8_t *buffer);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
uint8_t device_address = 8;
uint8_t hardaddress = 8;
uint8_t image_as_dir = 1;

int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

  /*Initialize LVGL*/
  lv_init();

  /*Initialize the HAL (display, input devices, tick) for LVGL*/
  hal_init(480, 480);

  //lv_demo_widgets();
  main_widget();

#if !ESP32
  //ESP_LOGI(TAG, "display_send_prefixed cmd %x prefix %x len %d quelen %d core %d", cmd, prefixbyte, len, uxQueueMessagesWaiting(to_display_queue), xPortGetCoreID());
  display_send_prefixed(DISPLAY_MOUNTED,1,0,0);
#endif


  while(1) {
    /* Periodically call the lv_task handler.
     * It could be done in a timer interrupt or an OS task too.*/
    lv_timer_handler();
    usleep(5 * 1000);
  }

  return 0;
}

char *display_cmd2str(uint8_t cmd) {
  switch (cmd) {
  case SYSTEM_STORE:
    return "SYSTEM_STORE";
  case SYSTEM_CHDIR:
    return "SYSTEM_CHDIR";
  case SYSTEM_CHADDR:
    return "SYSTEM_CHADDR";
  case SYSTEM_DOSCMD:
    return "SYSTEM_DOSCMD";
  case SYSTEM_MOUNT:
    return "SYSTEM_MOUNT";
  case SYSTEM_UNMOUNT:
    return "SYSTEM_UNMOUNT";
  case 42:
    return "TEST STUFF";
  case 69:
    return "FILE TEST";
  }
  return "UNKNOWN";
}


void send_system_message(uint8_t cmd, char* data) {
  system_message msg;
  msg.cmd = cmd;
  if (data) {
    strcpy(msg.data, data);
  } else {
    msg.data[0] = 0;
  }
#if ESP_PLATFORM
#else
  ESP_LOGI("TAG", "Received msg %d %s", (int)msg.cmd, display_cmd2str(msg.cmd));

  switch (msg.cmd) {
  case SYSTEM_CHADDR:
    /* New address selected */
    break;

  case SYSTEM_DOSCMD:
    if (!strcmp(msg.data, "CP1"))
      display_send_prefixed(DISPLAY_CURRENT_PART, 0, 0, 0);
    if (!strcmp(msg.data, "CP2"))
      display_send_prefixed(DISPLAY_CURRENT_PART, 1, 0, 0);
    break;

  case SYSTEM_CHDIR:
    /* New directory selected */
    break;

  case SYSTEM_STORE:
    // Write config to eeprom
    break;

  case SYSTEM_MOUNT:
    display_send_prefixed(DISPLAY_MOUNTED, 0, 0, 0);
    break;

  case SYSTEM_UNMOUNT:
    display_send_prefixed(DISPLAY_UNMOUNTED, 0, 0, 0);
    break;

  default:
  }


#endif

}


void display_send_prefixed(uint8_t cmd, uint8_t prefixbyte, uint8_t len, const uint8_t *buffer) {
  //ESP_LOGI(TAG, "display_send_prefixed cmd %x prefix %x len %d quelen %d core %d", cmd, prefixbyte, len, uxQueueMessagesWaiting(to_display_queue), xPortGetCoreID());
  display_message msg;
  msg.cmd = cmd;
  msg.prefixbyte = prefixbyte;
  len = MIN(sizeof(msg.buffer)-1, (size_t)len);
  msg.len = len;
  memcpy(msg.buffer, buffer, len);
  msg.buffer[len] = 0;

  lv_obj_send_event(lv_scr_act(), MY_EVENT_1, &msg);
}

bool esp32fs_sdcard_ismounted() {
  return true;
}

bool esp32fs_sdcard_mount( char *mount_point) {
  return true;
}
void esp32fs_sdcard_unmount( char *mount_point) {}
bool sdcard_format( char *mount_point) {
  return false;
}

uint64_t esp32fs_get_bytes_free(const char *mount_point) {
  return 1;
}
uint64_t esp32fs_get_bytes_used(const char *mount_point) {
    return 2;
}

const char *esp32fs_sdcard_get_type () { return "FAKE"; }
const char *esp32fs_sdcard_get_name() { return "NAME"; }
uint64_t esp32fs_sdcard_get_size() { return 123; }


/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static lv_display_t * hal_init(int32_t w, int32_t h)
{

  lv_group_set_default(lv_group_create());

  lv_display_t * disp = lv_sdl_window_create(w, h);

  lv_indev_t * mouse = lv_sdl_mouse_create();
  lv_indev_set_group(mouse, lv_group_get_default());
  lv_indev_set_display(mouse, disp);
  lv_display_set_default(disp);

  LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
  lv_obj_t * cursor_obj;
  cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
  lv_image_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
  lv_indev_set_cursor(mouse, cursor_obj);             /*Connect the image  object to the driver*/

  lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
  lv_indev_set_display(mousewheel, disp);
  lv_indev_set_group(mousewheel, lv_group_get_default());

  lv_indev_t * kb = lv_sdl_keyboard_create();
  lv_indev_set_display(kb, disp);
  lv_indev_set_group(kb, lv_group_get_default());

  return disp;
}
