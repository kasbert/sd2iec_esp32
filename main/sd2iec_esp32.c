#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <esp_check.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if CONFIG_SD2IEC_USE_DISPLAY
#include "ui.h"
#endif

static const char *TAG = "app_main";

void app_main(void) {

#if CONFIG_SD2IEC_USE_DISPLAY
  esp_err_t sd2iec_lcd_hw_init(void);
  ESP_ERROR_CHECK(sd2iec_lcd_hw_init());
#endif

  esp_err_t sd2iec_system_init(void);
  ESP_ERROR_CHECK(sd2iec_system_init());

#if CONFIG_SD2IEC_USE_DISPLAY
  esp_err_t sd2iec_display_init();
  ESP_LOGI(TAG, "Initialize UI");
  ESP_ERROR_CHECK(sd2iec_display_init());
#endif

  volatile int64_t next_debug = esp_timer_get_time() + 1000000L;
  while (1) {
    int64_t now = esp_timer_get_time();
    if (next_debug - now < 0L) {
      void debug_state(const char *tag);
      debug_state(TAG);
      next_debug = now + 10000000L;
    }

#if CONFIG_SD2IEC_USE_DISPLAY
    extern bool is_ui_lagging();
    if (is_ui_lagging()) {
      ESP_LOGE(TAG, "ui is lagging");
      assert(0);
    }
#endif

    extern bool is_system_lagging();
    if (is_system_lagging()) {
      ESP_LOGE(TAG, "system is lagging");
      assert(0);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  ESP_LOGE(TAG, "Restarting now.\n");
  fflush(stdout);
  esp_restart();
}
