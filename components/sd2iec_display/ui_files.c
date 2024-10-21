
#include "sdkconfig.h"
#if ESP_PLATFORM
#include "esp_log.h"
#include "esp_system.h"
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "cbmdirent.h"
#include "esp32/esp-display.h"
#include "esp32/espfs.h"
#include "lv_file_explorer.h"
#include "ui.h"
#include "ui_common.h"

#define TAG "ui_files"

static lv_obj_t *file_explorer;


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
    uint16_t typeflags = check_extension(sel_fn, &ptr);

    strcpy(file_info, cur_path);
    // Strip /sdcard and /flash from the beginning
    if (!strncmp(SDMOUNT_POINT, file_info, strlen(SDMOUNT_POINT))) {
      fn += strlen(SDMOUNT_POINT);
    } else if (!strncmp(SPIMOUNT_POINT, file_info, strlen(SPIMOUNT_POINT))) {
      fn += strlen(SPIMOUNT_POINT);
    }
    strcat(file_info, sel_fn);

    // FIXME check if image/dir

    ESP_LOGI(TAG, "CD '%s'", fn);
    send_system_message(SYSTEM_CHDIR, fn);
  } else {
    LV_LOG_USER("%s%s", cur_path, sel_fn);
  }
}

void lv_example_file_explorer_1(lv_obj_t *parent) {}



void ui_files2(lv_obj_t *parent) {
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

void ui_files_open_dir(const char *buffer) {
    lv_file_explorer_open_dir(file_explorer, (char *)buffer);
    return;
}

void ui_files_show_current(const uint8_t *buffer) {
    DIR *dp = opendir ((char*)buffer);
    if (dp) {
  ESP_LOGI(TAG, "HELLO DISPLAY_CURRENT_DIR %s", buffer);
        // Directory
        closedir(dp);
        lv_file_explorer_open_dir(file_explorer, (char *)buffer);
        return;
    }
  // Not a directory
  ESP_LOGI(TAG, "HELLO DISPLAY_CURRENT_DIR FILE %s", buffer);
  char *ptr = strrchr((char*)buffer, '/');
  if (ptr) {
    *ptr = 0;
    ptr += 1;
  } else {
    ptr = (char*)buffer;
  }
  const char *current_path = lv_file_explorer_get_current_path(file_explorer);
  if (!strcmp(current_path, (char*)buffer)) {
    //lv_file_explorer_open_dir(file_explorer, (char *)buffer);
  }
  int row = lv_file_explorer_find_file_row(file_explorer, ptr);
  lv_file_explorer_set_highlight_row(file_explorer, row);
  ESP_LOGI(TAG, "HELLO DISPLAY_CURRENT_DIR  FILE %s cd %s %s row %d", ptr, buffer, current_path, row);
}
