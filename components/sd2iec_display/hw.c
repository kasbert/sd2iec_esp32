/*

ESP32-4848S040-demo
See
https://github.com/arendst/Tasmota/discussions/20527


*/

#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_check.h>

#include "lvgl.h"
#include "esp_lvgl_port.h"


// lcd
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_st7701.h"
#include "driver/ledc.h"

#if CONFIG_EXAMPLE_TOUCH_I2C_NUM > -1
#include "esp_lcd_touch.h"
#include "driver/i2c.h"
#include "esp_lcd_touch_gt911.h"
#endif

#include "hw.h"

static const char *TAG = "lcd_hw";

#include "esp_attr.h"
#include "esp_timer.h"

#include <esp_random.h>

static esp_lcd_panel_io_handle_t   panel_io_handle;
static esp_lcd_panel_handle_t      panel_handle;
static esp_lcd_panel_io_handle_t   touch_io_handle;
static esp_lcd_touch_handle_t      touch_handle;
static lv_display_t *disp;

#if 0
static void lcd_panel_test(esp_lcd_panel_handle_t panel_handle);
#endif

const st7701_lcd_init_cmd_t lcd_init_cmds1[] = {
    //BEGIN_WRITE,
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10,}, 5, 0},
    {0xC0, (uint8_t []){0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t []){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t []){0x31, 0x05}, 2, 0},
    {0xCD, (uint8_t []){0x00}, 1, 0},//0x08
     // Positive Voltage Gamma Control
    {0xB0, (uint8_t []){0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18,}, 16, 0},
     // Negative Voltage Gamma Control
    {0xB1, (uint8_t []){0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18,}, 16, 0},
    // PAGE1
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x11,}, 5, 0},
    {0xB0, (uint8_t []){0x60}, 1, 0}, // Vop=4.7375v
    {0xB1, (uint8_t []){0x32}, 1, 0}, // VCOM=32
    {0xB2, (uint8_t []){0x07}, 1, 0}, // VGH=15v
    {0xB3, (uint8_t []){0x80}, 1, 0},
    {0xB5, (uint8_t []){0x49}, 1, 0}, // VGL=-10.17v
    {0xB7, (uint8_t []){0x85}, 1, 0},
    {0xB8, (uint8_t []){0x21}, 1, 0}, // AVDD=6.6 & AVCL=-4.6
    {0xC1, (uint8_t []){0x78}, 1, 0},
    {0xC2, (uint8_t []){0x78}, 1, 0},
    {0xE0, (uint8_t []){0x00, 0x1B, 0x02,}, 3, 0},
    {0xE1, (uint8_t []){0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44,}, 11, 0},
    {0xE2, (uint8_t []){0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00,}, 12, 0},
    {0xE3, (uint8_t []){0x00, 0x00, 0x11, 0x11,}, 4, 0},
    {0xE4, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t []){0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0,}, 16, 0},
    {0xE6, (uint8_t []){0x00, 0x00, 0x11, 0x11,}, 4, 0},
    {0xE7, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t []){0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0,}, 16, 0},
    {0xEB, (uint8_t []){0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40,}, 7, 0},
    {0xEC, (uint8_t []){0x3C, 0x00}, 2, 0},
    {0xED, (uint8_t []){0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA,}, 16, 0},
    //-----------VAP & VAN---------------
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x13,}, 5, 0},
    {0xE5, (uint8_t []){0xE4}, 1, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x00,}, 5, 0},
    {0x3A, (uint8_t []){0x60}, 1, 10}, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565
    //{0x21, (uint8_t []){0x00}, 0, 0},0X00,
    ////END_WRITE,
    //DELAY, 10,
    {0x11, (uint8_t []){0x00}, 0, 120}, // Sleep Out
    //END_WRITE,
    //DELAY, 120,
    //BEGIN_WRITE,
    {0x29, (uint8_t []){0x00}, 0, 0}, // Display On
    //END_WRITE
};

#define USE_BACKLIGHT_PWM 1
#if USE_BACKLIGHT_PWM
#define EXAMPLE_LEDC_CHANNEL LEDC_CHANNEL_3
#define EXAMPLE_LEDC_TIMER LEDC_TIMER_3

void backlight_level(int level) { // 0 - 1023
    ledc_set_duty(LEDC_LOW_SPEED_MODE, EXAMPLE_LEDC_CHANNEL, level);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, EXAMPLE_LEDC_CHANNEL);
}

void backlight_on() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, EXAMPLE_LEDC_CHANNEL, 1023);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, EXAMPLE_LEDC_CHANNEL);
}

void backlight_off() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, EXAMPLE_LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, EXAMPLE_LEDC_CHANNEL);
}

static void backlight_del() {
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_reset_pin(CONFIG_EXAMPLE_LCD_BK_LIGHT_PIN));
}

static void backlight_init() {
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE,           // timer mode
        .timer_num = EXAMPLE_LEDC_TIMER,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t led_cfg = {
        .channel    = EXAMPLE_LEDC_CHANNEL,
        .duty       = 0,
        .gpio_num   = CONFIG_EXAMPLE_LCD_BK_LIGHT_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = EXAMPLE_LEDC_TIMER
    };
    ledc_channel_config(&led_cfg);
}

#else
void backlight_on() {
    // Turn on backlight (Different LCD screens may need different levels)
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(CONFIG_EXAMPLE_LCD_BK_LIGHT_PIN, CONFIG_EXAMPLE_LCD_BK_LIGHT_ON_LEVEL));
}

void backlight_off() {
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(CONFIG_EXAMPLE_LCD_BK_LIGHT_PIN, !CONFIG_EXAMPLE_LCD_BK_LIGHT_ON_LEVEL));
}

static void backlight_init() {
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << CONFIG_EXAMPLE_LCD_BK_LIGHT_PIN
    };
    // Initialize the GPIO of backlight
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
}

static void backlight_del() {
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_reset_pin(CONFIG_EXAMPLE_LCD_BK_LIGHT_PIN));
}
#endif

 static bool example_panel_init() {

    backlight_init();
    // Turn off backlight to avoid unpredictable display on the LCD screen while initializing
    // the LCD panel driver. (Different LCD screens may need different levels)
    backlight_off();

    panel_io_handle = NULL;
    panel_handle = NULL;

    ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO, //IO_TYPE_EXPANDER, // Set to `IO_TYPE_GPIO` if using GPIO, same to below
        .cs_gpio_num = CONFIG_EXAMPLE_LCD_IO_SPI_CS,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = CONFIG_EXAMPLE_LCD_IO_SPI_SCL,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = CONFIG_EXAMPLE_LCD_IO_SPI_SDA,
        .io_expander = NULL,    // Set to NULL if not using IO expander
    };
    esp_lcd_panel_io_3wire_spi_config_t panel_io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&panel_io_config, &panel_io_handle));

    ESP_LOGI(TAG, "Install ST7701S panel driver");
    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT, // LCD_CLK_SRC_PLL160M
        .data_width = CONFIG_EXAMPLE_RGB_DATA_WIDTH,
        .bits_per_pixel = CONFIG_EXAMPLE_RGB_BIT_PER_PIXEL,
        .de_gpio_num = CONFIG_EXAMPLE_LCD_IO_RGB_DE,
        .pclk_gpio_num = CONFIG_EXAMPLE_LCD_IO_RGB_PCLK,
        .vsync_gpio_num = CONFIG_EXAMPLE_LCD_IO_RGB_VSYNC,
        .hsync_gpio_num = CONFIG_EXAMPLE_LCD_IO_RGB_HSYNC,
        .disp_gpio_num = CONFIG_EXAMPLE_LCD_IO_RGB_DISP,
        .data_gpio_nums = {
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA0,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA1,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA2,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA3,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA4,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA5,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA6,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA7,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA8,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA9,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA10,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA11,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA12,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA13,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA14,
            CONFIG_EXAMPLE_LCD_IO_RGB_DATA15,
        },
        .timings = {
            .pclk_hz = CONFIG_EXAMPLE_LCD_PIXEL_CLOCK_HZ, /* > 10MHz does not work */
            .h_res = CONFIG_EXAMPLE_LCD_H_RES,
            .v_res = CONFIG_EXAMPLE_LCD_V_RES,
            .hsync_front_porch = CONFIG_EXAMPLE_LCD_H_F_PORCH,
            .hsync_pulse_width = CONFIG_EXAMPLE_LCD_H_P_WIDTH,
            .hsync_back_porch = CONFIG_EXAMPLE_LCD_H_B_PORCH,
            .vsync_front_porch = CONFIG_EXAMPLE_LCD_V_F_PORCH,
            .vsync_pulse_width = CONFIG_EXAMPLE_LCD_V_P_WIDTH,
            .vsync_back_porch = CONFIG_EXAMPLE_LCD_V_B_PORCH,
            .flags.pclk_active_neg = false,
            .flags.hsync_idle_low = 0, // (hsync_polarity == 0)
            .flags.vsync_idle_low = 0, // (vsync_polarity == 0)
            //.flags.de_idle_high = 0,
            //.flags.pclk_idle_high = 0,
        },
        .psram_trans_align = 64,
        .sram_trans_align = 8,
#if CONFIG_EXAMPLE_LCD_DOUBLE_FB
        .num_fbs = 2,
#else
        .num_fbs = 1,
#endif
        //.bounce_buffer_size_px = CONFIG_EXAMPLE_LCD_H_RES*8,
        .flags = {
            .fb_in_psram = 1, // allocate frame buffer in PSRAM
#if CONFIG_EXAMPLE_LCD_DOUBLE_FB
            .double_fb = 1,
#endif
            //.refresh_on_demand = 0,
            .bb_invalidate_cache = 1,
            //.disp_active_low = 0,
            //.relax_on_idle = 0,
        },
    };

    st7701_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        .init_cmds = lcd_init_cmds1,      // Uncomment these line if use custom initialization commands
        .init_cmds_size = sizeof(lcd_init_cmds1) / sizeof(st7701_lcd_init_cmd_t),
        .flags = {
            .auto_del_panel_io = 0,
            /**
             * Set to 1 if panel IO is no longer needed after LCD initialization.
             * If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
             * Please set it to 1 to release the pins.
             */
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = CONFIG_EXAMPLE_LCD_IO_RST,           // Set to -1 if not use
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,     // Implemented by LCD command `36h`
        .bits_per_pixel = CONFIG_EXAMPLE_LCD_BIT_PER_PIXEL,    // Implemented by LCD command `3Ah` (16/18/24)
        .vendor_config = &vendor_config,
    };

    /**
     * Only create RGB when `auto_del_panel_io` is set to 0,
     * or initialize st7701 meanwhile
    */
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(panel_io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));     // Only reset RGB when `auto_del_panel_io` is set to 1, or reset st7701 meanwhile
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));      // Only initialize RGB when `auto_del_panel_io` is set to 1, or initialize st7701 meanwhile

    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
    // the gap is LCD panel specific, even panels with the same driver IC, can have different gap value
    esp_lcd_panel_set_gap(panel_handle, 0, 0);
    // Turn on the screen
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if CONFIG_EXAMPLE_TOUCH_I2C_NUM > -1
    /*
    * Touch panel init
    */
    i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = CONFIG_EXAMPLE_TOUCH_I2C_SDA,
      .scl_io_num = CONFIG_EXAMPLE_TOUCH_I2C_SCL,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master = {
        .clk_speed = CONFIG_EXAMPLE_TOUCH_I2C_CLK_HZ,
      },
      .clk_flags = 0, /*!< Optional, you can use
        I2C_SCLK_SRC_FLAG_*
        flags to choose i2c source clock here. */
  };

  ESP_ERROR_CHECK(i2c_param_config(CONFIG_EXAMPLE_TOUCH_I2C_NUM, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(CONFIG_EXAMPLE_TOUCH_I2C_NUM, I2C_MODE_MASTER, 0, 0, 0));

  touch_io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
  // Attach the TOUCH to the I2C bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)CONFIG_EXAMPLE_TOUCH_I2C_NUM,
                                           &tp_io_config, &touch_io_handle));

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = CONFIG_EXAMPLE_LCD_H_RES,
      .y_max = CONFIG_EXAMPLE_LCD_V_RES,
      .rst_gpio_num = CONFIG_EXAMPLE_TOUCH_GPIO_RST,
      .int_gpio_num = CONFIG_EXAMPLE_TOUCH_GPIO_INT,
      .levels =
          {
              .reset = 0,
              .interrupt = 0,
          },
      .flags =
          {
              .swap_xy = 0,
              .mirror_x = 0,
              .mirror_y = 0,
          },
      .process_coordinates = NULL,
      .interrupt_callback = NULL,
  };

  ESP_LOGI(TAG, "Initialize touch controller GT911");
#if DO_I2C_SCAN
    i2c_cmd_handle_t cmd;
    for (int i = 0; i < 0x7f; i++)
    {
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        if (i2c_master_cmd_begin(CONFIG_EXAMPLE_TOUCH_I2C_NUM, cmd, portMAX_DELAY) == ESP_OK)
        {
            ESP_LOGI(TAG, "Got I2C response from address %02X", i);
        }
        i2c_cmd_link_delete(cmd);
    }
#endif
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(touch_io_handle, &tp_cfg, &touch_handle));
#endif

    return true;
}

void sd2iec_lcd_hw_del() {
    backlight_off();
    if (disp) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(lvgl_port_remove_disp(disp));
    }
    esp_lcd_panel_disp_on_off(panel_handle, false);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_del(panel_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_io_del(panel_io_handle));
    //ESP_ERROR_CHECK_WITHOUT_ABORT(spi_bus_free(CONFIG_EXAMPLE_SPI_HOST_ID));
    // TODO touch_del
    backlight_del();
}

bool sd2iec_lvgl_init() {
    ESP_LOGI(TAG, "Initialize LVGL with port");
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,         /* LVGL task priority */
        .task_stack = 8192,         /* LVGL task stack size */
        .task_affinity = 0,        /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500,   /* Maximum sleep in LVGL task */
        .timer_period_ms = 5        /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = panel_io_handle,
        .panel_handle = panel_handle,
        .buffer_size = CONFIG_EXAMPLE_LCD_H_RES * CONFIG_EXAMPLE_LCD_V_RES,
#if CONFIG_EXAMPLE_LCD_DOUBLE_FB
        .double_buffer = 1,
#endif
        .hres = CONFIG_EXAMPLE_LCD_H_RES,
        .vres = CONFIG_EXAMPLE_LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
        }
    };
    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
#if EXAMPLE_LCD_RGB_BOUNCE_BUFFER_MODE
            .bb_mode = true,
#else
            .bb_mode = false,
#endif
#if EXAMPLE_LCD_LVGL_AVOID_TEAR
            .avoid_tearing = true,
#else
            .avoid_tearing = false,
#endif
        }
    };
    disp = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);
    // buffer was filled with white
    lcd_panel_draw_rectangle(0, 0, CONFIG_EXAMPLE_LCD_H_RES, CONFIG_EXAMPLE_LCD_V_RES, 0); 

#if CONFIG_EXAMPLE_TOUCH_I2C_NUM > -1
    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = touch_handle,
    };
    // lv_indev_t* th =
    lvgl_port_add_touch(&touch_cfg);
#endif
    return true;
}


int hw_test_io() {

char data[1];
#define ESP_LCD_TOUCH_GT911_READ_KEY_REG    (0x8093)
#define ESP_LCD_TOUCH_GT911_READ_XY_REG     (0x814E)
#define ESP_LCD_TOUCH_GT911_CONFIG_REG      (0x8047)
#define ESP_LCD_TOUCH_GT911_PRODUCT_ID_REG  (0x8140)
#define ESP_LCD_TOUCH_GT911_ENTER_SLEEP     (0x8040)
    esp_err_t err = esp_lcd_panel_io_rx_param(touch_io_handle, ESP_LCD_TOUCH_GT911_READ_XY_REG, data, 1);
    ESP_RETURN_ON_ERROR(err, TAG, "I2C read error!");
    return 0;
}

esp_err_t sd2iec_lcd_hw_init(void)
{
    esp_err_t ret = ESP_OK;
    panel_io_handle = 0;
    panel_handle = 0;
    touch_io_handle = 0;
    touch_handle = 0;
    disp = 0;
    example_panel_init();
    //lcd_panel_test(panel_handle);
    return ret;
/*
    err:
    sd2iec_lcd_hw_del();
    return false;
*/
}

#define TEST_IMG_SIZE (200 * 200 * sizeof(uint16_t))

#if 0
// Draw some coloured rectangles
static void lcd_panel_test(esp_lcd_panel_handle_t panel_handle)
{
    uint8_t *img = heap_caps_malloc(TEST_IMG_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(img == 0);

    for (int i = 0; i < 10; i++) {
        uint8_t color_byte = esp_random() & 0xFF;
        int x_start = esp_random() % (CONFIG_EXAMPLE_LCD_H_RES - 200);
        int y_start = esp_random() % (CONFIG_EXAMPLE_LCD_V_RES - 200);
        memset(img, color_byte, TEST_IMG_SIZE);
        esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_start + 200, y_start + 200, img);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    free(img);
}
#endif

void lcd_panel_draw_rectangle(uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t *img = heap_caps_malloc(width * height * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(img == 0);

    for (int i = 0; i < width * height; i++) {
        img[i] = color;
    }
    esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_start + width, y_start + height, img);
    free(img);
}

void lcd_panel_draw_splash(const lv_image_dsc_t *dsc)
{
    uint16_t x_start = (CONFIG_EXAMPLE_LCD_H_RES - dsc->header.w) / 2;
    uint16_t y_start = (CONFIG_EXAMPLE_LCD_V_RES - dsc->header.h) / 2;
    uint16_t width = dsc->header.w;
    uint16_t height = dsc->header.h;
    uint16_t *img = (uint16_t *)dsc->data;
    esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_start + width, y_start + height, img);
}
