/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2021-2024 Jarkko Sonninen <kasper@iki.fi>

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

   debug.c: Verbose debug functions

*/

#include "config.h"
#include "debug.h"
#include "iec.h"
#include "uart.h"
#include "led.h"
#include "iec-bus.h"
#include <stdarg.h>
#include <string.h>
#include <esp_system.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"

static const char *TAG = "sd2iec_esp32";

// FIXME
#define  ets_printf printf

#ifdef CONFIG_DEBUG_VERBOSE
uint8_t atn_state, clock_state, data_state, srq_state;

static void ets_timestamp(void) {
  unsigned long ts = esp_timer_get_time();
  ets_printf("%3lu.%03lu %03lu ", ts / 1000000, (ts / 1000) % 1000, ts % 1000);
}

void debug_d(const char *tag, void *buf, size_t len) {
  char b[256];
  strncpy(b, buf, 255);
  b[255] = 0;
  ESP_LOGI(tag, "%s", b);
}


void debug_state(const char *tag) {
    ESP_LOGI(tag, "%-15s %-15s %s%s%s%s ATN:%c CLOCK:%c DATA:%c leds:%d core:%d",
    state2str(iec_data.bus_state), dstate2str(iec_data.device_state),
    (iec_data.iecflags & EOI_RECVD) ? "EOI " : "",
    (iec_data.iecflags & COMMAND_RECVD) ? "COMMAND " : "",
    (iec_data.iecflags & JIFFY_ACTIVE) ? "JIFFY_ACTIVE " : "",
    (iec_data.iecflags & JIFFY_LOAD) ? "JIFFY_LOAD " : "",
    (char)(!atn_state ? '_' : '0' + !!IEC_ATN),
    (char)(!clock_state ? '_' : '0' + !!IEC_CLOCK),
    (char)(!data_state ? '_' : '0' + !!IEC_DATA), led_state,
    xPortGetCoreID());
}

void debug_atn_command(char *message, uint8_t cmd1) {
  ESP_LOGI(TAG, "%s ATNCMD %02x %-15s dev/sec %d", message,cmd1,atncmd2str(cmd1),cmd1&0x1f);
}


char *state2str(int bus_state)
{
  switch (bus_state)
  {
  case BUS_SLEEP:
    return "BUS_SLEEP";
  case BUS_IDLE:
    return "BUS_IDLE";
  case BUS_FOUNDATN:
    return "BUS_FOUNDATN";
  case BUS_ATNACTIVE:
    return "BUS_ATNACTIVE";
  case BUS_FORME:
    return "BUS_FORME";
  case BUS_NOTFORME:
    return "BUS_NOTFORME";
  case BUS_ATNFINISH:
    return "BUS_ATNFINISH";
  case BUS_ATNPROCESS:
    return "BUS_ATNPROCESS";
  case BUS_CLEANUP:
    return "BUS_CLEANUP";
  }
  return "UNKNOWN STATE";
}

char *dstate2str(int device_state)
{
  switch (device_state)
  {
  case DEVICE_IDLE:
    return "DEVICE_IDLE";
  case DEVICE_LISTEN:
    return "DEVICE_LISTEN";
  case DEVICE_TALK:
    return "DEVICE_TALK";
  }
  return "UNKNOWN STATE";
}

char *atncmd2str(int cmd)
{
  if (cmd == 0x3F)
    return "ATN_CODE_UNLISTEN";
  if (cmd == 0x5F)
    return "ATN_CODE_UNTALK";
  switch (cmd & 0xf0)
  {
  case 0x20:
    return "ATN_CODE_LISTEN";
  case 0x40:
    return "ATN_CODE_TALK";
  case 0x60:
    return "ATN_CODE_DATA";
  case 0xE0:
    return "ATN_CODE_CLOSE";
  case 0xF0:
    return "ATN_CODE_OPEN";
  };
  return "UNKNOWN CMD";
}

// USE ESP_LOG_BUFFER_HEXDUMP instead
void debug_print_buffer(const char *msg, unsigned const char *p, size_t size)
{
  ets_timestamp();
  ets_printf("%s [%d] '", msg, size);
  for (int i = 0; i < size; i++)
  {
    if (p[i] == 0) {
      ets_printf("\\0");
    }
    else if (p[i] < ' ' || p[i] > 127)
    {
      ets_printf("\\x%02x", p[i]);
    }
    else
    {
      ets_printf("%c", p[i]);
    }
  }
  ets_printf("'\n");
}
#endif
