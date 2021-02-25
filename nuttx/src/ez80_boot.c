#include <nuttx/config.h>

#include <stdbool.h>

#include <nuttx/arch.h>
#include <arch/chip/io.h>

#include "chip.h"
#include "z80_internal.h"
#include "ezpz.h"

void ez80_board_initialize()
{
#ifdef CONFIG_EZ80_SPI
    ez80_spidev_initialize();
#endif
}

// is void board_late_initialize(void) needed?
