#include "sdkconfig.h"
#if ESP_PLATFORM
#include "esp_log.h"
#include "esp_system.h"
#endif

#include <stdio.h>

#include "display.h"
#include "esp32/esp-display.h"
#include "esp32/espfs.h"
#include "ui.h"
#include "ui_common.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "gui"
/**********************
 *  STATIC PROTOTYPES
 **********************/

// Font
#include "chargen.c"

static lv_obj_t *tv;

lv_style_t style_text_muted;

lv_style_t style_condensed;

// static lv_obj_t * calendar;
/*
static lv_style_t style_lb;
static lv_style_t style_inv_lb;
*/

/*
static lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;
*/

/*
static const lv_font_t *font_large;
static const lv_font_t *font_normal;
*/

// static lv_timer_t *meter2_timer;

uint32_t BL_EVENT_1;
#if ESP_PLATFORM
#else
uint32_t MY_EVENT_1;
#endif

bool ui_is_status_tab() { return lv_tabview_get_tab_active(tv) == 0; }

// UI functions


#if ESP_PLATFORM
#else
// Simulate system messaging
static void msg_event_cb(lv_event_t *e) {
  void *param = lv_event_get_param(e);
  ESP_LOGI(TAG, "EVENT ! %p", param);
  display_message *msg = param;
  ESP_LOGI(TAG, "display_message ! %d", msg->cmd);
  esp_display_event(msg->cmd, msg->prefixbyte, msg->len, msg->buffer);
}
#endif

void main_widget() {
  lv_disp_t *dispp = lv_disp_get_default();
  lv_theme_t *theme = lv_theme_default_init(
      dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
      true, LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);

  lv_style_init(&style_text_muted);
  lv_style_set_text_opa(&style_text_muted, LV_OPA_50);

  lv_style_init(&style_condensed);
  lv_style_set_pad_top(&style_condensed, 0);
  lv_style_set_pad_bottom(&style_condensed, 0);
  lv_style_set_pad_left(&style_condensed, 0);
  lv_style_set_pad_right(&style_condensed, 0);

  lv_obj_t *container = lv_obj_create(lv_scr_act());
  // lv_obj_t *container = lv_obj_create(t0);
  lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
  lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE); /// Flags
  lv_obj_set_style_bg_color(container, lv_color_hex(0x000000),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(container, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_style_text_font(lv_scr_act(), LV_FONT_DEFAULT, 0);

  lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  tv = lv_tabview_create(container); // LV_DIR_TOP
  lv_tabview_set_tab_bar_size(tv, 45);
  lv_obj_set_y(tv, 30);

  lv_obj_t *t0 = lv_tabview_add_tab(tv, "Status");
  lv_obj_t *t1 = lv_tabview_add_tab(tv, "Files");
  lv_obj_t *t3 = lv_tabview_add_tab(tv, "Config");
  lv_obj_t *t2 = lv_tabview_add_tab(tv, "About");
  ui_status(t0);
  ui_about(t2);
  ui_files2(t1);
  ui_config(t3);

  update_mount_status();

  add_status_message("SD2IEC ESP32 ok,0,0");

  BL_EVENT_1 = lv_event_register_id();
#if ESP_PLATFORM
#else
  MY_EVENT_1 = lv_event_register_id();
  lv_obj_add_event_cb(lv_scr_act(), msg_event_cb, MY_EVENT_1, 0);
#endif
}

uint8_t esp_display_event(uint8_t cmd, uint8_t prefixbyte, uint8_t length,
                          const uint8_t *buffer) {
  ESP_LOGI(TAG, "display_message received prefix %d cmd %s(%d) len %d",
           prefixbyte, display_cmd2str(cmd), cmd, length);
#if ESP_PLATFORM
  ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, length, ESP_LOG_INFO);
#endif
  switch (cmd) {
  case DISPLAY_INIT:
    add_status_message("init");
    break;

  case DISPLAY_ADDRESS:
    add_status_message("device %d", prefixbyte);
    break;

  case DISPLAY_FILENAME_READ:
    add_status_message("load %d:%s", prefixbyte, buffer);
    ui_files_show_current(buffer);
    break;

  case DISPLAY_FILENAME_WRITE:
    add_status_message("save %d:%s", prefixbyte, buffer);
    ui_files_show_current(buffer);
    break;

  case DISPLAY_DOSCOMMAND:
    add_status_message("DOSCMD %s", buffer);
    break;

  case DISPLAY_ERRORCHANNEL:
    add_status_message("%s", buffer);
    break;

  case DISPLAY_CURRENT_DIR:
    add_status_message("current part %d dir %s", prefixbyte, buffer);
    ui_files_show_current(buffer);
    break;

  case DISPLAY_CURRENT_PART:
    add_status_message("current part %d", prefixbyte);
    ui_status_set_partition(prefixbyte);
    if (prefixbyte == 0) {
      ui_files_open_dir(SDMOUNT_POINT);
    } else if (prefixbyte == 1) {
      ui_files_open_dir(SPIMOUNT_POINT);
    }
    break;

  case DISPLAY_MOUNTED:
    update_mount_status();
    break;

  case DISPLAY_UNMOUNTED:
    update_mount_status();
    break;
  }

  return 0;
}
