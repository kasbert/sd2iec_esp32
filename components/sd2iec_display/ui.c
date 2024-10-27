#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <esp_check.h>
#include "esp_lvgl_port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hw.h"
#include "ui.h"
#include "ui_common.h"
#include "display.h"
#include "esp32/esp-display.h"
#include "esp32/espfs.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "gui"
#define LV_TICK_PERIOD_MS 1

static void gui_task(void *pvParameter);
static bool sd2iec_lcd_ui_init();


/**********************
 *   APPLICATION MAIN
 **********************/

#define UI_STACK_SIZE 4096 * 2
// StaticTask_t gui_xTaskBuffer;
// StackType_t gui_xStack[UI_STACK_SIZE];

static int64_t last_event_time;

static TaskHandle_t ui_task_handle;

esp_err_t sd2iec_display_init() {
  esp_err_t ret = ESP_OK;

  /* If you want to use a task to create the graphic, you NEED to create a
   * Pinned task Otherwise there can be problem such as memory corruption and so
   * on. NOTE: When not using Wi-Fi nor Bluetooth you can pin the gui_task to
   * core 0 */

  /*
  ui_task_handle =
      xTaskCreateStaticPinnedToCore(gui_task, "gui", UI_STACK_SIZE, NULL,
                                    tskIDLE_PRIORITY, gui_xStack,
  &gui_xTaskBuffer, 1);
                                    */
  xTaskCreatePinnedToCore(gui_task, "gui", UI_STACK_SIZE, 0, 0, &ui_task_handle,
                          0);
  return ret; // TODO check xTaskCreatePinnedToCore

/*
err:
  return ret;
*/
}

volatile uint64_t last_ui_loop;

bool extra_work() {
  last_ui_loop = esp_timer_get_time();

  // Dim backlight after 120s
  int64_t now = esp_timer_get_time();
  {
    static bool last_state;
    if (last_event_time + 120 * 1000000L > now) {
      if (!last_state) {
        backlight_level(1023 * backlight_percent / 100);
        last_state = true;
      }
    } else {
      if (last_state) {
        backlight_level(80 * backlight_percent / 100);
        last_state = false;
      }
    }
  }

  display_message msg;
  if (display_receive_message(&msg)) {
    esp_display_event(msg.cmd, msg.prefixbyte, msg.len, msg.buffer);
    // lv_task_handler();
    return true;
  }
    update_line_status();
  return false;
}


static void backlight_event_cb(lv_event_t *e) {
#if ESP_PLATFORM
  last_event_time = esp_timer_get_time();
#endif
}

static void backlight_set_event_cb(lv_event_t *e) {
  //ESP_LOGI(TAG, "BL_EVENT %d!", backlight_percent);
#if ESP_PLATFORM
  backlight_level(1023 * backlight_percent / 100);
#endif
}


bool is_ui_lagging() {
  int64_t now = esp_timer_get_time();
  return (now - last_ui_loop > 100 * 1000000L);
}

static void gui_task(void *pvParameter) {
  esp_err_t ret = ESP_OK;
  //*x = pvParameter;

  // x *ui_ctx = pvParameter;
  ESP_LOGI(TAG, "Running gui_task (core %d)", xPortGetCoreID());

  ESP_GOTO_ON_FALSE(sd2iec_lvgl_init(), ESP_ERR_INVALID_ARG, err, TAG,
                    "LVGL initialization failed!");

  ESP_GOTO_ON_FALSE(sd2iec_lcd_ui_init(), ESP_ERR_INVALID_ARG, err, TAG,
                    "LVGL init failed!");

#if CONFIG_EXAMPLE_SHOW_SPLASH
  show_splash();
  backlight_on();
  vTaskDelay(pdMS_TO_TICKS(3000));
#endif
  backlight_on();

  lvgl_port_lock(0);
  main_widget();
  lvgl_port_unlock();

  lv_obj_add_event_cb(lv_scr_act(), backlight_event_cb, LV_EVENT_ALL, 0);
  lv_obj_add_event_cb(lv_scr_act(), backlight_set_event_cb, BL_EVENT_1, 0);

  ESP_LOGI(TAG, "UI Stack high water mark %u ", uxTaskGetStackHighWaterMark(0));

  ESP_LOGI(TAG, "Enter ui loop");
  while (1) {
    last_ui_loop = esp_timer_get_time();
    /* Try to take the semaphore, call lvgl related function on success */
    if (lvgl_port_lock(1)) {
      if (extra_work()) {
        lvgl_port_unlock();
        continue;
      }
      lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
err:
  ESP_LOGE(TAG, "Internal error");
  /* A task should NEVER return */
  vTaskDelete(NULL);
}

static bool sd2iec_lcd_ui_init() {
  esp_err_t ret = ESP_OK;
  return true;
}

void update_line_status() {
  extern volatile uint8_t led_state;
#define LED_ERROR 1
#define LED_BUSY 2
#define LED_DIRTY 4

#define RGB565COLOR(r, g, b)                                                   \
  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) & 0xf8) >> 3)

  lcd_panel_draw_rectangle(20, 0, 20, 15,
                           (led_state & LED_BUSY) ? RGB565COLOR(0, 255, 0)
                                                  : RGB565COLOR(0, 64, 0));
  lcd_panel_draw_rectangle(60, 0, 20, 15,
                           (led_state & LED_DIRTY) ? RGB565COLOR(255, 0, 0)
                                                   : RGB565COLOR(64, 0, 0));
  lcd_panel_draw_rectangle(100, 0, 20, 15,
                           (gpio_get_level(CONFIG_SD2IEC_PIN_ATN))
                               ? RGB565COLOR(0, 0, 255)
                               : RGB565COLOR(0, 0, 64));
  lcd_panel_draw_rectangle(140, 0, 20, 15,
                           (gpio_get_level(CONFIG_SD2IEC_PIN_DATA))
                               ? RGB565COLOR(0, 0, 255)
                               : RGB565COLOR(0, 0, 64));
  lcd_panel_draw_rectangle(180, 0, 20, 15,
                           (gpio_get_level(CONFIG_SD2IEC_PIN_CLK))
                               ? RGB565COLOR(0, 0, 255)
                               : RGB565COLOR(0, 0, 64));
  void ui_update_line_status(uint8_t busy, uint8_t dirty);
  ui_update_line_status(led_state & LED_BUSY, led_state & LED_DIRTY);
}


#define RGB565COLOR(r, g, b)                                                   \
  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) & 0xf8) >> 3)

void ui_update_line_status(uint8_t busy, uint8_t dirty) {
  if (ui_is_status_tab()) {
    // Draw "leds" to picture
    lcd_panel_draw_rectangle(
        65, 184, 15, 15, busy ? RGB565COLOR(0, 255, 0) : RGB565COLOR(0, 64, 0));
    lcd_panel_draw_rectangle(115, 184, 15, 15,
                             dirty ? RGB565COLOR(255, 0, 0)
                                   : RGB565COLOR(64, 0, 0));
  }
}

#if CONFIG_EXAMPLE_SHOW_SPLASH
// under lvgl. lvgl musb be initialized
#include "libs/lodepng/lodepng.h"

//xxd -g 1 -i components/sd2iec_display/img/splash.png  > components/sd2iec_display/img/splash_png.c
const
#include "img/splash_png.c"
void show_splash() {
  unsigned error;
  lv_draw_buf_t * decoded ;
  unsigned width, height;
  error = lodepng_decode24((unsigned char **)&decoded, &width, &height, components_sd2iec_display_img_splash_png, components_sd2iec_display_img_splash_png_len);
  if(error) {
    ESP_LOGE(TAG, "Error %u: %s", error, lodepng_error_text(error));
    return;
  }
  ESP_LOGI(TAG, "Splash image %ux%u: %p", width, height, decoded);
  unsigned char *p = decoded->data;
  for (int i = 0; i < width*height; i++) {
    ((uint16_t*)p)[i] = RGB565COLOR(p[i*3], p[i*3+1], p[i*3+2]);
  }

  const lv_image_dsc_t splash = {
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565,
  //  .header.flags = 0 | LV_IMAGE_FLAGS_COMPRESSED,
    .header.w = width,
    .header.h = height,
    .header.stride = 800,
    .data_size = width * height * 2,
    .data = p,
  };

  lcd_panel_draw_splash(&splash);
  free(decoded);
}
#endif
