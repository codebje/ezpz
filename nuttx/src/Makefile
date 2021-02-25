############################################################################

include $(TOPDIR)/Make.defs

CSRCS = ez80_boot.c ez80_bringup.c

ifeq ($(CONFIG_LIB_BOARDCTL),y)
CSRCS += ez80_appinit.c
endif

ifeq ($(CONFIG_EZ80_SPI),y)
CSRCS += ez80_spi.c
ifeq ($(CONFIG_MMCSD_SPI),y)
CSRCS += ez80_spimmcsd.c
endif
endif

ifeq ($(CONFIG_EZPZ_SDBOOT),y)
CSRCS += sd_main.c
endif

include $(TOPDIR)/boards/Board.mk
