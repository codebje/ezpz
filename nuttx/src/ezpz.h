#ifndef __BOARD_EZPZ_H
#define __BOARD_EZPZ_H

#include <nuttx/config.h>

#define HAVE_MMCSD 1
#if !defined(CONFIG_MMCSD_SPI) || !defined(CONFIG_EZ80_SPI)
#  undef HAVE_MMCSD
#endif

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

int ez80_bringup(void);

#ifdef HAVE_MMCSD
int ez80_mmcsd_initialize(void);
#endif

#ifdef CONFIG_EZ80_SPI
void ez80_spidev_initialize(void);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif // __BOARD_EZPZ_H
