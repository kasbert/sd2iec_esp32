
#include "sdkconfig.h"
#if ESP_PLATFORM
#include "esp_log.h"
#include "esp_system.h"
#endif

#include <stdio.h>

#include "esp32/esp-display.h"
#include "ui.h"
#include "ui_common.h"

#define TAG "ui_about"

static void test_system_event_handler(lv_event_t *e) {
  send_system_message(42, 0);
}

static void test_event_handler(lv_event_t *e) {
  ESP_LOGI(TAG, "Testing SD card file operations");
  send_system_message(69, 0);
}

void ui_about(lv_obj_t *container) {
  // lv_obj_t *container = lv_obj_create(tab);
  //lv_obj_remove_style_all(container);
  //lv_style_copy(&style, &lv_style_transp);
  //lv_obj_add_style(container, &style, 0);
    
  lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
  // lv_style_set_pad_row(&container, 0);
  lv_obj_add_style(container, &style_black_bg, 0);

  lv_obj_set_style_margin_left(container, -40, LV_PART_MAIN);
  lv_obj_set_style_margin_right(container, -40, LV_PART_MAIN);
  lv_obj_set_style_margin_top(container, 0, LV_PART_MAIN);

  {
    // Drive pic 
    lv_obj_t *cont0 = lv_obj_create(container);
    lv_obj_remove_style_all(cont0);
    // lv_obj_set_size(cont, 300, 220);
    lv_obj_set_width(cont0, lv_pct(100));
    lv_obj_set_height(cont0, LV_SIZE_CONTENT);
    // lv_obj_center(cont0);
    lv_obj_set_flex_flow(cont0, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(cont0, LV_OBJ_FLAG_SCROLLABLE);

#if 1
    const char *label_text = "SD2IEC ESP32 by Kasper in 2024\n"
      "Graphics by Manu\n";
    lv_obj_t *label1 = lv_label_create(cont0);
    lv_obj_remove_style_all(label1);
    //lv_obj_set_style_margin_top(label1, -40, LV_PART_MAIN);
    //lv_obj_set_pos(label1, 0, 0);
    //lv_obj_add_style(label1, &style_text_muted, LV_PART_MAIN);
    lv_obj_set_style_text_color(label1, lv_color_make(0xf0, 0x00, 0x00), LV_PART_MAIN);

    lv_obj_set_style_pad_left(label1, 40, LV_PART_MAIN);
    lv_obj_set_style_text_font(label1, &chargen_font_sparse,
                               LV_PART_MAIN);
    lv_label_set_text(label1, label_text);

    lv_obj_t *label2 = lv_label_create(label1);
    lv_obj_remove_style_all(label2);
    //lv_obj_set_style_margin_top(label2, -40, LV_PART_MAIN);
    lv_obj_set_pos(label2, 1, 1);
    lv_obj_set_style_text_color(label2, lv_color_white(), LV_PART_MAIN);
    //lv_obj_set_style_pad_left(label2, 40, LV_PART_MAIN);
    lv_obj_set_style_text_font(label2, &chargen_font_sparse,
                               LV_PART_MAIN);
    lv_label_set_text(label2, label_text);
#endif


#if CONFIG_EXAMPLE_SHOW_SPLASH
    lv_obj_t *img = lv_image_create(cont0);
//lv_img_set_offset_x(img, -20);
    lv_img_set_offset_y(img, -80);
    lv_obj_set_height(img, 400);
    lv_image_set_src(img, &splash);
    //lv_obj_set_pos(img, 0, 0);
    lv_obj_set_width(img, LV_HOR_RES);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_align(img, LV_ALIGN_CENTER);

    //lv_obj_set_style_margin_bottom(cont0, -40, LV_PART_MAIN);
#else
    lv_obj_t *img = lv_image_create(cont0);
    lv_image_set_src(img, &c1541);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_margin_left(img, 40, LV_PART_MAIN);
    //lv_obj_set_pos(img, 40, 0);
#endif
    //lv_obj_add_event_cb(icon, test_system_event_handler , LV_EVENT_CLICKED, 0);
    //lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  }
  {
    lv_obj_t *cont3 = lv_obj_create(container);
    lv_obj_remove_style_all(cont3);
    lv_obj_set_style_margin_top(cont3, 10, LV_PART_MAIN);
    lv_obj_set_style_margin_left(cont3, 40, LV_PART_MAIN);
    // lv_obj_set_size(cont, 300, 220);
    lv_obj_set_width(cont3, lv_pct(100));
    lv_obj_set_height(cont3, LV_SIZE_CONTENT);
    // lv_obj_center(cont0);
    lv_obj_set_flex_flow(cont3, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(cont3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(cont3, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(cont3, 0, LV_PART_MAIN);

    lv_obj_t *help_label = lv_label_create(cont3);
    lv_obj_remove_style_all(help_label);
    lv_obj_set_pos(help_label, 0, 0);
    lv_obj_set_width(help_label, lv_pct(100));
    lv_obj_set_style_text_font(help_label, &chargen_font_sparse,
                               LV_PART_MAIN);
    //lv_obj_add_style(help_label, &style_text_muted, LV_PART_MAIN);
    //lv_obj_add_style(help_label, &style_condensed, LV_PART_MAIN);
    lv_label_set_text(help_label, 
      "Filename \"{{n}{path}:}pattern\"\n"
      " n : partition \"0\" = SDCARD \"1\" == Flash\n"
      " path: directory in sdcard\n"
      " pattern: commodore file name\n"
      "Load directory \"${{n}{path}{:pattern{=type}}}\""
      );

  }
#if 0
  {
    lv_obj_t *cont0 = lv_obj_create(container);
    lv_obj_set_width(cont0, lv_pct(100));
    lv_obj_set_height(cont0, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont0, LV_FLEX_FLOW_ROW);
    lv_obj_t *test_btn = lv_btn_create(cont0);
    //lv_obj_set_height(test_btn, LV_SIZE_CONTENT);
    //lv_obj_set_width(test_btn, LV_PCT(30));
    lv_obj_t *test_label = lv_label_create(test_btn);
    lv_label_set_text(test_label, "Test files");
    lv_obj_center(test_label);
    lv_obj_add_event_cb(test_btn, test_event_handler, LV_EVENT_CLICKED, 0);

    lv_obj_t *test_btn2 = lv_btn_create(cont0);
    lv_obj_set_height(test_btn2, LV_SIZE_CONTENT);
    //lv_obj_set_width(test_btn2, LV_PCT(30));
    lv_obj_t *test_label2 = lv_label_create(test_btn2);
    lv_label_set_text(test_label2, "Test stuff");
    lv_obj_center(test_label2);
    lv_obj_add_event_cb(test_btn2, test_system_event_handler, LV_EVENT_CLICKED, 0);
  }
#endif

}
