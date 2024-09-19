/* sd2iec - SD/MMC to Commodore serial bus interface/controller
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


   display.h: Remote display interface

*/

#ifndef ESP_DISPLAY_H
#define ESP_DISPLAY_H

#include <stdint.h>

typedef struct {
 uint8_t cmd;
 uint8_t prefixbyte;
 uint8_t len;
 uint8_t buffer[81];
} display_message;

enum system_commands {
  SYSTEM_STORE = 1,
  SYSTEM_CHDIR,
  SYSTEM_CHADDR,
  SYSTEM_DOSCMD,
  SYSTEM_MOUNT,
  SYSTEM_UNMOUNT
};

enum display_commands_extra {
  // In addition to display.h
  DISPLAY_MOUNTED=0x50,
  DISPLAY_UNMOUNTED,
};

typedef struct {
 char cmd;
 char data[83]; // CONFIG_COMMAND_BUFFER_SIZE == 255
} system_message;

void display_init_early();
void system_wake();

void send_system_message(uint8_t cmd, char* data);

bool system_receive_message(system_message *msg);
void system_display_service(system_message *msg);

bool display_receive_message(display_message *msg);

char *display_cmd2str(uint8_t cmd);
char *system_cmd2str(uint8_t cmd);

#endif // ESP_DISPLAY_H
