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


   filesystem.h: small abstractions for multi-filesystem support

*/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#ifdef CONFIG_HAVE_FATFS
#include "fatops.h"
#endif
#ifdef CONFIG_HAVE_EEPROMFS
#include "eefs-ops.h"
#endif
#ifdef CONFIG_HAVE_VFS
#include "vfsops.h"
#endif

// FIXME: Move d64_invalidate and maybe p00cache_invalidate out of fatops.c?

/* initialize both fatops and eefs */
static inline void filesystem_init(uint8_t preserve_dir) {
#ifdef CONFIG_HAVE_FATFS
  fatops_init(preserve_dir);
#endif
#ifdef CONFIG_HAVE_EEPROMFS
  eefsops_init();
#endif
#ifdef CONFIG_HAVE_VFS
#if CONFIG_SD2IEC_USE_SDCARD
  vfsops_init(preserve_dir, SDMOUNT_POINT);
#endif
#if CONFIG_SD2IEC_USE_SPI_PARTITION
  vfsops_init(preserve_dir, SPIMOUNT_POINT);
#endif
#endif
}

#endif
