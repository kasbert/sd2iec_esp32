/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2022 Jarkko Sonninen <kasper@iki.fi>
   Copyright (C) 2007-2017  Ingo Korb <ingo@akana.de>

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


   arch-timer.h: Architecture-specific system timer definitions

*/

#ifndef ARCH_TIMER_H
#define ARCH_TIMER_H

#include <stdint.h>
#include <esp_attr.h>

/* Types for unsigned and signed tick values */
typedef uint32_t tick_t;
typedef int32_t stick_t;

extern int32_t arch_timeout;

// https://sub.nanona.fi/esp8266/timing-and-ticks.html
static inline int32_t asm_ccount(void) {
    int32_t r;
    asm volatile ("rsr %0, ccount" : "=r"(r));
    return r;
}

/**
 * start_timeout - start a timeout
 * @usecs: number of microseconds before timeout
 *
 * This function sets up a timer so it times out after the specified
 * number of microseconds.
 */

IRAM_ATTR
static inline void start_timeout(uint32_t usecs) {
  arch_timeout = asm_ccount() + usecs * (CONFIG_MCU_FREQ/1000000);
}

/**
 * has_timed_out - returns true if timeout was reached
 *
 * This function returns true if the timer started by start_timeout
 * has reached its timeout value.
 */
IRAM_ATTR
static inline unsigned int has_timed_out(void) {
  return (int32_t)(arch_timeout - asm_ccount()) < 0;
}


static inline void delay_us(unsigned int usecs) {
  // 160MHz clock, 160 cycles/us
  volatile int32_t timeout = asm_ccount() + usecs * (CONFIG_MCU_FREQ/1000000);
  while ((int32_t)(timeout - asm_ccount()) >= 0) ;
}

static inline void delay_ms(unsigned int msecs) {
  // This is used only in some fastloaders and time <= 20ms
  delay_us(msecs * 1000);
}

#endif

