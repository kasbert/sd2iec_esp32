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


   iec-bus.c: Architecture-specific IEC bus initialisation

   This is not in arch-config.h becaue using the set_* functions
   from iec-bus.h simplifies the code and the ARM version isn't
   space-constrained yet.
*/

#include "config.h"
#include "iec-bus.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include "esp_rom_gpio.h"
#include "rom/gpio.h"

void system_pin_intr_handler();
static void pin_intr_handler(void *ctx);

/*** Timer/GPIO interrupt demux ***/

/* Declare handler functions */
#ifdef SD_CHANGE_HANDLER
SD_CHANGE_HANDLER;
#endif
IEC_ATN_HANDLER;
#ifdef IEC_CLOCK_HANDLER
IEC_CLOCK_HANDLER;
#endif

IRAM_ATTR
static void pin_intr_handler(void *ctx) {
#if USE_COMMON_ISR_HANDLER
#else
  uint32_t gpio_intr_status = READ_PERI_REG(
      GPIO_STATUS_REG); // read status to get interrupt status for GPIO0-31
  uint32_t gpio_intr_status_h = READ_PERI_REG(
      GPIO_STATUS1_REG); // read status1 to get interrupt status for GPIO32-39
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG,
                    gpio_intr_status); // Clear intr for gpio0-gpio31
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG,
                    gpio_intr_status_h); // Clear intr for gpio32-39
  // gpio_intr_status & BIT(IEC_BIT_ATN);
#endif
  iec_atn_handler();

#ifdef CONFIG_LOADER_DREAMLOAD
  iec_clock_handler();
#endif

  system_pin_intr_handler();
}

void iec_interrupts_init(void) {
#if USE_COMMON_ISR_HANDLER
  gpio_isr_handler_add(IEC_PIN_ATN, pin_intr_handler, 0);
#else
  gpio_isr_handle_t handle;
  gpio_isr_register(pin_intr_handler, 0,
                    ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED, &handle);
#endif
#ifdef HAVE_CLOCK_IRQ
#if USE_COMMON_ISR_HANDLER
  gpio_isr_handler_add(IEC_PIN_CLOCK, pin_intr_handler, 0);
#endif
#endif

  gpio_set_intr_type(IEC_PIN_ATN, GPIO_INTR_NEGEDGE);
#ifdef HAVE_CLOCK_IRQ
  gpio_set_intr_type(IEC_PIN_CLOCK, GPIO_INTR_NEGEDGE);
#endif
}

void iec_interface_init(void) {
  gpio_reset_pin(IEC_PIN_ATN);
  gpio_intr_disable(IEC_PIN_ATN);

  gpio_pad_select_gpio(IEC_PIN_ATN);
  gpio_pad_select_gpio(IEC_PIN_DATA);
  gpio_pad_select_gpio(IEC_PIN_CLOCK);
#ifdef IEC_PIN_SRQ
  gpio_pad_select_gpio(IEC_PIN_SRQ);
#endif

  gpio_set_direction(IEC_PIN_ATN, GPIO_MODE_INPUT);
  gpio_set_direction(IEC_PIN_DATA, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_set_direction(IEC_PIN_CLOCK, GPIO_MODE_INPUT_OUTPUT_OD);
#ifdef IEC_PIN_SRQ
  gpio_set_direction(IEC_PIN_SRQ, GPIO_MODE_INPUT_OUTPUT_OD);
#endif

#if 1
  gpio_set_pull_mode(IEC_PIN_ATN, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(IEC_PIN_DATA, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(IEC_PIN_CLOCK, GPIO_PULLUP_ONLY);
#ifdef IEC_PIN_SRQ
  gpio_set_pull_mode(IEC_PIN_SRQ, GPIO_PULLUP_ONLY);
#endif
#endif

  /* Set up outputs before switching the pins */
  set_atn(1);
  set_data(1);
  set_clock(1);
  set_srq(1);

  /* SRQ is special-cased because it may be unconnected */
}

void bus_interface_init(void)
    __attribute__((weak, alias("iec_interface_init")));
