/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2024 Jarkko Sonninen <kasper@iki.fi>
   Copyright (C) 2007-2022  Ingo Korb <ingo@akana.de>

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


   iec-bus.h: A few wrappers around the port definitions

*/

#ifndef IEC_BUS_H
#define IEC_BUS_H

#include "config.h"

#include <rom/gpio.h>
#include <driver/gpio.h>
#include <esp_attr.h>

/* output functions are defined in arch-config.h */
//#define CONFIG_HAVE_IEC 1
/* --- IEC --- */
#ifdef CONFIG_HAVE_IEC

//#define HAVE_CLOCK_IRQ 1


/* Return type of iec_bus_read() */
#if 0
typedef uint64_t iec_bus_t;

// Override iec_bus.h
#define IEC_BUS_H

#define IEC_BIT_ATN ((iec_bus_t)1L << (IEC_PIN_ATN))
#define IEC_BIT_DATA ((iec_bus_t)1L << (IEC_PIN_DATA))
#define IEC_BIT_CLOCK ((iec_bus_t)1L << (IEC_PIN_CLOCK))
#ifdef IEC_PIN_SRQ
#define IEC_BIT_SRQ ((iec_bus_t)1 << (IEC_PIN_SRQ))
#endif
#else

typedef uint8_t iec_bus_t;
#define IEC_BIT_ATN 1
#define IEC_BIT_DATA 2
#define IEC_BIT_CLOCK 4
#ifdef IEC_PIN_SRQ
#define IEC_BIT_SRQ 8
#endif
#endif

/*** Input definitions (generic versions) ***/
#ifdef IEC_INPUTS_INVERTED
#define COND_INV(state) (!state)
#define IEC_ATN (gpio_get_level(IEC_PIN_ATN) ? 0 : IEC_BIT_ATN)
#define IEC_CLOCK (gpio_get_level(IEC_PIN_CLOCK) ? 0 : IEC_BIT_CLOCK)
#define IEC_DATA (gpio_get_level(IEC_PIN_DATA) ? 0 : IEC_BIT_DATA)
#ifdef IEC_PIN_SRQ
#define IEC_SRQ (gpio_get_level(IEC_PIN_SRQ) ? 0 : IEC_BIT_SRQ)
#define IEC_INPUT (IEC_ATN | IEC_CLOCK | IEC_DATA | IEC_SRQ)
#else
#define IEC_INPUT (IEC_ATN | IEC_CLOCK | IEC_DATA)
#endif

static inline iec_bus_t iec_bus_read(void) { return ~IEC_INPUT; }
#else
#define COND_INV(state) (state)

#if USE_SLOW_GPIO
#define IEC_ATN (gpio_get_level(IEC_PIN_ATN) ? IEC_BIT_ATN : 0)
#define IEC_CLOCK (gpio_get_level(IEC_PIN_CLOCK) ? IEC_BIT_CLOCK : 0)
#define IEC_DATA (gpio_get_level(IEC_PIN_DATA) ? IEC_BIT_DATA : 0)
#ifdef IEC_PIN_SRQ
#define IEC_SRQ (gpio_get_level(IEC_PIN_SRQ) ? IEC_BIT_SRQ : 0)
#define IEC_INPUT (IEC_ATN | IEC_CLOCK | IEC_DATA | IEC_SRQ)
#else
#define IEC_INPUT (IEC_ATN | IEC_CLOCK | IEC_DATA)
#endif
#else
#if IEC_PIN_ATN < 32
#define IEC_ATN ((REG_READ(GPIO_IN_REG) & (1 << IEC_PIN_ATN)) ? IEC_BIT_ATN : 0)
#else
#define IEC_ATN                                                                \
  ((REG_READ(GPIO_IN1_REG) & (1 << (IEC_PIN_ATN - 32))) ? IEC_BIT_ATN : 0)
#endif
#if IEC_PIN_CLOCK < 32
#define IEC_CLOCK                                                              \
  ((REG_READ(GPIO_IN_REG) & (1 << IEC_PIN_CLOCK)) ? IEC_BIT_CLOCK : 0)
#else
#define IEC_CLOCK                                                              \
  ((REG_READ(GPIO_IN1_REG) & (1 << (IEC_PIN_CLOCK - 32))) ? IEC_BIT_CLOCK : 0)
#endif
#if IEC_PIN_DATA < 32
#define IEC_DATA                                                               \
  ((REG_READ(GPIO_IN_REG) & (1 << IEC_PIN_DATA)) ? IEC_BIT_DATA : 0)
#else
#define IEC_DATA                                                               \
  ((REG_READ(GPIO_IN1_REG) & (1 << (IEC_PIN_DATA - 32))) ? IEC_BIT_DATA : 0)
#endif
#ifdef IEC_PIN_SRQ
#define IEC_SRQ (gpio_get_level(IEC_PIN_SRQ) ? IEC_BIT_SRQ : 0)
#define IEC_INPUT (IEC_ATN | IEC_CLOCK | IEC_DATA | IEC_SRQ)
#else
#define IEC_INPUT (IEC_ATN | IEC_CLOCK | IEC_DATA)
#endif
#endif

// TODO optimize

static inline iec_bus_t iec_bus_read(void) { return IEC_INPUT; }

/* IEC output functions */

#ifdef CONFIG_DEBUG_VERBOSE
extern uint8_t atn_state, clock_state, data_state, srq_state;
#endif

static inline __attribute__((always_inline)) void set_atn(uint8_t state) {
#ifdef CONFIG_DEBUG_VERBOSE
  atn_state = state;
#endif
  gpio_set_level(IEC_PIN_ATN, COND_INV(state));
}

#define set_data(state) ((state) ? set_data1() : set_data0())

static inline __attribute__((always_inline)) void _set_data(uint8_t state) {
#ifdef CONFIG_DEBUG_VERBOSE
  data_state = state;
#endif
  gpio_set_level(IEC_PIN_DATA, COND_INV(state));
}

static inline __attribute__((always_inline)) void set_data0() {
#ifdef CONFIG_DEBUG_VERBOSE
  data_state = 0;
#endif
#if USE_SLOW_GPIO
  gpio_set_level(IEC_PIN_DATA, COND_INV(0));
#else
#if IEC_PIN_DATA < 32
  // TODO COND_INV
  REG_WRITE(GPIO_OUT_W1TC_REG, 1 << IEC_PIN_DATA);
#else
  REG_WRITE(GPIO_OUT1_W1TC_REG, 1 << (IEC_PIN_DATA - 32));
#endif
#endif
}

static inline __attribute__((always_inline)) void set_data1() {
#ifdef CONFIG_DEBUG_VERBOSE
  data_state = 1;
#endif
#if USE_SLOW_GPIO
  gpio_set_level(IEC_PIN_DATA, COND_INV(1));
#else
#if IEC_PIN_DATA < 32
  // TODO COND_INV
  REG_WRITE(GPIO_OUT_W1TS_REG, 1 << IEC_PIN_DATA);
#else
  REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (IEC_PIN_DATA - 32));
#endif
#endif
}

#define set_clock(state) ((state) ? set_clock1() : set_clock0())

static inline __attribute__((always_inline)) void _set_clock(uint8_t state) {
#ifdef CONFIG_DEBUG_VERBOSE
  clock_state = state;
#endif
  gpio_set_level(IEC_PIN_CLOCK, COND_INV(state));
}

static inline __attribute__((always_inline)) void set_clock0() {
#ifdef CONFIG_DEBUG_VERBOSE
  data_state = 0;
#endif
#if USE_SLOW_GPIO
  gpio_set_level(IEC_PIN_CLOCK, COND_INV(0));
#else
#if IEC_PIN_CLOCK < 32
  // TODO COND_INV
  REG_WRITE(GPIO_OUT_W1TC_REG, 1 << IEC_PIN_CLOCK);
#else
  REG_WRITE(GPIO_OUT1_W1TC_REG, 1 << (IEC_PIN_CLOCK - 32));
#endif
#endif
}

static inline __attribute__((always_inline)) void set_clock1() {
#ifdef CONFIG_DEBUG_VERBOSE
  clock_state = 1;
#endif
#if USE_SLOW_GPIO
  gpio_set_level(IEC_PIN_CLOCK, COND_INV(1));
#else
#if IEC_PIN_CLOCK < 32
  // TODO COND_INV
  REG_WRITE(GPIO_OUT_W1TS_REG, 1 << IEC_PIN_CLOCK);
#else
  REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (IEC_PIN_CLOCK - 32));
#endif
#endif
}

static inline __attribute__((always_inline)) void set_srq(uint8_t state) {
#ifdef CONFIG_DEBUG_VERBOSE
  srq_state = state;
#endif
#ifdef IEC_PIN_SRQ
  gpio_set_level(IEC_PIN_SRQ, COND_INV(state));
#endif
}

void iec_interrupts_init(void);

#define IEC_ATN_HANDLER IRAM_ATTR void iec_atn_handler(void)
#define IEC_CLOCK_HANDLER IRAM_ATTR void iec_clock_handler(void)
#ifdef PARALLEL_ENABLED
#define PARALLEL_HANDLER IRAM_ATTR void parallel_handler(void)
#endif

/* Enable/disable ATN interrupt */
static inline void set_atn_irq(uint8_t state) {
  if (state) {
    gpio_intr_enable(IEC_PIN_ATN);
  } else {
    gpio_intr_disable(IEC_PIN_ATN);
  }
}

/* Enable/disable CLOCK interrupt */
static inline void set_clock_irq(uint8_t state) {
#ifdef HAVE_CLOCK_IRQ
  if (state) {
    gpio_intr_enable(IEC_PIN_CLOCK);
  } else {
    gpio_intr_disable(IEC_PIN_CLOCK);
  }
#endif
}

void iec_interface_init(void);

#endif /* CONFIG_HAVE_IEC */

#endif
#endif
