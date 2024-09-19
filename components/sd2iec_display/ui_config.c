
#include "sdkconfig.h"
#if ESP_PLATFORM
#include "esp_log.h"
#include "esp_system.h"
#include <esp_check.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bus.h"
#include "esp32/esp-display.h"
#include "esp32/espfs.h"
#include "flags.h"

#include "ui.h"
#include "ui_common.h"

#define TAG "gui"

lv_obj_t *sdcard_mount_label;

uint8_t backlight_percent = 50;

// static lv_obj_t *spinbox;
static lv_obj_t *drive_number_label;
static lv_obj_t *mbox1;
static lv_obj_t *slider_label;

static void mount_event_handler(lv_event_t *e) {

  if (!esp32fs_sdcard_ismounted()) {
    ESP_LOGI(TAG, "Mounting SD card");
    send_system_message(SYSTEM_MOUNT, 0);
    // esp32fs_sdcard_mount(, SDMOUNT_POINT);
  } else {
    ESP_LOGI(TAG, "Unmounting SD card");
    send_system_message(SYSTEM_UNMOUNT, 0);
    // esp32fs_sdcard_unmount(, SDMOUNT_POINT);
  }
  // update_mount_status(ctx);
}

static void dialog_yes_event_handler(lv_event_t *e) {
  lv_msgbox_close(mbox1);
  // FIXME SEND EVENT
  if (!esp32fs_sdcard_ismounted()) {
    ESP_LOGI(TAG, "Mounting SD card");
    esp32fs_sdcard_mount(SDMOUNT_POINT);
  }
  ESP_LOGI(TAG, "Formatting SD card");
  // FIXME esp32fs_sdcard_format(SDMOUNT_POINT);
  /*
  browser_set_current_path(SDMOUNT_POINT);
  browser_show_dir();
  */
  lv_label_set_text(sdcard_mount_label, "Unmount");
}

static void dialog_no_event_handler(lv_event_t *e) { lv_msgbox_close(mbox1); }

static void format_event_handler(lv_event_t *e) {
  mbox1 = lv_msgbox_create(NULL);

  lv_msgbox_add_title(mbox1, "Are you sure?");

  lv_msgbox_add_text(mbox1, "Formatting will wipe out all files");
  lv_msgbox_add_close_button(mbox1);

  lv_obj_t *btn;
  btn = lv_msgbox_add_footer_button(mbox1, "Nope");
  lv_obj_add_event_cb(btn, dialog_yes_event_handler, LV_EVENT_CLICKED, 0);
  btn = lv_msgbox_add_footer_button(mbox1, "No");
  lv_obj_add_event_cb(btn, dialog_no_event_handler, LV_EVENT_CLICKED, 0);
}

static void lv_spinbox_increment_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    // lv_spinbox_increment(spinbox);
    device_address++;
    if (device_address > 15)
      device_address = 8;
    lv_label_set_text_fmt(drive_number_label, "%d", device_address);
    ui_status_update_device_address();
  }
}

static void lv_spinbox_decrement_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    // lv_spinbox_decrement(spinbox);
    device_address--;
    if (device_address < 8)
      device_address = 15;
    lv_label_set_text_fmt(drive_number_label, "%d", device_address);
    ui_status_update_device_address();
  }
}

#if 0
static void config_create(lv_obj_t *parent) {
  lv_obj_t *panel1 = lv_obj_create(parent);
  lv_obj_set_height(panel1, LV_SIZE_CONTENT);

  // sd device number 8-14
  // internal flash device number 8-14
  // net settings ?

  // UNMOUNT
  // MOUNT
  // FORMAT
  // sdinfo

  lv_obj_t *name = lv_label_create(panel1);
  lv_label_set_text(name, "Elena Smith");
  lv_obj_add_style(name, &style_title, 0);

  spinbox = lv_spinbox_create(parent);
  lv_spinbox_set_range(spinbox, 8, 12);
  lv_spinbox_set_digit_format(spinbox, 1, 1);
  lv_spinbox_step_prev(spinbox);
  lv_obj_set_width(spinbox, 100);
  lv_obj_align(spinbox, LV_ALIGN_CENTER, 0, 0);

  lv_coord_t h = lv_obj_get_height(spinbox);
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, h, h);
  lv_obj_align(btn, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  // lv_theme_apply(btn, LV_THEME_SPINBOX_BTN);
  // lv_obj_set_style_local_value_str(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
  // LV_SYMBOL_PLUS);
  lv_obj_add_event_cb(btn, lv_spinbox_increment_event_cb, LV_EVENT_CLICKED,
                      NULL);

  btn = lv_btn_create(parent);
  lv_obj_align(btn, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_add_event_cb(btn, lv_spinbox_decrement_event_cb, LV_EVENT_CLICKED,
                      NULL);
  // lv_obj_set_style_local_value_str(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
  // LV_SYMBOL_MINUS);

  lv_obj_t *dsc = lv_label_create(panel1);
  lv_obj_add_style(dsc, &style_text_muted, 0);
  lv_label_set_text(
      dsc, "This is a short description of me. Take a look at my profile!");
  lv_label_set_long_mode(dsc, LV_LABEL_LONG_WRAP);

  lv_obj_t *email_label = lv_label_create(panel1);
  lv_label_set_text(email_label, "elena@smith.com");
}
#endif

static void test_event_handler(lv_event_t *e) {
  ESP_LOGI(TAG, "Testing SD card file operations");
  send_system_message(69, 0);
}

static void globalflags_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  uint32_t flag = (uint32_t)lv_event_get_user_data(e);
  ESP_LOGI(TAG, "%d State: %s %ld\n", code,
           lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off", (long)flag);
  if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
    globalflags |= flag;
  } else {
    globalflags &= ~flag;
  }
}

static void slider_event_cb(lv_event_t *e) {
  lv_obj_t *slider = lv_event_get_target(e);
  backlight_percent = lv_slider_get_value(slider);
  lv_label_set_text_fmt(slider_label, "%d%%", (int)backlight_percent);
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  display_message msg;
  msg.cmd = 0;
  msg.prefixbyte = backlight_percent;
  msg.len = 0;
  msg.buffer[0] = 0;
  lv_obj_send_event(lv_scr_act(), BL_EVENT_1, &msg);
}

lv_style_t style_config_item;

void ui_config(lv_obj_t *container) {

  lv_style_init(&style_config_item);
  lv_style_set_pad_ver(&style_config_item, 0);
  // lv_style_set_bg_opa(&style_config_item, LV_OPA_COVER);
  // lv_style_set_bg_color(&style_config_item, lv_color_black());
  lv_style_set_border_width(&style_config_item, 1);

  lv_style_set_width(&style_config_item, lv_pct(100));
  lv_style_set_height(&style_config_item, LV_SIZE_CONTENT);
  // lv_style_set_flex_flow(&style_config_item, LV_FLEX_FLOW_ROW);
  lv_style_set_pad_column(&style_config_item, 10);
  lv_style_set_text_align(&style_config_item, LV_TEXT_ALIGN_RIGHT);

  lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_text_font(container, &lv_font_montserrat_20, 0);

  {
    lv_obj_t *cont2 = lv_obj_create(container);
    lv_obj_remove_style_all(cont2);
    // lv_obj_set_size(cont, 300, 220);
    lv_obj_set_width(cont2, lv_pct(100));
    lv_obj_set_height(cont2, LV_SIZE_CONTENT);
    lv_obj_center(cont2);
    lv_obj_set_flex_flow(cont2, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(cont2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_column(cont2, 10, 0);
    lv_obj_set_style_pad_row(cont2, 10, 0);

    // lv_example_spinbox_1(cont2);

    lv_obj_t *mount_btn = lv_btn_create(cont2);
    lv_obj_set_height(mount_btn, LV_SIZE_CONTENT);
    lv_obj_set_width(mount_btn, LV_PCT(30));
    sdcard_mount_label = lv_label_create(mount_btn);
    lv_label_set_text(sdcard_mount_label, "Mount"); // or Unmount
    // lv_obj_set_width(sdcard_mount_label, 100);
    lv_obj_center(sdcard_mount_label);
    lv_obj_add_event_cb(mount_btn, mount_event_handler, LV_EVENT_CLICKED, 0);

    lv_obj_t *format_btn = lv_btn_create(cont2);
    lv_obj_set_height(format_btn, LV_SIZE_CONTENT);
    lv_obj_set_width(format_btn, LV_PCT(30));
    lv_obj_t *format_label = lv_label_create(format_btn);
    lv_label_set_text(format_label, "Format");
    lv_obj_center(format_label);
    lv_obj_add_event_cb(format_btn, format_event_handler, LV_EVENT_CLICKED, 0);

    lv_obj_t *test_btn = lv_btn_create(cont2);
    lv_obj_set_height(test_btn, LV_SIZE_CONTENT);
    lv_obj_set_width(test_btn, LV_PCT(30));
    lv_obj_t *test_label = lv_label_create(test_btn);
    lv_label_set_text(test_label, "Test");
    lv_obj_center(test_label);
    lv_obj_add_event_cb(test_btn, test_event_handler, LV_EVENT_CLICKED, 0);
  }

  // sdmmc_card_info(parent, card);
  {
    lv_obj_t *cont1 = lv_obj_create(container);
    // lv_obj_set_size(cont, 300, 220);
    lv_obj_remove_style_all(cont1);
    lv_obj_add_style(cont1, &style_config_item, 0);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(cont1, LV_OBJ_FLAG_SCROLLABLE);

    /*
    spinbox = lv_spinbox_create(parent);
    lv_spinbox_set_range(spinbox, 8, 15);
    lv_spinbox_set_digit_format(spinbox, 2, 2);
    lv_spinbox_step_prev(spinbox);
    lv_obj_set_width(spinbox, 50);
    lv_obj_center(spinbox);

    lv_coord_t h = lv_obj_get_height(spinbox);
    */

    lv_obj_t *label = lv_label_create(cont1);
    lv_label_set_text(label, "Device address");
    lv_obj_set_width(label, 240);
    lv_obj_set_style_margin_top(
        label, (32 - lv_font_montserrat_20.line_height) / 2, 0);

    lv_obj_t *minus_btn = lv_btn_create(cont1);
    lv_obj_set_style_bg_img_src(minus_btn, LV_SYMBOL_MINUS, 0);
    lv_obj_set_width(minus_btn, 30);
    lv_obj_set_height(minus_btn, 30);

    lv_obj_t *plus_btn = lv_btn_create(cont1);
    lv_obj_set_style_bg_img_src(plus_btn, LV_SYMBOL_PLUS, 0);
    lv_obj_set_width(plus_btn, 30);
    lv_obj_set_height(plus_btn, 30);

    drive_number_label = lv_label_create(cont1);
    lv_obj_set_width(drive_number_label, 40);
    lv_label_set_text_fmt(drive_number_label, "%d", device_address);
    lv_obj_set_style_margin_top(
        drive_number_label, (32 - lv_font_montserrat_20.line_height) / 2, 0);

    // lv_obj_set_size(btn, h, h);
    // lv_obj_align_to(btn, spinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_align_to(plus_btn, drive_number_label, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_add_event_cb(plus_btn, lv_spinbox_increment_event_cb, LV_EVENT_ALL,
                        NULL);

    // lv_obj_set_size(btn, h, h);
    // lv_obj_align_to(btn, spinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_align_to(minus_btn, drive_number_label, LV_ALIGN_OUT_LEFT_MID, -5,
                    0);
    lv_obj_add_event_cb(minus_btn, lv_spinbox_decrement_event_cb, LV_EVENT_ALL,
                        NULL);
  }
#if 0
  lv_obj_t *mount_btn = lv_btn_create(container);
  lv_obj_set_height(mount_btn, LV_SIZE_CONTENT);
  lv_obj_t *label = lv_label_create(mount_btn);
  lv_label_set_text(label, "Mount"); // or Unmount
  lv_obj_center(label);
  lv_obj_add_event_cb(mount_btn, mount_event_handler, LV_EVENT_CLICKED, NULL);
#endif

  const char *labels[] = {"VIC-20 mode", //"AUTOSWAP_ACTIVE", "SWAPLIST_ASCII",
                          "EXTENSION_HIDING", "POSTMATCH"};
  const int flags[] = {VC20MODE, // AUTOSWAP_ACTIVE, SWAPLIST_ASCII,
                       EXTENSION_HIDING, POSTMATCH};
  for (int i = 0; i < sizeof(flags) / sizeof(int); i++) {
    lv_obj_t *label, *sw;
    lv_obj_t *cont1 = lv_obj_create(container);
    lv_obj_remove_style_all(cont1);
    lv_obj_add_style(cont1, &style_config_item, 0);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(cont1, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(cont1);
    lv_label_set_text(label, labels[i]);
    lv_obj_set_width(label, 300);
    // lv_obj_set_style_margin_right(label, 16, 0);
    // lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_margin_top(
        label, (32 - lv_font_montserrat_20.line_height) / 2, 0);

    sw = lv_switch_create(cont1);
    if (globalflags & flags[i])
      lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, globalflags_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void *)flags[i]);
  }

#if 0
#define VC20MODE (1 << 0)
#define AUTOSWAP_ACTIVE (1 << 2)
#define SWAPLIST_ASCII (1 << 5)

/* permanent (EEPROM-saved) flags */
/* 1<<1 was JIFFY_ENABLED */
#define EXTENSION_HIDING (1 << 3)
#define POSTMATCH (1 << 4)

/* Disk image-as-directory mode, defined in fileops.c */
extern uint8_t image_as_dir;

#define IMAGE_DIR_NORMAL 0
#define IMAGE_DIR_DIR 1
#define IMAGE_DIR_BOTH 2


  lv_obj_t *format_btn = lv_btn_create(container);
  lv_obj_set_height(format_btn, LV_SIZE_CONTENT);
  label = lv_label_create(format_btn);
  lv_label_set_text(label, "Format");
  lv_obj_center(label);
  lv_obj_add_event_cb(format_btn, format_event_handler, LV_EVENT_CLICKED, NULL);
#endif

#if 0

    obj = lv_table_create(parent);
    lv_table_set_cell_value(obj, 0, 0, "00");
    lv_table_set_cell_value(obj, 0, 1, "01");
    lv_table_set_cell_value(obj, 1, 0, "10");
    lv_table_set_cell_value(obj, 1, 1, "11");
    lv_table_set_cell_value(obj, 2, 0, "20");
    lv_table_set_cell_value(obj, 2, 1, "21");
    lv_table_set_cell_value(obj, 3, 0, "30");
    lv_table_set_cell_value(obj, 3, 1, "31");
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    obj = lv_calendar_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    obj = lv_btnmatrix_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    obj = lv_checkbox_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    obj = lv_slider_create(parent);
    lv_slider_set_range(obj, 0, 10);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    obj = lv_switch_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    obj = lv_spinbox_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    obj = lv_dropdown_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    obj = lv_roller_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
#endif

  {
    // brightness slider
    lv_obj_t *label = lv_label_create(container);
    lv_label_set_text(label, "Backlight");
    lv_obj_t *slider = lv_slider_create(container);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, backlight_percent, false);
    lv_obj_center(slider);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_set_style_anim_duration(slider, 2000, 0);
    /*Create a label below the slider*/
    slider_label = lv_label_create(container);
    lv_label_set_text_fmt(slider_label, "%d%%", (int)backlight_percent);

    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
  }
}

void update_mount_status() {
  if (esp32fs_sdcard_ismounted()) {
    lv_label_set_text(sdcard_mount_label, "Unmount");
    // esp32fs_list_files(SDMOUNT_POINT);
    /*
    browser_set_current_path(SDMOUNT_POINT);
    browser_show_dir();
    */
  } else {
    lv_label_set_text(sdcard_mount_label, "Mount");
    /*
    browser_set_current_path("/");
    browser_show_dir();
    */
  }
  sdmmc_card_info(SDMOUNT_POINT);
}
