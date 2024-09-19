#pragma once

#include <stdbool.h>
#include <stdint.h>

esp_err_t sd2iec_lcd_hw_init(void);
bool sd2iec_lvgl_init();
void sd2iec_lcd_hw_del();

void backlight_level(int level); // 0 - 1023

void lcd_panel_draw_rectangle(uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height, uint16_t color);
