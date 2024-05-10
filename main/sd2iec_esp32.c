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

static const char *TAG = "app_main";

void app_main(void) {
  bool sd2iec_system_init(void);
  sd2iec_system_init();

  volatile int64_t next_debug = esp_timer_get_time() + 1000000L;
  while (1) {
    int64_t now = esp_timer_get_time();
    if (next_debug - now < 0L) {
      void debug_state(const char *tag);
      debug_state(TAG);
      next_debug = now + 10000000L;
    }

    extern volatile uint64_t last_system_sleep;
    if (now - last_system_sleep > 60 * 1000000L) {
      ESP_LOGE(TAG, "system is lagging");
      if (now - last_system_sleep > 300 * 1000000L) {
        //assert(0);
        break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  ESP_LOGE(TAG, "Restarting now.\n");
  fflush(stdout);
  esp_restart();
}
