/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2024 Jarkko Sonninen <kasper@iki.fi>

   Inspired by MMC2IEC by Lars Pontoppidan et al.

   FAT filesystem access based on code from ChaN and Jim Brain, see ff.c|h.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License only.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


   arch-config.c: ESP32 port

*/

#ifndef ARCH_CONFIG_H
#define ARCH_CONFIG_H

#include <stdint.h>

#include "esp32/iec-bus.h"
#include "integer.h"
#include "led.h"

#if CONFIG_SD2IEC_USE_SDCARD
#define SDMOUNT_POINT "/sdcard"
#endif

#if CONFIG_SD2IEC_USE_SPI_PARTITION
#define SPIMOUNT_POINT "/flash"
#endif

// Leds

extern volatile uint8_t led_state;

// Initialize ports for all LEDs
//
static inline void leds_init(void) {
  /* Set the GPIO as a push/pull output */
//    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
#if CONFIG_SD2IEC_PIN_LED_BUSY != -1
  gpio_set_direction(CONFIG_SD2IEC_PIN_LED_BUSY, GPIO_MODE_OUTPUT);
#endif
#if CONFIG_SD2IEC_PIN_LED_DIRTY != -1
  gpio_set_direction(CONFIG_SD2IEC_PIN_LED_DIRTY, GPIO_MODE_OUTPUT);
#endif
}

static inline void set_busy_led(uint8_t state) {
#if CONFIG_SD2IEC_PIN_LED_BUSY != -1
  gpio_set_level(CONFIG_SD2IEC_PIN_LED_BUSY, state);
#endif
  if (state) {
    led_state |= LED_BUSY;
  } else {
    led_state &= (uint8_t)~LED_BUSY;
  }
}

static inline void set_dirty_led(uint8_t state) {
#if CONFIG_SD2IEC_PIN_LED_DIRTY != -1
  gpio_set_level(CONFIG_SD2IEC_PIN_LED_DIRTY, state);
#endif
  if (state) {
    led_state |= LED_DIRTY;
  } else {
    led_state &= (uint8_t)~LED_DIRTY;
  }
}

// Toggle function used for error blinking
static inline void toggle_dirty_led(void) {
  set_dirty_led(!(led_state & LED_DIRTY));
}

// Buttons

typedef uint8_t rawbutton_t;
static inline uint8_t buttons_read() { return 0; }
static inline void buttons_init(void) {}
//#define BUTTON_NEXT 1
//#define BUTTON_PREV 2

static inline void device_hw_address_init(void) {}
static inline int device_hw_address() { return 8; }
#define SPI_SPEED_SLOW 0
static inline void spi_init(int speed) {}
static inline unsigned int display_intrq_active(void) { return 0; }

/* Interrupt handler for system tick */
#define SYSTEM_TICK_HANDLER IRAM_ATTR void systick_handler(void *arg)

/* so the _HANDLER macros are created here.     */
// #define SD_CHANGE_HANDLER  IRAM_ATTR void sdcard_change_handler(void)

extern uint8_t file_extension_mode;

#endif