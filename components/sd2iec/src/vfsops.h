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


   vfsops.h: Definitions for the FAT operations

*/

#ifndef VFSOPS_H
#define VFSOPS_H

#include "buffers.h"
#include "cbmdirent.h"
#include "wrapops.h"

/* API */
void     vfsops_init(uint8_t preserve_dir, const char *basepath);
void     parse_error(int res, uint8_t readflag);
uint8_t  vfs_delete(path_t *path, cbmdirent_t *dent);
uint8_t  vfs_chdir(path_t *path, cbmdirent_t *dent);
void     vfs_mkdir(path_t *path, uint8_t *dirname);
void     vfs_open_read(path_t *path, cbmdirent_t *filename, buffer_t *buf);
void     vfs_open_write(path_t *path, cbmdirent_t *filename, uint8_t type, buffer_t *buf, uint8_t append);
uint8_t  vfs_getdirlabel(path_t *path, uint8_t *label);
uint8_t  vfs_getid(path_t *path, uint8_t *id);
uint16_t vfs_freeblocks(uint8_t part);
uint8_t  vfs_opendir(dh_t *dh, path_t *dir);
int8_t   vfs_readdir(dh_t *dh, cbmdirent_t *dent);
void     vfs_read_sector(buffer_t *buf, uint8_t part, uint8_t track, uint8_t sector);
void     vfs_write_sector(buffer_t *buf, uint8_t part, uint8_t track, uint8_t sector);
void     format_dummy(uint8_t drive, uint8_t *name, uint8_t *id);

extern const fileops_t vfsops;

#endif
