/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2021 Jarkko Sonninen <kasper@iki.fi>
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


   uart.c: UART access routines

*/

#include <stdio.h>
#include "config.h"
#include "uart.h"

#if CONFIG_UART_DEBUG

#include <string.h>
#undef uart_init
#undef uart_flush
#include <esp_rom_uart.h>

void uart_div_modify(uint8_t uart_no, uint32_t DivLatchValue);

void uart_tx(char c);

void uart_puthex(uint8_t num) {
  uint8_t tmp;
  tmp = (num & 0xf0) >> 4;
  if (tmp < 10)
    uart_putc('0'+tmp);
  else
    uart_putc('a'+tmp-10);

  tmp = num & 0x0f;
  if (tmp < 10)
    uart_putc('0'+tmp);
  else
    uart_putc('a'+tmp-10);
}

void uart_trace(void *ptr, uint16_t start, uint16_t len) {
  uint16_t i;
  uint8_t j;
  uint8_t ch;
  uint8_t *data = ptr;

  data+=start;
  for(i=0;i<len;i+=16) {

    uart_puthex(start>>8);
    uart_puthex(start&0xff);
    uart_putc('|');
    uart_putc(' ');
    for(j=0;j<16;j++) {
      if(i+j<len) {
        ch=*(data + j);
        uart_puthex(ch);
      } else {
        uart_putc(' ');
        uart_putc(' ');
      }
      uart_putc(' ');
    }
    uart_putc('|');
    for(j=0;j<16;j++) {
      if(i+j<len) {
        ch=*(data++);
        if(ch<32 || ch>0x7e)
          ch='.';
        uart_putc(ch);
      } else {
        uart_putc(' ');
      }
    }
    uart_putc('|');
    uart_putcrlf();
    start+=16;
  }
}

void uart_puts_P(const char *text) {
  uint8_t ch;
  while ((ch = pgm_read_byte(text++))) {
    esp_rom_uart_tx_one_char(ch);
  }
}

void uart_putcrlf(void) {
  esp_rom_uart_tx_one_char('\n');
}

void uart_putc(char c) {
  esp_rom_uart_tx_one_char(c);
}

void sd2iec_uart_init(void) {
}

void sd2iec_uart_flush(void) {
  esp_rom_uart_tx_wait_idle(ESP_ROM_UART_0);
}

#endif