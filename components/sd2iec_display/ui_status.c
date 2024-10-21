#include "sdkconfig.h"
#if ESP_PLATFORM
#include "esp_log.h"
#include "esp_system.h"
#include <esp_check.h>
// #include "sdmmc_cmd.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"
#include "ui_common.h"

#include "bus.h"
#include "esp32/esp-display.h"
#include "esp32/espfs.h"

#include "micro_sd_card.c"
#include "processor.c"

#define TAG "gui"

static lv_style_t style_part_active;
static lv_style_t style_part_inactive;

static lv_obj_t *status_label1 = 0;
static lv_obj_t *status_label2;
static lv_obj_t *sdcard_label;
static lv_obj_t *flash_label;
static lv_obj_t *sdcard_box;
static lv_obj_t *flash_box;
static lv_obj_t *drive_number_label1;

static void spiflash_info(lv_obj_t *label, const char *mount_point);

// Picture
#include "c1541.c"

static void sdcard_event_cb(lv_event_t *event) {
  send_system_message(SYSTEM_DOSCMD, "CP1");
}

static void flash_event_cb(lv_event_t *event) {
  send_system_message(SYSTEM_DOSCMD, "CP2");
}

void ui_status(lv_obj_t *container) {
  add_status_message(" ");
  add_status_message(" ");
  add_status_message(" ");
  add_status_message(" ");
  add_status_message(" ");
  /*
  lv_style_init(&style_lb);
  lv_style_set_bg_color(&style_lb, lv_color_black());
  lv_style_set_bg_opa(&style_lb, LV_OPA_COVER);
  lv_style_set_text_color(&style_lb, lv_color_white());
  lv_style_set_text_opa(&style_lb, LV_OPA_100);

  lv_style_init(&style_inv_lb);
  lv_style_set_bg_color(&style_inv_lb, lv_color_white());
  lv_style_set_bg_opa(&style_inv_lb, LV_OPA_COVER);
  lv_style_set_text_color(&style_inv_lb, lv_color_black());
  lv_style_set_text_opa(&style_inv_lb, LV_OPA_100);
  */

  lv_style_init(&style_part_inactive);
  /*
  lv_style_set_border_width(&style_part_inactive, 2);
  lv_style_set_border_color(&style_part_inactive, lv_color_black());
  lv_style_set_radius(&style_part_inactive, 5);
  */
  lv_style_set_bg_color(&style_part_inactive, lv_color_black());

  lv_style_init(&style_part_active);
  //    lv_style_set_text_font(&style_part_active, &my_font);

  lv_style_set_bg_color(&style_part_active, lv_color_make(10, 10, 10));
  lv_style_set_bg_opa(&style_part_active, LV_OPA_COVER);
  /*
  lv_style_set_text_color(&style_part_active, lv_color_white());
  lv_style_set_text_opa(&style_part_active, LV_OPA_100);
  */
  lv_style_set_border_width(&style_part_active, 2);
  lv_style_set_border_color(&style_part_active, lv_color_white());
  lv_style_set_radius(&style_part_active, 5);

  // lv_obj_t *container = lv_obj_create(tab);
  lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
  // lv_style_set_pad_row(&container, 0);

  {
    // Drive pic + drive number
    lv_obj_t *cont0 = lv_obj_create(container);
    lv_obj_remove_style_all(cont0);
    // lv_obj_set_size(cont, 300, 220);
    lv_obj_set_width(cont0, lv_pct(100));
    lv_obj_set_height(cont0, LV_SIZE_CONTENT);
    // lv_obj_center(cont0);
    lv_obj_set_flex_flow(cont0, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(cont0, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_image_create(cont0);
    lv_image_set_src(icon, &c1541);
    lv_obj_remove_flag(icon, LV_OBJ_FLAG_SCROLLABLE);

    drive_number_label1 = lv_label_create(cont0);
    lv_obj_set_style_text_font(drive_number_label1, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(drive_number_label1, "%d", device_address);
  }

  {
    // Scrolling status texts with chargen font
    lv_obj_t *cont3 = lv_obj_create(container);
    lv_obj_remove_style_all(cont3);
    // lv_obj_set_size(cont, 300, 220);
    lv_obj_set_width(cont3, lv_pct(100));
    lv_obj_set_height(cont3, LV_SIZE_CONTENT);
    // lv_obj_center(cont0);
    lv_obj_set_flex_flow(cont3, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(cont3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(cont3, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(cont3, 0, LV_PART_MAIN);

    status_label2 = lv_label_create(cont3);
    lv_obj_remove_style_all(status_label2);
    lv_obj_set_width(status_label2, lv_pct(100));
    lv_obj_set_style_text_font(status_label2, &chargen_font_sparse,
                               LV_PART_MAIN);
    lv_obj_add_style(status_label2, &style_text_muted, LV_PART_MAIN);
    lv_obj_add_style(status_label2, &style_condensed, LV_PART_MAIN);

    status_label1 = lv_label_create(cont3);
    lv_obj_remove_style_all(status_label1);
    lv_obj_set_width(status_label1, lv_pct(100));
    lv_obj_set_style_text_font(status_label1, &chargen_font_sparse,
                               LV_PART_MAIN);
  }
  /*
      lv_obj_t *test2_btn = lv_btn_create(container);
      lv_obj_set_height(test2_btn, LV_SIZE_CONTENT);
      //lv_obj_set_width(test2_btn, LV_PCT(33));
      lv_obj_set_width(test2_btn, LV_SIZE_CONTENT);
      lv_obj_t *test2_label = lv_label_create(test2_btn);
      lv_label_set_text(test2_label, "Test2");
      lv_obj_center(test2_label);
      lv_obj_add_event_cb(test2_btn, test_system_event_handler,
     LV_EVENT_CLICKED, 0);
  */

  {
    // SDCARD and flash boxes
    lv_obj_t *cont1 = lv_obj_create(container);
    lv_obj_remove_style_all(cont1);
    // lv_obj_set_size(cont, 300, 220);
    lv_obj_set_width(cont1, lv_pct(100));
    lv_obj_set_height(cont1, LV_SIZE_CONTENT);
    lv_obj_center(cont1);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(cont1, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_pad_all(cont1, 0, 0);

    // lv_obj_set_style_bg_color(cont1, lv_color_hex(0x000000), LV_PART_MAIN);

    sdcard_box = lv_obj_create(cont1);
    lv_obj_set_flex_flow(sdcard_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_width(sdcard_box, lv_pct(50));
    lv_obj_add_event_cb(sdcard_box, sdcard_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(sdcard_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(sdcard_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sdcard_icon = lv_image_create(sdcard_box);
    lv_image_set_src(sdcard_icon, &micro_sd_card);
    lv_obj_set_style_image_recolor_opa(sdcard_icon, LV_OPA_100, 0);
    lv_obj_set_style_image_recolor(sdcard_icon, lv_color_white(), 0);

    sdcard_label = lv_label_create(sdcard_box);
    lv_obj_remove_style_all(sdcard_label);
    lv_obj_set_width(sdcard_label, lv_pct(60));
    lv_label_set_text_fmt(sdcard_label, "Initializing");
    // lv_obj_set_style_text_font(sdcard_label, &my_font, 0);

    flash_box = lv_obj_create(cont1);
    lv_obj_set_flex_flow(flash_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_width(flash_box, lv_pct(50));
    lv_obj_add_event_cb(flash_box, flash_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(flash_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(flash_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *flash_icon = lv_image_create(flash_box);
    lv_image_set_src(flash_icon, &processor);
    lv_obj_set_style_image_recolor_opa(flash_icon, LV_OPA_100, 0);
    lv_obj_set_style_image_recolor(flash_icon, lv_color_white(), 0);

    flash_label = lv_label_create(flash_box);
    lv_obj_remove_style_all(flash_label);
    lv_obj_set_width(flash_label, lv_pct(60));
    lv_label_set_text_fmt(flash_label, "Initializing");
  }

  spiflash_info(flash_label, SPIMOUNT_POINT);
}

void ui_status_update_device_address() {
  lv_label_set_text_fmt(drive_number_label1, "%d", device_address);
}

#define MAX_STATUS_MESSAGES 5
__attribute__((format(printf, 1, 2))) void add_status_message(const char *fmt,
                                                              ...) {
static char *status_texts[MAX_STATUS_MESSAGES] = {0};
static char *status_all = 0;

  char buffer[80];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  ESP_LOGI(TAG, "Status: '%s'", buffer);

  if (status_texts[MAX_STATUS_MESSAGES - 1]) {
    free(status_texts[MAX_STATUS_MESSAGES - 1]);
  }
  for (int i = MAX_STATUS_MESSAGES - 1; i > 0; i--) {
    status_texts[i] = status_texts[i - 1];
  }
  status_texts[0] = malloc(strlen(buffer) + 1);
  strcpy(status_texts[0], buffer);

  int text_len = 1;
  for (int i = 1; i < MAX_STATUS_MESSAGES; i++) {
    text_len += status_texts[i] ? (strlen(status_texts[i]) + 1) : 1;
  }
  if (status_all)
    free(status_all);
  status_all = malloc(text_len);
  status_all[0] = 0;
  for (int i = MAX_STATUS_MESSAGES - 1; i > 0; i--) {
    if (status_texts[i]) {
      strcat(status_all, status_texts[i]);
    }
    strcat(status_all, "\n");
  }
  if (status_label1) {
    lv_label_set_text_static(status_label1, status_texts[0]);
    lv_label_set_text_static(status_label2, status_all);
  }
}

static void spiflash_info(lv_obj_t *label, const char *mount_point) {
  uint64_t out_total_bytes = esp32fs_get_bytes_used(mount_point);
  uint64_t out_free_bytes = esp32fs_get_bytes_free(mount_point);
  ESP_LOGI(TAG, "%s Total bytes: %lld, free bytes: %lld", mount_point,
           (long long)out_total_bytes, (long long)out_free_bytes);
  lv_label_set_text_fmt(label,
                        "Internal Flash\nTotal bytes: %lld\n"
                        "Free: %d%%",
                        //"Free bytes: %lld",
                        (long long)out_total_bytes, 
                        (int)(out_free_bytes * 100 / out_total_bytes)
                        //(long long)out_free_bytes
                        );
}

void sdmmc_card_info(const char *mount_point) {
  if (!esp32fs_sdcard_ismounted()) {
    lv_label_set_text(sdcard_label, "No card mounted\n");
    return;
  }
  uint64_t out_total_bytes = esp32fs_get_bytes_used(mount_point);
  uint64_t out_free_bytes = esp32fs_get_bytes_free(mount_point);
  ESP_LOGI(TAG, "%s Total bytes: %lld, free bytes: %lld", mount_point,
           (long long)out_total_bytes, (long long)out_free_bytes);
  lv_label_set_text_fmt(
      sdcard_label,
      "Name: %s\nType: %s\nSize: %lluMB\n"
      "Free: %d%%",
      //"Total bytes: %lld\nFree bytes: %lld",
      esp32fs_sdcard_get_name(), esp32fs_sdcard_get_type(),
      (long long)esp32fs_sdcard_get_size(),
      (int)(out_free_bytes*100/out_total_bytes)
      //(long long)out_total_bytes,(long long)out_free_bytes
      );
}

void ui_status_set_partition(int prefixbyte) {
  if (prefixbyte == 0) {
    lv_obj_remove_style(sdcard_box, &style_part_inactive, 0);
    lv_obj_add_style(sdcard_box, &style_part_active, 0);
    // lv_task_handler();
    lv_obj_remove_style(flash_box, &style_part_active, 0);
    lv_obj_add_style(flash_box, &style_part_inactive, 0);
  } else if (prefixbyte == 1) {
    lv_obj_remove_style(sdcard_box, &style_part_active, 0);
    lv_obj_add_style(sdcard_box, &style_part_inactive, 0);
    // lv_task_handler();
    lv_obj_remove_style(flash_box, &style_part_inactive, 0);
    lv_obj_add_style(flash_box, &style_part_active, 0);
  } else {
    ESP_LOGW(TAG, "Unknown partition %d", prefixbyte);
  }
}
