
#include "lvgl.h"

extern const lv_font_t chargen_font_sparse ;
extern lv_obj_t *sdcard_mount_label;
extern lv_style_t style_text_muted;
extern lv_style_t style_condensed;
extern lv_style_t style_black_bg;

void ui_status(lv_obj_t *container);
void ui_config(lv_obj_t *container);
// void ui_files1(lv_obj_t *container);
void ui_files2(lv_obj_t *container);
void ui_about(lv_obj_t *container);

bool ui_is_status_tab();

void ui_files_show_current(const uint8_t *buffer);
void ui_files_open_dir(const char *buffer);
void ui_status_set_partition(int prefixbyte);

void add_status_message(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

void sdmmc_card_info(const char *mount_point);
void update_mount_status();

void ui_status_update_device_address();

uint8_t esp_display_event(uint8_t cmd, uint8_t prefixbyte,
                                 uint8_t length, const uint8_t *buffer);

extern uint8_t backlight_percent;
void backlight_level(int level); // 0 - 1023
void backlight_on();
void backlight_off();

extern const lv_img_dsc_t micro_sd_card;
extern const lv_img_dsc_t processor;

extern const lv_image_dsc_t  c1541;
//extern const lv_image_dsc_t  splash;
void lcd_panel_draw_splash(const lv_image_dsc_t *dsc);

#if CONFIG_EXAMPLE_SHOW_SPLASH
void show_splash();
extern const unsigned char img_splash_png[];
extern unsigned int img_splash_png_len ;
extern lv_image_dsc_t splash;
#endif

#define RGB565COLOR(r, g, b)                                                   \
  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) & 0xf8) >> 3)
