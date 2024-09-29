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


   display.c: Remote display interface

*/

#include "config.h"
//#include "i2c.h"

//#include <esp_event.h>
#include <esp_log.h>
#include <string.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

//#include "uart.h"
#include "display.h"

#include "esp32/esp-display.h"
/*
#include "eeprom-conf.h"
#include "bus.h"
#include "ustring.h"
*/
#include "espfs.h"
#include "utils.h"
#include "parser.h"
/*
#include "cbmdirent.h"

#include "wrapops.h"
*/

static const char *TAG = "esp-display";


/*
#include "buffers.h"
#include "fatops.h"
#include "iec.h"
#include "parser.h"
#include "progmem.h"
#include "ustring.h"
#include "utils.h"
#include "display.h"
*/

uint8_t display_found;
QueueHandle_t to_system_queue, to_display_queue;

void display_init_early() {
  to_system_queue = xQueueCreate(10, sizeof(system_message));
  to_display_queue = xQueueCreate(10, sizeof(display_message));
}

uint8_t display_init(uint8_t len, uint8_t *message) {
  // TODO
  display_found = 1;
  return display_found;
}

void display_send_prefixed(uint8_t cmd, uint8_t prefixbyte, uint8_t len, const uint8_t *buffer) {
  //ESP_LOGI(TAG, "display_send_prefixed cmd %x prefix %x len %d quelen %d core %d", cmd, prefixbyte, len, uxQueueMessagesWaiting(to_display_queue), xPortGetCoreID());
  display_message msg;
  msg.cmd = cmd;
  msg.prefixbyte = prefixbyte;
  len = min(sizeof(msg.buffer)-1, (size_t)len);
  msg.len = len;
  memcpy(msg.buffer, buffer, len);
  msg.buffer[len] = 0;
  /*
  if (cmd == DISPLAY_FILENAME_READ || cmd == DISPLAY_FILENAME_WRITE) {
    pet2asc(msg.buffer);
  }
  */
  if (xQueueSendToBack( to_display_queue, ( void * ) &msg, 0) != pdPASS){
      // Failed to post the message, even after 10 ticks.
      ESP_LOGE(TAG, "Cannot send to to_display_queue");
      return;
  }
  system_wake();
}

void display_send_cmd(uint8_t cmd, uint8_t len, const void *buf) {
  display_send_prefixed(cmd, 0, len, buf);
}


static void vfs_path(char *buffer, uint8_t part, char *name) {
  strcpy (buffer, partition[part].base_path);
  strcat (buffer, "/");
  if (partition[part].current_dir.pathname[0]) {
    strcat (buffer, partition[part].current_dir.pathname);
    strcat (buffer, "/");
  }
  strcat (buffer, name);
}

void display_filename_read(uint8_t part, cbmdirent_t *dent) {
  char buffer[512]; // FIXME
  vfs_path(buffer, part, dent->pvt.vfs.realname);
  display_send_prefixed(DISPLAY_FILENAME_READ, part, strlen(buffer), (unsigned char *)buffer);
  //display_send_prefixed(DISPLAY_FILENAME_READ, part, len, buf);
}

void display_filename_write(uint8_t part, cbmdirent_t *dent) {
  char buffer[512]; // FIXME
  vfs_path(buffer, part, dent->pvt.vfs.realname);
  display_send_prefixed(DISPLAY_FILENAME_WRITE, part, strlen(buffer), (unsigned char *)buffer);
  //display_send_prefixed(DISPLAY_FILENAME_WRITE, part, len, buf);
}

void display_address(uint8_t dev) {
  display_send_prefixed(DISPLAY_ADDRESS, dev, 0, 0);
}

void display_current_part(uint8_t part) {
  display_send_prefixed(DISPLAY_CURRENT_PART, part, 0, 0);
}

void display_menu_reset(void) {
  display_send_prefixed(DISPLAY_MENU_RESET, 0, 0, 0);
}

void display_current_directory(uint8_t part, const char *name) {
  char buffer[512]; // FIXME
  strcpy (buffer, partition[part].base_path);
  //strcat (buffer, "/");
  strcat (buffer, name);
  //printf("HELLO display_current_directory %s %s \n", name, buffer);

  display_send_prefixed(DISPLAY_CURRENT_DIR, part, strlen(buffer), (unsigned char *)buffer);
  //display_send_prefixed(DISPLAY_CURRENT_DIR, part, strlen((char *)name), (unsigned char *)name);
}


#if 0
// From doscmd.h
void do_chdir(uint8_t *parsestr);


void i2c_init(void) {
  ESP_LOGI(TAG, "No i2c_init"); // Not used here
}

// Called from mainloop, when active_keys |= KEY_DISPLAY;
// menucommand is set in UI
#endif

void display_service(void) {}

bool system_receive_message(system_message *msg) {
    return xQueueReceive(to_system_queue, msg, (TickType_t)0);
}

bool display_receive_message(display_message *msg) {
  return xQueueReceive(to_display_queue, msg, (TickType_t)1);
}


// Test stuff

#include "buffers.h"
#include "doscmd.h"
#include "fileops.h"

#include "parser.h"
#include "arch-timer.h"

static void read_to_end(uint8_t secondary) {
  buffer_t *buf;
  buf = find_buffer(secondary);
  while (buf) {
    ESP_LOGI(TAG, "Buf %p position %d lastused %d recordlen %d sendeoi %d", buf,
             buf->position, buf->lastused, buf->recordlen, buf->sendeoi);
    printf("DIRH %p count %d i %d\n", &buf->pvt.dir.dh, buf->pvt.dir.dh.dir.vfs.count, buf->pvt.dir.dh.dir.vfs.i);
    ESP_LOG_BUFFER_HEXDUMP(TAG, buf->data, buf->lastused, ESP_LOG_INFO);
    if (buf->sendeoi) {
      break;
    }
    ESP_LOGI(TAG, "refill %d", buf->refill(buf));
    /*
       uint8_t lastused;
       uint8_t position;
       uint8_t secondary;
       uint8_t recordlen;
       uint32_t fptr;  // FIXME: Missing from doc comment
       int     allocated:1;
       int     mustflush:1;
       int     read:1;
       int     write:1;
       int     dirty:1;
       int     sendeoi:1;
       int     sticky:1;
     */
    buf = find_buffer(secondary);
  }
  if (buf) {
    cleanup_and_free_buffer(buf);
  } else {
    ESP_LOGE(TAG, "NOOOO BUF in read");
  }
}

static void speed_test() {

    portDISABLE_INTERRUPTS();
  volatile int32_t timeout = asm_ccount() + 111100 * (CONFIG_MCU_FREQ/1000000);
  volatile int32_t now = asm_ccount(), prev;
  uint32_t min = 10000000, max = 0, count = 0;
  uint64_t sum = 0;
  prev = now - 50;
  while ((int32_t)(timeout - now) >= 0) {
    uint32_t diff = now - prev;
    if (diff < min) min = diff;
    if (diff > max) max = diff;
    sum += diff;
    count ++;
    prev = now;
    now = asm_ccount();
  }
    portENABLE_INTERRUPTS();
  printf("MIN %ld MAX %ld COUNT %ld AVG %lld\n", min, max, count, sum/count);
}

void do_test_stuff() {
  static int count = -1;
  buffer_t *buf;
  ESP_LOGI(TAG,
           "HELLO---- BEGIN TEST STUFF %d partition %d base_path %s cwd '%s'",
           count, current_part, partition[current_part].base_path,
           partition[current_part].current_dir.pathname);
  switch (count) {
  case -1:

    for (int i = 0; i < 50; i++)
      speed_test();
    break;

  case 0:
    ESP_LOGI(TAG, "HELLO TEST STUFF DIR");
    strcpy((char *)command_buffer, "$");
    command_length = strlen((char *)command_buffer);
    file_open(0);
    read_to_end(0);
    break;

  case 1:
    ESP_LOGI(TAG, "HELLO TEST STUFF FILE OPEN FOO.TXT");
    strcpy((char *)command_buffer, "FOO.TXT");
    command_length = strlen((char *)command_buffer);
    file_open(0);
    read_to_end(0);
    break;

  case 2:
    ESP_LOGI(TAG, "MD ");
    strcpy((char *)command_buffer, "MD:TMP");
    command_length = strlen((char *)command_buffer);
    parse_doscommand();
    break;

  case 3:
    ESP_LOGI(TAG, "CD ");
    strcpy((char *)command_buffer, "CD:TMP");
    command_length = strlen((char *)command_buffer);
    parse_doscommand();
    ESP_LOGI(TAG, "RD ");
    strcpy((char *)command_buffer, "RD:TMP");
    command_length = strlen((char *)command_buffer);
    parse_doscommand();
    break;

  case 4:
    ESP_LOGI(TAG, "FILE WRITE JOO");
    strcpy((char *)command_buffer, "JOO");
    command_length = strlen((char *)command_buffer);
    file_open(1);
    buf = find_buffer(1);
    if (!buf) {
      ESP_LOGE(TAG, "NOOO WRITE NO BUF!");
      break;
    }
    strcpy((char *)buf->data, "ABC");
    buf->lastused = 4;
    buf->sendeoi = 1;
    ESP_LOGI(TAG, "Buf position %d lastused %d recordlen %d sendeoi %d",
             buf->position, buf->lastused, buf->recordlen, buf->sendeoi);
    ESP_LOG_BUFFER_HEXDUMP(TAG, buf->data, buf->lastused, ESP_LOG_INFO);
    buf->refill(buf);
    cleanup_and_free_buffer(buf);
    break;

  case 5:
    ESP_LOGI(TAG, "FILE OPEN JOO");
    strcpy((char *)command_buffer, "JOO");
    command_length = strlen((char *)command_buffer);
    file_open(0);
    read_to_end(0);
    break;

  case 6:
    ESP_LOGI(TAG, "HELLO TEST STUFF DIR");
    strcpy((char *)command_buffer, "$");
    command_length = strlen((char *)command_buffer);
    file_open(0);
    read_to_end(0);
    break;

  case 7:
    ESP_LOGI(TAG, "DEL JOO ");
    strcpy((char *)command_buffer, "S:JOO");
    command_length = strlen((char *)command_buffer);
    parse_doscommand();
    break;

  case 8:
    ESP_LOGI(TAG, "HELLO TEST STUFF DIR");
    strcpy((char *)command_buffer, "$");
    command_length = strlen((char *)command_buffer);
    file_open(0);
    read_to_end(0);
    break;

  case 9:
    ESP_LOGI(TAG, "CD _");
    strcpy((char *)command_buffer, "CD:_");
    command_length = strlen((char *)command_buffer);
    parse_doscommand();
    break;

  case 10:
    ESP_LOGI(TAG, "RD ");
    strcpy((char *)command_buffer, "RD:TMP");
    command_length = strlen((char *)command_buffer);
    parse_doscommand();
    break;

  default:
    count = -2;
  }
  command_length = 0;
  count++;
  ESP_LOGI(TAG,
           "HELLO---- END TEST STUFF %d partition %d base_path %s cwd '%s'",
           count, current_part, partition[current_part].base_path,
           partition[current_part].current_dir.pathname);
}


#include "bus.h"
#include "display.h"
#include "doscmd.h"
#include "eeprom-conf.h"
#include "parser.h"
#include "wrapops.h"

void send_system_message(uint8_t cmd, char* data) {
  system_message msg;
  msg.cmd = cmd;
  if (data) {
    strcpy(msg.data, data);
  } else {
    msg.data[0] = 0;
  }
  if (xQueueSendToBackFromISR(to_system_queue, (void *)&msg, 0) != pdPASS) {
    ESP_LOGE(TAG, "Cannot send to system queue");
  }
}


// called from esp-system
void system_display_service(system_message *msg) {
  ESP_LOGI(TAG, "Received msg %d %s", (int)msg->cmd, system_cmd2str(msg->cmd));
  switch (msg->cmd) {
  case SYSTEM_CHADDR:
    /* New address selected */
    if (msg->data[0] >= 8 && msg->data[0] < 15) {
      device_address = msg->data[0];
    }
    display_address(device_address);
    break;

  case SYSTEM_DOSCMD:
    strcpy((char *)command_buffer, msg->data);
    command_length = strlen((char *)command_buffer);
    parse_doscommand();
    command_length = 0;
    break;

  case SYSTEM_CHDIR:
    /* New directory selected */
    path_t path;
    cbmdirent_t dent;

    if (current_part >= 2) {
      image_unmount(current_part);
    }
    /* Read directory name into displaybuffer */
    path.part = current_part;
    path.dir = partition[current_part].current_dir;

    if (msg->data[0] == '/') {
      // absolute dir, real name
      char *fn = msg->data;
      memset(&dent, 0, sizeof(dent));
      ESP_LOGI(TAG, "CD '%s' part %d", fn, path.part);
      strcpy((char*)dent.pvt.vfs.realname, fn);
      char *ext;
      dent.typeflags = check_extension(fn, &ext);
      if (dent.typeflags == TYPE_UNK) {
        // Cannot determine dir from filename
        dent.typeflags = TYPE_DIR;
      }
      strcpy(path.dir.pathname, "/");
      //strcpy(path.dir.pathname, fn);
      dent.name[0] = '.';
      dent.name[1] = 0;
    } else if (msg->data[0] == '.' && msg->data[1] == '.' && msg->data[2] == 0) {
      /* Previous directory */
      dent.name[0] = '_';
      dent.name[1] = 0;
    } else {
      if (first_match(&path, (uint8_t *)msg->data, FLAG_HIDDEN, &dent))
        return;
    }

    w_chdir(&path, &dent);
    if (dent.typeflags == TYPE_IMG_DISK) {
      strcpy (&path.dir.pathname, (char*)dent.pvt.vfs.realname);
    }
    update_current_dir(&path);
    break;

  case SYSTEM_STORE:
    write_configuration();
    break;

  case SYSTEM_MOUNT:
    esp32fs_sdcard_mount(SDMOUNT_POINT);
    display_send_prefixed(DISPLAY_MOUNTED, 0, 0, 0);
    break;

  case SYSTEM_UNMOUNT:
    esp32fs_sdcard_unmount(SDMOUNT_POINT);
    display_send_prefixed(DISPLAY_UNMOUNTED, 0, 0, 0);
    break;

  case 69:
    if (esp32fs_sdcard_ismounted()) {
      esp32fs_filetest(SDMOUNT_POINT, esp32fs_sdcard_get_name());
      esp32fs_list_files(SDMOUNT_POINT);
      // show_dir(ctx->file_table, SDMOUNT_POINT);
    }
    break;

  default:
    void do_test_stuff();
    do_test_stuff();
  }
}

char *system_cmd2str(uint8_t cmd) {
  switch (cmd) {
  case SYSTEM_STORE:
    return "SYSTEM_STORE";
  case SYSTEM_CHDIR:
    return "SYSTEM_CHDIR";
  case SYSTEM_CHADDR:
    return "SYSTEM_CHADDR";
  case SYSTEM_DOSCMD:
    return "SYSTEM_DOSCMD";
  case SYSTEM_MOUNT:
    return "SYSTEM_MOUNT";
  case SYSTEM_UNMOUNT:
    return "SYSTEM_UNMOUNT";
  case 42:
    return "TEST STUFF";
  case 69:
    return "FILE TEST";
  }
  return "UNKNOWN";
}

char *display_cmd2str(uint8_t cmd) {
  switch (cmd) {
  case DISPLAY_INIT:
    return "DISPLAY_INIT";
  case DISPLAY_ADDRESS:
    return "DISPLAY_ADDRESS";
  case DISPLAY_FILENAME_READ:
    return "DISPLAY_FILENAME_READ";
  case DISPLAY_FILENAME_WRITE:
    return "DISPLAY_FILENAME_WRITE";
  case DISPLAY_DOSCOMMAND:
    return "DISPLAY_DOSCOMMAND";
  case DISPLAY_ERRORCHANNEL:
    return "DISPLAY_ERRORCHANNEL";
  case DISPLAY_CURRENT_DIR:
    return "DISPLAY_CURRENT_DIR";
  case DISPLAY_CURRENT_PART:
    return "DISPLAY_CURRENT_PART";
  case DISPLAY_MENU_RESET:
    return "DISPLAY_MENU_RESET";
  case DISPLAY_MENU_ADD:
    return "DISPLAY_MENU_ADD";
  case DISPLAY_MENU_SHOW:
    return "DISPLAY_MENU_SHOW";
  case DISPLAY_MENU_GETSELECTION:
    return "DISPLAY_MENU_GETSELECTION";
  case DISPLAY_MENU_GETENTRY:
    return "DISPLAY_MENU_GETENTRY";
  case DISPLAY_MOUNTED:
    return "DISPLAY_MOUNTED";
  case DISPLAY_UNMOUNTED:
    return "DISPLAY_UNMOUNTED";
  }
  return "UNKNOWN";
}


