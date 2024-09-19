#include "sdkconfig.h"
#if ESP_PLATFORM
#include "esp_log.h"
#include "esp_system.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "cbmdirent.h"
#include "display.h"
#include "esp32/esp-display.h"
#include "esp32/espfs.h"
#include "lv_file_explorer.h"
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
static lv_obj_t *file_explorer;

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

static void file_explorer_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  ESP_LOGI(TAG, "HELLO EVENT %d", code);

  if (code != LV_EVENT_VALUE_CHANGED) {
    return;
  }
  const char *cur_path = lv_file_explorer_get_current_path(obj);
  const char *sel_fn = lv_file_explorer_get_selected_file_name(obj);
  uint16_t path_len = strlen(cur_path);
  uint16_t fn_len = strlen(sel_fn);
  ESP_LOGI(TAG, "HELLO LV_EVENT_VALUE_CHANGED %s %s", cur_path, sel_fn);

  if ((path_len + fn_len) <= LV_FILE_EXPLORER_PATH_MAX_LEN) {
    char file_info[LV_FILE_EXPLORER_PATH_MAX_LEN];

    char *ptr, *fn = file_info;
    /* Search for the file extension */
    uint8_t typeflags = check_extension(sel_fn, &ptr);

    strcpy(file_info, cur_path);
    // Strip /sdcard and /flash from the beginning
    if (!strncmp(SDMOUNT_POINT, file_info, strlen(SDMOUNT_POINT))) {
      fn += strlen(SDMOUNT_POINT);
    } else if (!strncmp(SPIMOUNT_POINT, file_info, strlen(SPIMOUNT_POINT))) {
      fn += strlen(SPIMOUNT_POINT);
    }
    strcat(file_info, sel_fn);

    // FIXME check if image/dir

    ESP_LOGI(TAG, "CD %s", fn);
    send_system_message(SYSTEM_CHDIR, fn);
  } else {
    LV_LOG_USER("%s%s", cur_path, sel_fn);
  }
}

void lv_example_file_explorer_1(lv_obj_t *parent) {}

// UI functions

// static void ui_files1(lv_obj_t *container);
static void ui_files2(lv_obj_t *container);

/* Get info about glyph of `unicode_letter` in `font` font.
 * Store the result in `dsc_out`.
 * The next letter (`unicode_letter_next`) might be used to calculate the width
 * required by this glyph (kerning)
 */
bool chargen_get_glyph_dsc_cb(const lv_font_t *font,
                              lv_font_glyph_dsc_t *dsc_out,
                              uint32_t unicode_letter,
                              uint32_t unicode_letter_next) {
  dsc_out->adv_w = 8; /*Horizontal space required by the glyph in [px]*/
  dsc_out->box_h = 8; /*Height of the bitmap in [px]*/
  dsc_out->box_w = 8; /*Width of the bitmap in [px]*/
  dsc_out->ofs_x = 0; /*X offset of the bitmap in [pf]*/
  dsc_out->ofs_y = 0; /*Y offset of the bitmap measured from the as line*/
  dsc_out->format = LV_FONT_GLYPH_FORMAT_A1; // LV_FONT_GLYPH_FORMAT_A1;
                                             // LV_FONT_GLYPH_FORMAT_IMAGE
  dsc_out->is_placeholder = 0;

  if (unicode_letter >= 0x60 && unicode_letter <= 0x7f) {
    unicode_letter -= 0x60;
  }
  dsc_out->gid.index = unicode_letter;
  // printf ("YEAH GLYPH DSC %c\n", (int)unicode_letter);

  return true; /*true: glyph found; false: glyph was not found*/
}

typedef struct chargen_desc {
  int width;
} lv_font_fmt_chargen_dsc_t;

/* Get the bitmap of `unicode_letter` from `font`. */
const void *chargen_get_glyph_bitmap_cb(lv_font_glyph_dsc_t *gdsc,
                                        lv_draw_buf_t *draw_buf) {
  uint32_t gid_index = gdsc->gid.index;
  if (gid_index > 0xff) {
    ESP_LOGE(TAG, "Invalid chargen char %lx", (long)gid_index);
    return NULL;
  }

  //  const lv_font_t *font = gdsc->resolved_font;
  //  // lv_font_fmt_chargen_dsc_t * fdsc = (lv_font_fmt_chargen_dsc_t
  //  *)font->dsc;

  uint8_t *bitmap_out = draw_buf->data;
  const uint8_t *bitmap_in = chargen_bin + 2048 + (gid_index * 8);
  uint8_t *bitmap_out_tmp = bitmap_out;
  int32_t i = 0;
  int32_t x, y;
  uint32_t stride =
      lv_draw_buf_width_to_stride(gdsc->box_w, LV_COLOR_FORMAT_A8);

  for (y = 0; y < 8; y++, bitmap_in++, bitmap_out_tmp += stride) {
    for (i = 0x80, x = 0; x < 8; x++, i >>= 1) {
      bitmap_out_tmp[x] = (*bitmap_in) & i ? 0xff : 0x00;
    }
  }
  return draw_buf;
}

lv_font_fmt_chargen_dsc_t chargen_desc = {
    .width = 8,
};
/*Initialize a public general font descriptor*/
const lv_font_t chargen_font = {
    .get_glyph_dsc =
        chargen_get_glyph_dsc_cb, /*Set a callback to get info about glyphs*/
    .get_glyph_bitmap =
        chargen_get_glyph_bitmap_cb, /*Set a callback to get bitmap of a glyph*/
    .line_height = 8, /*The maximum line height required by the font*/
    .base_line = 0,   /*Baseline measured from the bottom of the line*/
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = 0,
    .underline_thickness = 0,
    .dsc = &chargen_desc, /*The custom font data. Will be accessed by
                             `get_glyph_bitmap/dsc` */
};

lv_font_fmt_chargen_dsc_t chargen_desc_sparse = {
    .width = 9,
};
/*Initialize a public general font descriptor*/
const lv_font_t chargen_font_sparse = {
    .get_glyph_dsc =
        chargen_get_glyph_dsc_cb, /*Set a callback to get info about glyphs*/
    .get_glyph_bitmap =
        chargen_get_glyph_bitmap_cb, /*Set a callback to get bitmap of a glyph*/
    .line_height = 9, /*The maximum line height required by the font*/
    .base_line = 0,   /*Baseline measured from the bottom of the line*/
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = 0,
    .underline_thickness = 0,
    .dsc = &chargen_desc_sparse, /*The custom font data. Will be accessed by
                                    `get_glyph_bitmap/dsc` */
};

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
  // lv_obj_t *t2 = lv_tabview_add_tab(tv, "About");
  ui_status(t0);
  // browser_init(t1);
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

static void ui_files2(lv_obj_t *parent) {
#if 1

  file_explorer = lv_file_explorer_create(parent);
  lv_file_explorer_set_sort(file_explorer, LV_EXPLORER_SORT_KIND);

  /* linux */
  lv_file_explorer_open_dir(file_explorer, SDMOUNT_POINT);

#if LV_FILE_EXPLORER_QUICK_ACCESS
  char *envvar = "HOME";
  char home_dir[LV_FS_MAX_PATH_LENGTH];
  strcpy(home_dir, "A:");
  /* get the user's home directory from the HOME environment variable*/
  strcat(home_dir, getenv(envvar));
  LV_LOG_USER("home_dir: %s\n", home_dir);
  lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_HOME_DIR,
                                         home_dir);
  char video_dir[LV_FS_MAX_PATH_LENGTH];
  strcpy(video_dir, home_dir);
  strcat(video_dir, "/Videos");
  lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_VIDEO_DIR,
                                         video_dir);
  char picture_dir[LV_FS_MAX_PATH_LENGTH];
  strcpy(picture_dir, home_dir);
  strcat(picture_dir, "/Pictures");
  lv_file_explorer_set_quick_access_path(file_explorer,
                                         LV_EXPLORER_PICTURES_DIR, picture_dir);
  char music_dir[LV_FS_MAX_PATH_LENGTH];
  strcpy(music_dir, home_dir);
  strcat(music_dir, "/Music");
  lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_MUSIC_DIR,
                                         music_dir);
  char document_dir[LV_FS_MAX_PATH_LENGTH];
  strcpy(document_dir, home_dir);
  strcat(document_dir, "/Documents");
  lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_DOCS_DIR,
                                         document_dir);

  lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_FS_DIR,
                                         "A:/");
#endif

  lv_obj_add_event_cb(file_explorer, file_explorer_event_handler,
                      LV_EVENT_VALUE_CHANGED, NULL);

#endif
}
#include "display.h"

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
    add_status_message("READ %d:%s", prefixbyte, buffer);
    break;

  case DISPLAY_FILENAME_WRITE:
    add_status_message("WRITE %d:%s", prefixbyte, buffer);
    break;

  case DISPLAY_DOSCOMMAND:
    add_status_message("DOSCMD %s", buffer);
    break;

  case DISPLAY_ERRORCHANNEL:
    add_status_message("%s", buffer);
    break;

  case DISPLAY_CURRENT_DIR:
    add_status_message("current part %d dir %s", prefixbyte, buffer);
    break;

  case DISPLAY_CURRENT_PART:
    add_status_message("current part %d", prefixbyte);
    ui_status_set_partition(prefixbyte);
    if (prefixbyte == 0) {
      lv_file_explorer_open_dir(file_explorer, SDMOUNT_POINT);
    } else if (prefixbyte == 1) {
      lv_file_explorer_open_dir(file_explorer, SPIMOUNT_POINT);
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
