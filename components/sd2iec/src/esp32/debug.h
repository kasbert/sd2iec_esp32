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
#ifndef DEBUG_H
#define DEBUG_H

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <stddef.h>
#include <esp_log.h>
//#define TAG "sd2iec"

#ifdef CONFIG_DEBUG_VERBOSE
void debug_state(const char *tag);
void debug_print_buffer(const char *msg, unsigned const char *p, size_t size);
void debug_atn_command(char *message, uint8_t cmd1);
char *state2str(int bus_state);
char *dstate2str(int device_state);
char *atncmd2str(int cmd);

#else
#define ESP_LOGI(tag, fmt,args...) do{}while(0)
#define debug_state(tag) do{}while(0)
#define debug_print_buffer(a,b) do{}while(0)
#endif

#endif
