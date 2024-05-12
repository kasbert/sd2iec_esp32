/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2007-2022  Ingo Korb <ingo@akana.de>
   ASCII/PET conversion Copyright (C) 2008 Jim Brain <brain@jbrain.com>

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


   vfsops.c: Posix/Virtual FS operations

*/

#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_check.h>


#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "buffers.h"
#include "d64ops.h"
#include "diskchange.h"
#include "diskio.h"
#include "display.h"
#include "doscmd.h"
#include "errormsg.h"
#include "fileops.h"
#include "flags.h"
#include "led.h"
#include "m2iops.h"
#include "p00cache.h"
#include "parser.h"
#include "progmem.h"
#include "uart.h"
#include "utils.h"
#include "ustring.h"
#include "wrapops.h"
#include "vfsops.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <rom/crc.h>
#include <dirent.h>

static const char *TAG = "vfsops";

#define P00_HEADER_SIZE       26
#define P00_CBMNAME_OFFSET    8
#define P00_RECORDLEN_OFFSET  25

#define BOOTSECTOR_FILE       "bootsect.128"

static const PROGMEM char p00marker[] = "C64File";
#define P00MARKER_LENGTH 7

typedef enum { EXT_UNKNOWN, EXT_IS_X00, EXT_IS_TYPE } exttype_t;

/* ------------------------------------------------------------------------- */
/*  Utility functions                                                        */
/* ------------------------------------------------------------------------- */

/**
 * parse_error - translates errno into a commodore error message
 * @res     : errno to be translated
 * @readflag: Flags if it was a read operation
 *
 * This function sets the error channel according to the problem given in
 * res. readflag specifies if a READ ERROR or WRITE ERROR should be used
 * if the error is FR_RW_ERROR.
 */
void parse_error(int res, uint8_t readflag) {
  switch (res) {
  case 0:
    set_error(ERROR_OK);
    break;

  case ENOENT:
    set_error_ts(ERROR_FILE_NOT_FOUND,res,0);
    break;

  case EACCES:
  case EPERM:
    set_error_ts(ERROR_WRITE_PROTECT,res,0);
    break;

  case EEXIST:
    set_error_ts(ERROR_FILE_EXISTS,res,0);
    break;

  case EBUSY:
  case EFAULT:
  //case EINVAL:
    set_error_ts(ERROR_DRIVE_NOT_READY,res,0);
    break;

  //case ENOSPC:

  //case EISDIR:
  case EBADF:
  //case ENOTDIR:
    set_error_ts(ERROR_DRIVE_NOT_READY,res,0);
    break;

#if __FIXMME
    set_error_ts(ERROR_DISK_FULL,res,0);
    break;
    set_error_ts(ERROR_FILE_NOT_FOUND_39,res,0);
    set_error_ts(ERROR_SYNTAX_JOKER,res,0);
    /* Just a random READ ERROR */
    if (readflag)
      set_error_ts(ERROR_READ_NOHEADER,res,0);
    else
      set_error_ts(ERROR_WRITE_VERIFY,res,0);
    break;
    // FIXME: What do the CMD drives return when removing a non-empty directory?
    set_error_ts(ERROR_FILE_EXISTS,res,0);
    set_error_ts(ERROR_FILE_EXISTS,res,0);
    break;
#endif

  default:
    set_error_ts(ERROR_SYNTAX_UNABLE,res,99);
    break;
  }
}

/**
 * check_extension - check for known file-type-based name extensions
 * @name: pointer to the file name
 * @ext : pointer to pointer to the file extension
 *
 * This function checks if the given file name has an extension that
 * indicates a specific file type like PRG/SEQ/P00/S00/... The ext
 * pointer will be set to the first character of the extension if
 * any is present or NULL if not. Returns EXT_IS_X00 for x00,
 * EXT_IS_TYPE for PRG/SEQ/... or EXT_UNKNOWN for an unknown file extension.
 */
static exttype_t check_extension(char *name, char **ext) {
  uint8_t f,s,t;

  /* Search for the file extension */
  if ((*ext = strrchr(name, '.')) != NULL) {
    f = toupper(*(++(*ext)));
    s = toupper(*(*ext+1));
    t = toupper(*(*ext+2));
    if ((f == 'P' || f == 'S' ||
         f == 'U' || f == 'R') &&
        isdigit(s) && isdigit(t))
      return EXT_IS_X00;
    else if ((f=='P' && s == 'R' && t == 'G') ||
             (f=='S' && s == 'E' && t == 'Q') ||
             (f=='R' && s == 'E' && t == 'L') ||
             (f=='U' && s == 'S' && t == 'R'))
      return EXT_IS_TYPE;
  }
  return EXT_UNKNOWN;
}

/**
 * should_save_raw - check if the file should be saved header-free
 * @name: pointer to the file name
 *
 * This function checks if the file of the given name should be saved
 * without any header for improved PC compatiblity.
 */
static bool should_save_raw(char* name) {
  if (check_imageext((uint8_t *)name) != IMG_UNKNOWN)
    return true;

  char* ext = strrchr(name, '.');
  if (ext == NULL)
    return false;

  ext++;

  if (*ext == 't' || *ext == 'T')
    ext++;

  if (ustrlen(ext) != 3)
    return false;

  char ucext[3];

  for (int i = 0; i < 3; i++) {
    ucext[i] = toupper(*ext++);
  }

  if (ucext[0] == 'C' && ucext[1] == 'R' && ucext[2] == 'T')
    return true;

  return false;
}

/**
 * is_valid_vfs_char - checks if a character is valid on FAT
 * @c: character to check
 *
 * This function checks if @c is a valid character for a FAT
 * file name. Returns true if it is, false if not.
 */
static bool is_valid_vfs_char(const uint8_t c) {
  if (isalnum(c) || c == '!' || c == ' ' ||
      (c >= '#' && c <= ')') ||
      c == '-' || c == '.')
    return true;
  else
    return false;
}

/**
 * is_valid_vfs_name - checks if a file name is valid on FAT
 * @name: name to check
 *
 * This function checks if @name is a valid name for a FAT
 * file. Returns true if it is, false if not.
 */
static bool is_valid_vfs_name(const char *name) {
  const char *ptr = name;
  unsigned char dots = 0;

  /* check all characters for validity */
  while (*ptr) {
    if (*ptr == '.')
      dots++;

    if (!is_valid_vfs_char(*ptr++))
      return false;
  }

  if (dots > 1)
    return false;

  /* check the last character */
  ptr--;

  if (*ptr == ' ')
    return false;

  if (*ptr == '.')
    return false;

  return true;
}

/**
 * build_name - convert PETSCII file name to valid FAT name
 * @name: pointer to a PETSCII file name to be converted
 * @type: file type
 *
 * This function converts a PETSCII file name to a suitable
 * FAT file name in-place. Returns a pointer to the last
 * character of the PC64 file extension if it was
 * created or NULL if not.
 */
static char* build_name(char *name, uint8_t type) {
  /* convert to PETSCII */
  pet2asc((uint8_t *)name);

#ifdef CONFIG_M2I
  /* do not add a header for raw files, even if the name may be invalid */
  if (type == TYPE_RAW)
    return NULL;
#endif

  /* known disk-image extensions are always without header or suffix */
  if (type == TYPE_PRG && should_save_raw(name))
    return NULL;

  /* PC64 mode or invalid FAT name? */
  if ((file_extension_mode == 1 && type != TYPE_PRG) ||
      file_extension_mode == 2 ||
      !is_valid_vfs_name(name)) {

      char *x00ext = NULL;

      /* Append .[PSUR]00 suffix to the file name */
      while (*name) {
        if (is_valid_vfs_char(*name)) {
          name++;
        } else {
          *name++ = '_';
        }
      }
      *name++ = '.';
      *name++ = pgm_read_byte(filetypes+3*type);
      *name++ = '0';
      x00ext = name;
      *name++ = '0';
      *name   = 0;

      return x00ext;
  }

  /* type-suffix mode? */
  if ((file_extension_mode == 3 && type != TYPE_PRG) ||
      (file_extension_mode == 4)) {
    /* Append type suffix to the file name */
    while (*name) name++;
    *name++ = '.';
    memcpy_P(name, filetypes + TYPE_LENGTH * (type & EXT_TYPE_MASK), TYPE_LENGTH);
    *(name+3) = 0;

    return NULL;
  }

  /* extension mode 0 and no special case */
  return NULL;
}

static off_t vfs_size(int fd) {
  int res;
  struct stat statbuf;

  res = fstat(fd, &statbuf);
  if (res) {
    // Should not happen as we have already opened the file
//printf("HELLO ERROR FSTAT\n");
    return -1;
  }
  return statbuf.st_size;
}

static off_t vfs_tell(int fd) {
  off_t    currpos;
  currpos = lseek(fd, 0, SEEK_CUR);
  return currpos;
}

static void vfs_path_dent(char *buffer, path_t *path, cbmdirent_t *dent) {
  strcpy (buffer, partition[path->part].base_path);
  strcat (buffer, "/");
  strcat (buffer, path->dir.pathname);
  strcat (buffer, "/");

  if (dent->pvt.vfs.realname[0])
    strcat (buffer, (char*)dent->pvt.vfs.realname);
  else {
    char *p = buffer + strlen(buffer);
    strcat (buffer, (char*)dent->name);
    pet2asc((uint8_t*)p);
  }
}

static void vfs_path(char *buffer, path_t *path, char *name) {
  strcpy (buffer, partition[path->part].base_path);
  strcat (buffer, "/");
  strcat (buffer, path->dir.pathname);
  strcat (buffer, "/");
  strcat (buffer, name);
}

static uint8_t _vfs_chdir(path_t *path, char *name) {
  char *pathname = path->dir.pathname;
  if (name[0] == '.' && name[1] == 0) {
    return 0;
  }
  if (name[0] == '.' && name[1] == '.' && name[2] == 0) {
    char *p = strrchr(pathname, '/');
    if (p) {
      *p = 0;
    } else {
      pathname[0] = 0;
    }
    return 0;
  }
  if (name[0] == 0 || (name[0] == '/' && name[1] == 0)) {
    pathname[0] = 0;
    return 0;
  }
  if (pathname[0]) {
    strcat(pathname, "/");
  }
  strcat(pathname, name);
//printf("_vfs_chdir %s CWD IS NOW '%s'\n", name, pathname);
  // FIXME check target
  return 0;
}

static int vfs_open(path_t *path, cbmdirent_t *dent, mode_t mode) {
  char buffer[512]; // FIXME
  vfs_path_dent(buffer, path, dent);
//printf("VFS_OPEN HELLO %s\n", buffer);
  return open(buffer, mode);
}

/* ------------------------------------------------------------------------- */
/*  Callbacks                                                                */
/* ------------------------------------------------------------------------- */

/**
 * vfs_file_read - read the next data block into the buffer
 * @buf: buffer to be worked on
 *
 * This function reads the next block of data from the associated file into
 * the given buffer. Used as a refill-callback when reading files
 */
static uint8_t vfs_file_read(buffer_t *buf) {
  ssize_t bytesread;
  size_t len;

  uart_putc('#');

  len = (buf->recordlen ? buf->recordlen : 254);
  bytesread = read(buf->pvt.vfs.fd, buf->data+2, len);
  if (bytesread < 0) {
    parse_error(errno, 1);
    free_buffer(buf);
    return 1;
  }

  /* The bus protocol can't handle 0-byte-files */
  if (bytesread == 0) {
    bytesread = 1;
    /* Experimental data suggests that this may be correct */
    buf->data[2] = (buf->recordlen ? 255 : 13);
  }

  buf->position = 2;
  buf->lastused = bytesread+1;
  if(buf->recordlen) // strip nulls from end of REL record.
    while(!buf->data[buf->lastused] && --(buf->lastused) > 1);

  if (bytesread < 254
      || (vfs_size(buf->pvt.vfs.fd) - vfs_tell(buf->pvt.vfs.fd)) == 0
      || buf->recordlen
     ) {
    buf->sendeoi = 1;
  } else
    buf->sendeoi = 0;

  return 0;
}

/**
 * write_data - write the current buffer data
 * @buf: buffer to be worked on
 *
 * This function writes the current contents of the given buffer into its
 * associated file.
 */
static uint8_t write_data(buffer_t *buf) {
  ssize_t byteswritten;

  uart_putc('/');

  if(!buf->mustflush)
    buf->lastused = buf->position - 1;

  if(buf->recordlen > buf->lastused - 1)
    memset(buf->data + buf->lastused + 1,0,buf->recordlen - (buf->lastused - 1));

  if(buf->recordlen)
    buf->lastused = buf->recordlen + 1;

  size_t count = buf->lastused-1;
  byteswritten = write(buf->pvt.vfs.fd, buf->data+2, count);
  if (byteswritten < 0) {
    uart_putc('r');
    parse_error(errno,1);
    close(buf->pvt.vfs.fd);
    free_buffer(buf);
    return 1;
  }

  if (byteswritten != buf->lastused-1U) {
    uart_putc('l');
    set_error(ERROR_DISK_FULL);
    close(buf->pvt.vfs.fd);
    free_buffer(buf);
    return 1;
  }

  mark_buffer_clean(buf);
  buf->mustflush = 0;
  buf->position  = 2;
  buf->lastused  = 2;
  buf->fptr      = vfs_tell(buf->pvt.vfs.fd) - buf->pvt.vfs.headersize;

  return 0;
}

/**
 * vfs_file_write - refill-callback for files opened for writing
 * @buf: target buffer
 *
 * This function writes the contents of buf to the associated file.
 */
static uint8_t vfs_file_write(buffer_t *buf) {
  uint32_t i = 0;

  off_t fsize = vfs_size(buf->pvt.vfs.fd);
  uint32_t fptr = fsize - buf->pvt.vfs.headersize;

  // on a REL file, the fptr will be be at the end of the record we just read.  Reposition.
  if (buf->fptr != fptr) {
    off_t offset = lseek(buf->pvt.vfs.fd, buf->pvt.vfs.headersize + buf->fptr, SEEK_SET);
    if (offset < 0) {
      parse_error(errno,1);
      close(buf->pvt.vfs.fd);
      free_buffer(buf);
      return 1;
    }
  }

  if(buf->fptr > fptr)
    i = buf->fptr - fptr;

  if (write_data(buf))
    return 1;

  if(i) {
    // we need to fill bytes.
    // position to old end of file.
    off_t offset = lseek(buf->pvt.vfs.fd, buf->pvt.vfs.headersize + fptr, SEEK_SET);
    buf->mustflush = 0;
    buf->fptr = fptr;
    buf->data[2] = (buf->recordlen?255:0);
    memset(buf->data + 3,0,253);
    while(offset < 0) {
      if (buf->recordlen)
        buf->lastused = buf->recordlen;
      else
        buf->lastused = (i>254 ? 254 : (uint8_t) i);

      i -= buf->lastused;
      buf->position = buf->lastused + 2;

      if(write_data(buf))
        return 1;
    }
    // TODO SEEK_END
    //offset = lseek(buf->pvt.vfs.fd, vfs_size(buf->pvt.vfs.fd), SEEK_SET);
    offset = lseek(buf->pvt.vfs.fd, 0, SEEK_END);
    if (offset < 0) {
      uart_putc('r');
      parse_error(errno,1);
      close(buf->pvt.vfs.fd);
      free_buffer(buf);
      return 1;
    }
    buf->fptr = vfs_tell(buf->pvt.vfs.fd) - buf->pvt.vfs.headersize;
  }

  return 0;
}

/**
 * vfs_file_seek - callback for seek
 * @buf     : buffer to be worked on
 * @position: offset to seek to
 * @index   : offset within the record to seek to
 *
 * This function seeks to the offset position in the file associated
 * with the given buffer and sets the read pointer to the byte given
 * in index, effectively seeking to (position+index) for normal files.
 * Returns 1 if an error occured, 0 otherwise.
 */
uint8_t vfs_file_seek(buffer_t *buf, uint32_t position, uint8_t index) {
  uint32_t pos = position + buf->pvt.vfs.headersize;

  if (buf->dirty)
    if (vfs_file_write(buf))
      return 1;

  off_t fsize = vfs_size(buf->pvt.vfs.fd);
  if (fsize >= pos) {
    off_t offset = lseek(buf->pvt.vfs.fd, pos, SEEK_SET);
    if (offset < 0) {
      parse_error(errno,0);
      close(buf->pvt.vfs.fd);
      free_buffer(buf);
      return 1;
    }

    if (vfs_file_read(buf))
      return 1;
  } else {
    buf->data[2]  = (buf->recordlen ? 255:13);
    buf->lastused = 2;
    buf->fptr     = position;
    set_error(ERROR_RECORD_MISSING);
  }

  buf->position = index + 2;
  if(index + 2 > buf->lastused)
    buf->position = buf->lastused;

  return 0;
}

/**
 * vfs_file_sync - synchronize the current REL file.
 * @buf: buffer to be worked on
 *
 */
static uint8_t vfs_file_sync(buffer_t *buf) {
  return vfs_file_seek(buf,buf->fptr + buf->recordlen,0);
}

/**
 * vfs_file_close - close the file associated with a buffer
 * @buf: buffer to be worked on
 *
 * This function closes the file associated with the given buffer. If the buffer
 * was opened for writing the data contents will be stored if required.
 * Additionally the buffer will be marked as free.
 * Used as a cleanup-callback for reading and writing.
 */
static uint8_t vfs_file_close(buffer_t *buf) {
  int res;

  if (!buf->allocated) return 0;

  if (buf->write) {
    /* Write the remaining data using the callback */
    if (buf->refill(buf))
      return 1;
  }

  res = close(buf->pvt.vfs.fd);
  buf->pvt.vfs.fd = -1;
  parse_error(errno,1);
  buf->cleanup = callback_dummy;

  if (res < 0)
    return 1;
  else
    return 0;
}

/* ------------------------------------------------------------------------- */
/*  Internal handlers for the various operations                             */
/* ------------------------------------------------------------------------- */

/**
 * vfs_open_read - opens a file for reading
 * @path: path of the file
 * @dent: pointer to cbmdirent with name of the file
 * @buf : buffer to be used
 *
 * This functions opens a file in the FAT filesystem for reading and sets up
 * buf to access it.
 */

void vfs_open_read(path_t *path, cbmdirent_t *dent, buffer_t *buf) {
  int fd = vfs_open(path, dent, O_RDONLY);
  if (fd < 0) {
    parse_error(errno,1);
    return;
  }
  buf->pvt.vfs.fd = fd;

  if (dent->opstype == OPSTYPE_VFS_X00) {
    /* It's a [PSUR]00 file, skip the header */
    /* If anything goes wrong here, refill will notice too */
    lseek(buf->pvt.vfs.fd, P00_HEADER_SIZE, SEEK_SET);
    buf->pvt.vfs.headersize = P00_HEADER_SIZE;
  }

  buf->read      = 1;
  buf->cleanup   = vfs_file_close;
  buf->refill    = vfs_file_read;
  buf->seek      = vfs_file_seek;

  stick_buffer(buf);

  /* Call the refill once for the first block of data */
  buf->refill(buf);
}

/**
 * create_file - creates a file
 * @path     : path of the file
 * @dent     : name of the file
 * @type     : type of the file
 * @buf      : buffer to be used
 * @recordlen: length of record, if REL file.
 *
 * This function opens a file in the FAT filesystem for writing and sets up
 * buf to access it. type is ignored here because FAT has no equivalent of
 * file types.
 */
int create_file(path_t *path, cbmdirent_t *dent, uint8_t type, buffer_t *buf, uint8_t recordlen) {
  char *x00ext = 0;

  /* check if the FAT name is already defined (used only for M2I) */
#ifdef CONFIG_M2I
  if (dent->pvt.vfs.realname[0])
    name = dent->pvt.vfs.realname;
  else
#endif
  {
    ustrcpy(dent->pvt.vfs.realname, dent->name);
    x00ext = build_name(dent->pvt.vfs.realname, type);
  }
  int fd = vfs_open(path, dent, O_CREAT | O_EXCL | O_RDWR);
  while (x00ext != NULL && fd < 0) {
    /* File exists, increment extension */
    *x00ext += 1;
    if (*x00ext == '9'+1) {
      *x00ext = '0';
      *(x00ext-1) += 1;
      if (*(x00ext-1) == '9'+1)
        break;
    }
    fd = vfs_open(path, dent, O_CREAT | O_EXCL | O_RDWR);
  }

  if (fd < 0)
    return fd;
  buf->pvt.vfs.fd = fd;

  if (x00ext != NULL || recordlen) {
    ssize_t byteswritten;

    if(x00ext != NULL) {
      /* Write a [PSUR]00 header */

      memset(ops_scratch, 0, P00_HEADER_SIZE);
      ustrcpy_P(ops_scratch, p00marker);
      memcpy(ops_scratch+P00_CBMNAME_OFFSET, dent->name, CBM_NAME_LENGTH);
      if(recordlen)
        ops_scratch[P00_RECORDLEN_OFFSET] = recordlen;
      buf->pvt.vfs.headersize = P00_HEADER_SIZE;
    } else if(recordlen) {
      ops_scratch[0] = recordlen;
      buf->pvt.vfs.headersize = 1;
    }
    byteswritten = write(buf->pvt.vfs.fd, ops_scratch, buf->pvt.vfs.headersize);
    if (byteswritten != buf->pvt.vfs.headersize) {
      close(buf->pvt.vfs.fd);
      buf->pvt.vfs.fd = -1;
      // TODO unlink file
      return -1;
    }
  }

  return fd;
}

/**
 * vfs_open_write - opens a file for writing
 * @path  : path of the file
 * @dent  : name of the file
 * @type  : type of the file
 * @buf   : buffer to be used
 * @append: Flags if the new data should be appended to the end of file
 *
 * This function opens a file in the FAT filesystem for writing and sets up
 * buf to access it. type is ignored here because FAT has no equivalent of
 * file types.
 */
void vfs_open_write(path_t *path, cbmdirent_t *dent, uint8_t type, buffer_t *buf, uint8_t append) {
  int fd;

  if (append) {
    fd = vfs_open(path, dent, O_WRONLY);
    if (fd >= 0) {
      if (dent->opstype == OPSTYPE_VFS_X00)
        /* It's a [PSUR]00 file */
        buf->pvt.vfs.headersize = P00_HEADER_SIZE;
      off_t fsize = vfs_size(buf->pvt.vfs.fd);
      //off_t res = lseek(fd, fsize, SEEK_SET);
      //off_t res = 
      lseek(buf->pvt.vfs.fd, 0, SEEK_END);
      buf->fptr = fsize - buf->pvt.vfs.headersize;
    }
  } else
    fd = create_file(path, dent, type, buf, 0);

  if (fd < 0) {
    parse_error(errno,0);
    return;
  }
  buf->pvt.vfs.fd = fd;

  mark_write_buffer(buf);
  buf->position  = 2;
  buf->lastused  = 2;
  buf->cleanup   = vfs_file_close;
  buf->refill    = vfs_file_write;
  buf->seek      = vfs_file_seek;

  /* If no data is written the file should end up with a single 0x0d byte */
  buf->data[2] = 13;
}

/**
 * vfs_open_rel - creates a rel file.
 * @path  : path of the file
 * @dent  : name of the file
 * @buf   : buffer to be used
 * @length: record length
 * @mode  : select between new or existing file
 *
 * This function opens a rel file and prepares it for access.
 * If the mode parameter is 0, create a new file. If it is != 0,
 * open an existing file.
 */
void vfs_open_rel(path_t *path, cbmdirent_t *dent, buffer_t *buf, uint8_t length, uint8_t mode) {
  int fd;
  ssize_t bytesread = 1;

  if(!mode) {
    fd = create_file(path, dent, TYPE_REL, buf, length);
    ops_scratch[0] = length;
  } else {
    fd = vfs_open(path, dent, O_RDWR);
    if (fd >= 0) {
      if (dent->opstype == OPSTYPE_VFS_X00) {
        off_t res = lseek(fd, P00_RECORDLEN_OFFSET, SEEK_SET);
        if (res >= 0)
          /* read record length */
          bytesread = read(fd, ops_scratch, 1);
        length = ops_scratch[0];
      }
    }
  }

  if (fd < 0 || bytesread != 1) {
    parse_error(errno,0);
    return;
  }
  buf->pvt.vfs.fd = fd;

  buf->pvt.vfs.headersize = (uint8_t)vfs_tell(buf->pvt.vfs.fd);
  buf->recordlen  = length;
  mark_write_buffer(buf);
  buf->read      = 1;
  buf->cleanup   = vfs_file_close;
  buf->refill    = vfs_file_sync;
  buf->seek      = vfs_file_seek;

  /* read the first record */
  if (!vfs_file_read(buf) && length != ops_scratch[0])
    set_error(ERROR_RECORD_MISSING);
}

/* ------------------------------------------------------------------------- */
/*  External interface for the various operations                            */
/* ------------------------------------------------------------------------- */

uint8_t vfs_opendir(dh_t *dh, path_t *path) {
  char buffer[512]; // FIXME
  vfs_path(buffer, path, "");
  DIR *dirp = opendir(buffer);
//printf ("OPENDIR %p part %d '%s' %p\n", dh, path->part, buffer, dirp);
  if (!dirp) {
    parse_error(errno,1);
    return 1;
  }
  dh->part = path->part;
  dh->dir.vfs.dirp = dirp;
  strcpy(dh->dir.vfs.pathname, buffer);
  return 0;
}

/**
 * vfs_readdir - readdir wrapper for FAT
 * @dh  : directory handle as set up by opendir
 * @dent: CBM directory entry for returning data
 *
 * This function reads the next directory entry into dent.
 * Returns 1 if an error occured, -1 if there are no more
 * directory entries and 0 if successful.
 */
int8_t vfs_readdir(dh_t *dh, cbmdirent_t *dent) {
  struct dirent *de;

  do {
    errno = 0;
    de = readdir(dh->dir.vfs.dirp);
    if (!de) {
//printf("readdir %p %p\n", dh->dir.vfs.dirp, de);
      if (errno) {
        parse_error(errno,1);
        return -1;
      }
      return -1;
    }
//printf("readdir %p %p '%s'\n", dh->dir.vfs.dirp, de, de->d_name);
  } while ((de->d_name[0] == '.' && de->d_name[1] == 0) ||
           (de->d_name[0] == '.' && de->d_name[1] == '.' && de->d_name[2] == 0));

  struct stat statbuf;
  char buffer[512]; // FIXME
  //vfs_path(buffer, dh->dir.path, de->d_name);
  strcpy(buffer, dh->dir.vfs.pathname);
  strcat(buffer, "/");
  strcat(buffer, de->d_name);
  int res = stat(buffer, &statbuf);
  if (res) {
//printf("HELLO ERROR FSTAT\n");
    return -1;
  }
  off_t fsize = statbuf.st_size;

  memset(dent, 0, sizeof(cbmdirent_t));
  dent->opstype = OPSTYPE_VFS;
  ustrcpy(dent->pvt.vfs.realname, de->d_name);

  uint8_t nameptr[256];
  ustrcpy(nameptr, de->d_name);
  asc2pet(nameptr);
  //ESP_LOGI(TAG, "HELLO '%s' '%s'",nameptr,de->d_name);

  /* File type */
  uint8_t typechar;
  if (de->d_type == DT_DIR) {
    dent->typeflags = TYPE_DIR;

  } else {
    char *ptr;
    /* Search for the file extension */
    exttype_t ext = check_extension(de->d_name, &ptr);
    if (ext == EXT_IS_X00) {
      /* [PSRU]00 file - try to read the internal name */
      uint32_t crc = crc32_le(0, (uint8_t*)buffer, strlen(de->d_name));
      uint8_t *name = p00cache_lookup(dh->part, crc);
      typechar = *ptr;

      if (name != NULL) {
        /* lookup successful */
        memcpy(nameptr, name, CBM_NAME_LENGTH);
      } else {
        path_t path;
        path.part = dh->part;
        /* read name from file */
        int fd = vfs_open(&path, dent, O_RDONLY);
        if (fd < 0)
          goto notp00;
          //partition[dh->part].imagefd= res;

        off_t bytesread = read(fd, ops_scratch, P00_HEADER_SIZE);
        close(fd);
        if (bytesread != P00_HEADER_SIZE)
          goto notp00;

        if (memcmp_P(ops_scratch, p00marker, P00MARKER_LENGTH))
          goto notp00;

        /* Copy the internal name - dent->name is still zeroed */
        ustrcpy(nameptr, ops_scratch + P00_CBMNAME_OFFSET);

        /* Some programs pad the name with 0xa0 instead of 0 */
        ptr = (char*)nameptr;
        for (uint8_t i=0;i<16;i++,ptr++)
          if (*ptr == 0xa0)
            *ptr = 0;

        /* add name to cache */
        // FIXME is 0 correct ? should be crc ?
        uint32_t crc = crc32_le(0, (uint8_t*)buffer, ustrlen(nameptr));
        p00cache_add(dh->part, crc, nameptr);
      }
      fsize -= P00_HEADER_SIZE;
      dent->opstype = OPSTYPE_VFS_X00;

    } else if (ext == EXT_IS_TYPE && (globalflags & EXTENSION_HIDING)) {
      /* Type extension */
      typechar = toupper(*ptr);
      uint8_t i = ustrlen(nameptr)-4;
      nameptr[i] = 0;

    } else { /* ext == EXT_UNKNOWN or EXT_IS_TYPE but hiding disabled */
      /* Unknown extension: PRG */
      typechar = 'P';
    }

  notp00:
    /* Set the file type */
    switch (typechar) {
    case 'P':
      dent->typeflags = TYPE_PRG;
      break;

    case 'S':
      dent->typeflags = TYPE_SEQ;
      break;

    case 'U':
      dent->typeflags = TYPE_USR;
      break;

    case 'R':
      dent->typeflags = TYPE_REL;
      break;
    }
  }

  /* Copy file name into dirent if it fits */
  ustrncpy(dent->name, nameptr, CBM_NAME_LENGTH);
  /*
  uint8_t *ptr = dent->name;
  while (*ptr) {
    if (*ptr == '~') *ptr = 0xff;
    ptr++;
  }
  */

  if (fsize > 16255746)
    /* File too large -> size 63999 blocks */
    dent->blocksize = 63999;
  else
    dent->blocksize = (fsize+253) / 254;

  dent->remainder = fsize % 254;

  /* Hide files/directories starting with . */
  if (*nameptr == '.')
    dent->typeflags |= FLAG_HIDDEN;

  if (check_imageext((uint8_t *)de->d_name) != IMG_UNKNOWN) {
    dent->typeflags |= FLAG_IMAGE;
  }

  /* Read-Only and hidden flags */
#if _FIXME
  if (finfo.fattrib & AM_RDO)
    dent->typeflags |= FLAG_RO;

  /* Date/Time */
  statbuf.st_mtime...
  dent->date.year  = (finfo.fdate >> 9) + 80;
  dent->date.month = (finfo.fdate >> 5) & 0x0f;
  dent->date.day   = finfo.fdate & 0x1f;

  dent->date.hour   = finfo.ftime >> 11;
  dent->date.minute = (finfo.ftime >> 5) & 0x3f;
  dent->date.second = (finfo.ftime & 0x1f) << 1;
#endif

  return 0;
}

/**
 * vfs_delete - Delete a file/directory on FAT
 * @path: path to the file/directory
 * @dent: pointer to cbmdirent with name of the file/directory to be deleted
 *
 * This function deletes the file filename in path and returns
 * 0 if not found, 1 if deleted or 255 if an error occured.
 */
uint8_t vfs_delete(path_t *path, cbmdirent_t *dent) {

  set_dirty_led(1);
  p00cache_invalidate();

  char buffer[512]; // FIXME
  vfs_path_dent(buffer, path, dent);
  int res = unlink((char*)buffer);
  update_leds();

  parse_error(errno,0);
  if (res < 0)
    return 1;
  //else if (res == FR_NO_FILE)
  //  return 0;
  else
    return 255;
}

/**
 * vfs_chdir - change directory in FAT and/or mount image
 * @path: path object for the location of dirname
 * @dent: Name of the directory/image to be changed into
 *
 * This function changes the directory of the path object to dirname.
 * If dirname specifies a file with a known extension (e.g. M2I or D64), the
 * current(!) directory will be changed to the directory of the file and
 * it will be mounted as an image file. Returns 0 if successful,
 * 1 otherwise.
 */

uint8_t vfs_chdir(path_t *path, cbmdirent_t *dent) {

//printf("vfs_chdir DIR %s \n", dent->name);
  uint8_t res;
  /* Left arrow moves one directory up */
  if (dent->name[0] == '_' && dent->name[1] == 0) {
    res = _vfs_chdir(path, "..");
    dent->typeflags = TYPE_DIR;
    return 0;
  }
  if (dent->name[0] == 0) {
    /* Empty string moves to the root dir */
    //path->dir.name[0] = 0;
    res = _vfs_chdir(path, "");
    return 0;
  }

  if ((dent->typeflags & TYPE_MASK) == TYPE_DIR) {
    /* It's a directory, change to it */
    res = _vfs_chdir(path, (char*)dent->pvt.vfs.realname);
    if (res) {
//printf("vfs_chdir NOOOO %s \n", (char*)dent->pvt.vfs.realname);
      parse_error(ERROR_SYNTAX_UNABLE,1);
      return 1;
    }
    return 0;
  }
//printf("vfs_chdir NO DIR %s \n", dent->name);
  /* Changing into a file, could be a mount request */
  if (!(dent->typeflags & FLAG_IMAGE)) {
    return 0;
  }
  /* D64/M2I mount request */
  free_multiple_buffers(FMB_USER_CLEAN);
  /* Open image file */
  int fd = vfs_open(path, dent, O_RDWR);
  partition[path->part].flag = 0;
  /* Try to open read-only if medium or file is read-only */
  if (fd < 0) {
    fd = vfs_open(path, dent, O_RDONLY);
    partition[path->part].flag = FLAG_RO;
  }
  if (fd < 0) {
    parse_error(errno,1);
    return 1;
  }

#ifdef CONFIG_M2I
  if (check_imageext(dent->pvt.vfs.realname) == IMG_IS_M2I)
    partition[path->part].fop = &m2iops;
    partition[path->part].parent_fop = &vfsops;
  else
#endif
    {
      uint32_t fsize = vfs_size(fd);
      if (d64_mount(path, (uint8_t *)dent->pvt.vfs.realname, fsize)) {
        close(fd);
        return 1;
      }
      partition[path->part].fop = &d64ops;
      partition[path->part].parent_fop = &vfsops;
    }
  partition[path->part].imagefd = fd;
  return 0;
}

/* Create a new directory */
void vfs_mkdir(path_t *path, uint8_t *dirname) {
  pet2asc(dirname);
  char buffer[512]; // FIXME
  vfs_path(buffer, path, (char*)dirname);
  int res = mkdir(buffer, 0);
  if (res)
    parse_error(errno,0);
}

/**
 * vfs_getvolumename - Get the volume label
 * @part : partition to request
 * @label: pointer to the buffer for the label (16 characters+zero-termination)
 *
 * This function reads the FAT volume label and stores it zero-terminated
 * in label. Returns 0 if successfull, != 0 if an error occured.
 */
static uint8_t vfs_getvolumename(uint8_t part, uint8_t *label) {
  uint8_t *name = partition[part].base_path + 1;
  memset(label, ' ', CBM_NAME_LENGTH+1);
  memcpy(label, name, ustrlen(name));
  asc2pet(label);
  // TODO
  /*
  int res = f_getlabel("", (char*)label, 0);
  if (res != FR_OK) {
    parse_error(ERROR_SYNTAX_UNABLE,0);
    return 1;
  }
  */
  return 0;
}

/**
 * vfs_getdirlabel - Get the directory label
 * @path : path object of the directory
 * @label: pointer to the buffer for the label (16 characters)
 *
 * This function reads the FAT volume label (if in root directory) or FAT
 * directory name (if not) and stored it space-padded
 * in the first 16 bytes of label.
 * Returns 0 if successfull, != 0 if an error occured.
 */
uint8_t vfs_getdirlabel(path_t *path, uint8_t *label) {
  memset(label, ' ', CBM_NAME_LENGTH);
  char *pathname = path->dir.pathname;
  if (pathname[0]) {
    uint8_t *name = ustrrchr((uint8_t *)pathname, '/');
    if (!name) {
      name = (uint8_t *)pathname;
    }
    memcpy(label, name, ustrlen(name));
    asc2pet(label);
    return 0;
  }
  return vfs_getvolumename(path->part, label);
}

/**
 * vfs_getid - "Read" a disk id
 * @path: path object
 * @id  : pointer to the buffer for the id (5 characters)
 *
 * This function creates a disk ID from the FAT type (12/16/32)
 * and the usual " 2A" of a 1541 in the first 5 bytes of id.
 * Always returns 0 for success.
 */
uint8_t vfs_getid(path_t *path, uint8_t *id) {
  // FIXME find correct values
  // "fat32" - who cares
    *id++ = '3';
    *id++ = '2';

  *id++ = ' ';
  *id++ = '2';
  *id++ = 'A';
  return 0;
}

/* Returns the number of free blocks */
uint16_t vfs_freeblocks(uint8_t part) {
  // FIXME
  uint64_t esp32fs_get_bytes_free(const char *mount_point);
  uint64_t freebytes = esp32fs_get_bytes_free(partition[part].base_path);
  uint32_t clusters = freebytes / 256; // FIXME

  if (clusters > 65535)
    return 65535;
  else
    return clusters;
}

/**
 * vfs_readwrite_sector - simulate direct sector access
 * @buf   : target buffer
 * @part  : partition number
 * @track : track to read
 * @sector: sector to read
 * @roflag: read only flag
 *
 * This function allows access to a file called bootsect.128
 * as track 1 sector 0 to enable the auto-boot function of
 * the C128 on FAT directories. If rwflag is false (0),
 * the sector will be written; otherwise it will be read.
 */
static void vfs_readwrite_sector(buffer_t *buf, uint8_t part,
                                 uint8_t track, uint8_t sector, uint8_t roflag) {
  int fd;
  UINT bytes;
  uint8_t mode;

  if (track != 1 || sector != 0) {
    set_error_ts(ERROR_READ_NOHEADER, track, sector);
    return;
  }

  if (roflag)
    mode = O_RDONLY;
  else
    mode = O_RDWR;

  fd = open((const char *)BOOTSECTOR_FILE, mode);
  if (fd < 0) {
    parse_error(errno, roflag);
    return;
  }

  if (roflag)
    bytes = read(fd, buf->data, 256);
  else
    bytes = write(fd, buf->data, 256);

  if (bytes != 256)
    parse_error(errno, roflag);

  if (close(fd) < 0)
    parse_error(errno, roflag);

  return;
}

/**
 * vfs_read_sector - simulate direct sector reads
 * @buf   : target buffer
 * @part  : partition number
 * @track : track to read
 * @sector: sector to read
 *
 * Wrapper for vfs_readwrite_sector in read mode
 */
void vfs_read_sector(buffer_t *buf, uint8_t part, uint8_t track, uint8_t sector) {
  vfs_readwrite_sector(buf, part, track, sector, 1);
}

/**
 * vfs_write_sector - simulate direct sector writes
 * @buf   : source buffer
 * @part  : partition number
 * @track : track to write
 * @sector: sector to write
 *
 * Wrapper for vfs_readwrite_sector in write mode
 */
void vfs_write_sector(buffer_t *buf, uint8_t part, uint8_t track, uint8_t sector) {
  vfs_readwrite_sector(buf, part, track, sector, 0);
}

/**
 * vfs_rename - rename a file
 * @path   : path object
 * @dent   : pointer to cbmdirent with old file name
 * @newname: new file name
 *
 * This function renames the file in dent in the directory referenced by
 * path to newname.
 */
void vfs_rename(path_t *path, cbmdirent_t *dent, uint8_t *newname) {
  ssize_t byteswritten;
  int res;

  if (dent->opstype == OPSTYPE_VFS_X00) {
    /* [PSUR]00 rename, just change the internal file name */
    p00cache_invalidate();

    int fd = vfs_open(path, dent, O_WRONLY);
    if (fd < 0) {
      parse_error(errno,0);
      return;
    }

    off_t res = lseek(fd, P00_CBMNAME_OFFSET, SEEK_SET);
    if (res < 0) {
      parse_error(errno,0);
      return;
    }

    /* Copy the new name into dent->name so we can overwrite all 16 bytes */
    memset(dent->name, 0, CBM_NAME_LENGTH);
    ustrcpy(dent->name, newname);

    byteswritten = write(fd, dent->name, CBM_NAME_LENGTH);
    if (byteswritten != CBM_NAME_LENGTH) {
      close(fd);
      parse_error(errno,0);
      return;
    }

    if (close(fd) < 0) {
      parse_error(errno,0);
      return;
    }
  } else {
    char *ext;
    switch (check_extension(dent->pvt.vfs.realname, &ext)) {
    case EXT_IS_TYPE:
      /* Keep type extension */
      ustrcpy(ops_scratch, newname);
      build_name((char*)ops_scratch, dent->typeflags & TYPE_MASK);
      // FIXME path
      res = rename(dent->pvt.vfs.realname, (char*)ops_scratch);
      if (res < 0)
        parse_error(errno, 0);
      break;

    default:
      /* Normal rename */
      pet2asc((uint8_t *)dent->name);
      pet2asc((uint8_t *)newname);
      char oldpath[512]; // FIXME
      vfs_path(oldpath, path, (char*)dent->name); // FIXME dent->pvt.vfs.realname ?
      char newpath[512]; // FIXME
      vfs_path(newpath, path, (char*)newname);
      res = rename(oldpath, newpath);
      if (res < 0)
        parse_error(errno, 0);
      break;
    }
  }
}

/**
 * vfsops_init - Initialize vfsops module
 * @preserve_path: Preserve the current directory if non-zero
 *
 * This function will initialize the vfsops module and force
 * mounting of the card. It can safely be called again if re-mounting
 * is required.
 */
 void vfsops_init(uint8_t preserve_path, const char *basepath) {
  //uint8_t realdrive,drive,part;
  //char logicaldrive[] = {'0',':', 0};
  memset(&(partition[max_part]), 0, sizeof(partition_t));

  partition[max_part].fop = &vfsops;
  partition[max_part].base_path = basepath;
  max_part++;

  if (!preserve_path) {
    current_part = 0;
    display_current_part(0);
#ifdef CONFIG_FIXME_CHANGELIST
    set_changelist(NULL, NULLSTRING);
#endif
    previous_file_dirent.name[0] = 0; // clear '*' file
  }

  /* Invalidate some caches */
  d64_invalidate();
  p00cache_invalidate();

#ifndef HAVE_HOTPLUG
  if (!max_part) {
    set_error_ts(ERROR_DRIVE_NOT_READY,0,0);
    return;
  }
#endif
}

/**
 * vfs_image_unmount - generic unmounting function for images
 * @part: partition number
 *
 * This function will clear all buffers, close the image file and
 * restore file operations to vfsops. It can be used for unmounting
 * any image file types that don't require special cleanups.
 * Returns 0 if successful, 1 otherwise.
 */
static uint8_t vfs_image_unmount(uint8_t part) {

  free_multiple_buffers(FMB_USER_CLEAN);

  /* call D64 unmount function to handle BAM refcounting etc. */
  // FIXME: ops entry?
  if (partition[part].fop == &d64ops)
    d64_unmount(part);

  if (display_found) {
    /* Send current path to display */
    path_t path;

    path.part    = part;
    vfs_getdirlabel(&path, ops_scratch);
    display_current_directory(part, ops_scratch);
  }

  partition[part].fop = &vfsops;
  int res = close(partition[part].imagefd);
  partition[part].imagefd = -1;
  if (res < 0) {
    parse_error(errno, 0);
    return 1;
  }
  return 0;
}

/**
 * image_chdir - generic chdir for image files
 * @path: path object of the location of dirname
 * @dent: directory to be changed into
 *
 * This function will ignore any names except _ (left arrow)
 * and unmount the image if that is found. It can be used as
 * chdir for all image types that don't support subdirectories
 * themselves. Returns 0 if successful, 1 otherwise.
 */
uint8_t image_chdir(path_t *path, cbmdirent_t *dent) {
  if (dent->name[0] == '_' && dent->name[1] == 0) {
    /* Unmount request */
    return image_unmount(path->part);
  }
  return 1;
}

/**
 * image_mkdir - generic mkdir for image files
 * @path   : path of the directory
 * @dirname: name of the directory to be created
 *
 * This function only sets an error message.
 */
void image_mkdir(path_t *path, uint8_t *dirname) {
  (void)path;
  (void)dirname;

  set_error(ERROR_SYNTAX_UNABLE);
  return;
}

/**
 * vfs_image_read - Seek to a specified image offset and read data
 * @part  : partition number
 * @offset: offset to be seeked to
 * @buffer: pointer to where the data should be read to
 * @bytes : number of bytes to read from the image file
 *
 * This function seeks to offset in the image file and reads bytes
 * byte into buffer. It returns 0 on success, 1 if less than
 * bytes byte could be read and 2 on failure.
 */
static uint8_t vfs_image_read(uint8_t part, DWORD offset, void *buffer, uint16_t bytes) {

  if (offset != (DWORD)-1) {
    off_t off = lseek(partition[part].imagefd, offset, SEEK_SET);
    if (off < 0) {
      parse_error(errno,1);
      return 2;
    }
  }

  ssize_t bytesread = read(partition[part].imagefd, buffer, bytes);
  if (bytesread < 0) {
    parse_error(errno,1);
    return 2;
  }

  if (bytesread != bytes)
    return 1;

  return 0;
}

/**
 * vfs_image_write - Seek to a specified image offset and write data
 * @part  : partition number
 * @offset: offset to be seeked to
 * @buffer: pointer to the data to be written
 * @bytes : number of bytes to read from the image file
 * @flush : Flags if written data should be flushed to disk immediately
 *
 * This function seeks to offset in the image file and writes bytes
 * byte into buffer. It returns 0 on success, 1 if less than
 * bytes byte could be written and 2 on failure.
 */
static uint8_t vfs_image_write(uint8_t part, DWORD offset, void *buffer, uint16_t bytes, uint8_t flush) {
  if (offset != (DWORD)-1) {
    off_t off = lseek(partition[part].imagefd, offset, SEEK_SET);
    if (off < 0) {
      parse_error(errno,0);
      return 2;
    }
  }

  ssize_t byteswritten = write(partition[part].imagefd, buffer, bytes);
  if (byteswritten < 0) {
    parse_error(errno,1);
    return 2;
  }

  if (byteswritten != bytes)
    return 1;

  if (flush)
    puts("FIXME sync(");// partition[part].imagefd

  return 0;
}

/* Dummy function for format */
void format_dummy(uint8_t drive, uint8_t *name, uint8_t *id) {
  (void)drive;
  (void)name;
  (void)id;

  set_error(ERROR_SYNTAX_UNKNOWN);
}

const PROGMEM fileops_t vfsops = {  // These should be at bottom, to be consistent with d64ops and m2iops
  &vfs_open_read,
  &vfs_open_write,
  &vfs_open_rel,
  &vfs_delete,
  &vfs_getvolumename,
  &vfs_getdirlabel,
  &vfs_getid,
  &vfs_freeblocks,
  &vfs_read_sector,
  &vfs_write_sector,
  &format_dummy,
  &vfs_opendir,
  &vfs_readdir,
  &vfs_mkdir,
  &vfs_chdir,
  &vfs_rename,
  &vfs_image_unmount,
  &vfs_image_read,
  &vfs_image_write,
};
