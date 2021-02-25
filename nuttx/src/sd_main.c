/****************************************************************************
 * boards/z80/ez80/ezpz/src/sd_main.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <hex2bin.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/fs/fs.h>
#include <nuttx/streams.h>

#include <arch/irq.h>

#include "ezpz.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MMCSD_BLOCKDEV   "/dev/mmcsd0"
#define MMCSD_MOUNTPT    "/mnt/sdcard"
#define MMCSD_HEXFILE    "/mnt/sdcard/nuttx.hex"

#define SRAM_START       0x040000
#define SRAM_SIZE        0x080000
#define SRAM_END         (SRAM_START + SRAM_SIZE)

#define SRAM_RESET       SRAM_START
#define SRAM_ENTRY       ((sram_entry_t)SRAM_START)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef CODE void (*sram_entry_t)(void);

struct cache_stdinstream_s
{
  struct lib_instream_s  public;
  int                    fd;
  char                   buffer[4096];
  size_t                 bufidx;
  size_t                 buflen;
  off_t                  flen;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int cache_stdinstream_getc(FAR struct lib_instream_s *this)
{
  FAR struct cache_stdinstream_s *sthis = (FAR struct cache_stdinstream_s *)this;
  int ret;

  if (sthis->bufidx < sthis->buflen)
    {
      ret = sthis->buffer[sthis->bufidx];
      sthis->bufidx++;
    }
  else
    {
      ssize_t avail;

      avail = _NX_READ(sthis->fd, sthis->buffer, 4096);
      syslog(LOG_INFO, "CACHE: Processed %zd bytes of %ld total\n", avail + this->nget, sthis->flen);
      if (avail < 1)
        {
          ret = EOF;
        }
      else
        {
          sthis->buflen = avail;
          sthis->bufidx = 1;
          ret = sthis->buffer[0];
        }
    }

  if (ret != EOF)
    {
      this->nget++;
    }

  return ret;
}

static void cache_stdinstream(FAR struct cache_stdinstream_s *instream,
                              int fd)
{
  struct stat st;

  instream->public.get = cache_stdinstream_getc;
  instream->public.nget = 0;
  instream->buflen = 0;
  instream->bufidx = 0;
  instream->fd = fd;

  if (fstat(fd, &st) == 0)
    {
      instream->flen = st.st_size;
    }
  else
    {
      instream->flen = 0;
      syslog(LOG_ERR, "CACHE: Could not stat file: %d\n", errno);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sd_main
 *
 * Description:
 *   sd_main is a tiny program that runs in FLASH.  sd_main will
 *   configure SRAM and load an Intel HEX file into SRAM,
 *   and either start that program or wait for you to break in with the
 *   debugger.
 *
 ****************************************************************************/

int sd_main(int argc, char *argv)
{
  struct lib_memsostream_s memoutstream;
  static struct cache_stdinstream_s stdinstream;
  int fd;
  int ret;

  /* SRAM was already initialized at boot time, so we are ready to load the
   * Intel HEX stream into SRAM.
   */

#ifndef CONFIG_BOARD_LATE_INITIALIZE
  /* Perform board-level initialization.  This should include registering
   * the MMC/SD block driver at /dev/mmcsd0.
   */

  DEBUGVERIFY(ez80_bringup());
#endif

  syslog(LOG_INFO, "Loading %s\n", MMCSD_HEXFILE);

  /* Mount the MMC/SD block drivers at /mnt/sdcard */

  ret = mount(MMCSD_BLOCKDEV, MMCSD_MOUNTPT, "vfat", 0, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      syslog(LOG_ERR, "ERROR: Failed to mount filesystem at %s: %d\n",
             MMCSD_MOUNTPT, errcode);
      goto halt;
    }

  /* Open the file /mnt/sdcard/nuttx.hex */

  fd = open(MMCSD_HEXFILE, O_RDONLY);
  if (fd < 0)
    {
      int errcode = errno;
      syslog(LOG_ERR, "ERROR: Failed to mount filesystem at %s: %d\n",
             MMCSD_MOUNTPT, errcode);
      goto halt_with_mount;
    }

  /* Load the HEX image into memory */

  cache_stdinstream(&stdinstream, fd);
  lib_memsostream(&memoutstream,
                  (FAR char *)SRAM_START, (int)(SRAM_END - SRAM_START));

  ret = hex2bin((FAR struct lib_instream_s *)&stdinstream,
                (FAR struct lib_sostream_s *)&memoutstream,
                (uint32_t)SRAM_START, (uint32_t)SRAM_END, HEX2BIN_NOSWAP);
  if (ret < 0)
    {
      /* We failed to load the HEX image */

      syslog(LOG_ERR, "ERROR: Intel HEX file load failed: %d\n", ret);
      goto halt_with_hexfile;
    }

  close(fd);
  umount(MMCSD_MOUNTPT);

  /* The reset vector should now be present at the beginning of SRAM.
   * This assumes that the image was loaded at 0x040000 and that the reset
   * vector is the first thing in memory.
   */

  syslog(LOG_INFO, "Starting at %p\n", SRAM_ENTRY);

  /* Interrupts must be disabled through the following. */

  up_irq_save();

  /* Then jump into SRAM via the reset vector at 0x040000 */

  SRAM_ENTRY();
  goto halt;

halt_with_hexfile:
  close(fd);
halt_with_mount:
  umount(MMCSD_MOUNTPT);
halt:
  for (; ; );
  return 0; /* Will not get here */
}
