README.txt
==========

The EZ-PZ is a retrobrew eZ80 board.

The board's schematics and PCB design may be found at:

    https://oshwlab.com/byron.ellacott/ez-pz

The CPU is a Zilog eZ80 running at 50 MHz, with 512 Kb of zero-wait state
RAM. An STM32 provides two USB UARTs and a Zilog ZDI controller. An SD card
slot is on board, but there are no user controllable LEDs or buttons on the
main board.

Contents
========

  o Supported compiler
  o Configurations
    - Common Configuration Notes
    - Configuration Subdirectories

Supported compiler
========================

The EZ-PZ supports ez80-clang.

Configurations
==============

Common Configuration Notes
--------------------------

  1. src/ and include/

     These directories contain common logic for all EZ-PZ configurations.

  2. Variations on the basic EZ-PZ configuration are maintained
     in subdirectories.  To configure any specific configuration, do the
     following steps:

       tools/configure.sh [OPTIONS] ezpz:<sub-directory>
       make

     Where <sub-directory> is the specific board configuration that you
     wish to build.  Use 'tools/configure.sh -h' to see the possible
     options.  Typical options are:

       -l Configure for a Linux host
       -m Configure for a Mac OS X hot

     Use configure.bat instead of configure.sh if you are building in a
     native Windows environment.

     The available board-specific configurations are  summarized in the
     following paragraphs.

     When the build completes successfully, you will find this files in
     the top level nuttx directory:

     a. nuttx.hex - A loadable file in Intel HEX format
     b. nuttx.bin - A loadable file in raw binary format
     c. nuttx.map - A linker map file

  3. This configuration uses the mconf-based configuration tool.  To
     change this configurations using that tool, you should:

     a. Build and install the kconfig-mconf tool.  See nuttx/README.txt
        see additional README.txt files in the NuttX tools repository.

     b. Execute 'make menuconfig' in nuttx/ in order to start the
        reconfiguration process.

Configuration 'nsh_basic'
-------------------------

This configuration builds the NuttShell (NSH) with minimal enabled
peripherals.

For more information on NuttShell see:
    apps/system/nsh/README.txt
    Documentation/NuttShell.html

This configuration does not support running NuttX from SRAM.

Enabled peripherals:

    o UART0 (console) initialized to 115200 8N1
    o UART1 initialized to 115200 8N1
    o SPI in Master mode
    o MMC/SD support with Slave Select on PB2
    o The real-time clock

Peripherals
===========

Serial ports
------------

The serial ports on the ezpz are connected to the stm32l0 USB bridge, and
appear on a host as eZ80COM1 (UART0) and eZ80COM3 (UART1). Additionally the
bridge provides eZ80COM5 with a ZDI console.

The two ports are both initialized to 115200 8N1. Neither one is connected
to expansion headers or available other than via the USB bridge.


Real-time Clock
---------------

The RTC is battery backed. Observations suggest the clock has significant
drift, and where possible should be updated on boot using a more reliable
time source.

The RTC can be read and set from the NSH date command.

    nsh> date
    Thu, Dec 19 20:53:29 2086
    nsh> help date
    date usage:  date [-s "MMM DD HH:MM:SS YYYY"]
    nsh> date -s "Jun 16 15:09:00 2019"
    nsh> date
    Sun, Jun 16 15:09:01 2019

SPI Master
----------

The SPI Master peripheral uses the MOSI (PB7), MOSI (PB6), and SCK (PB3)
pins, which are connected to the SD card reader as well as being available
on H3 for other SPI peripherals.

MMC/SD card
-----------

The SD card reader's chip select line is connected to either PB1 or BP2
according to the jumper on H5. It is recommended to leave the jumper set
to PB2.

When the system boots, it will probe the SD card and create a block driver
called mmcsd0:

    nsh> ls /dev
    /dev:
     console
     mmcsd0
     null
     ttyS0
    nsh> mount
      /proc type procfs

The SD card can be mounted with the following NSH mount command:

    nsh> mount -t vfat /dev/mmcsd0 /mnt/sdcard
    nsh> ls /mnt
    /mnt:
     sdcard/
    nsh> mount
      /mnt/sdcard type vfat
      /proc type procfs
    nsh> ls -lR /mnt/sdcard
    /mnt/sdcard:
     drw-rw-rw-       0 System Volume Information/
    /mnt/sdcard/System Volume Information:
     -rw-rw-rw-      76 IndexerVolumeGuid
     -rw-rw-rw-      12 WPSettings.dat

You can use the SD card as any other file system.

    nsh> ls /mnt/sdcard
    /mnt/sdcard:
     System Volume Information/
    nsh> echo "This is a test" >/mnt/sdcard/atest.txt
    nsh> ls /mnt/sdcard
    /mnt/sdcard:
     System Volume Information/
     atest.txt
    nsh> cat /mnt/sdcard/atest.txt
    This is a test

Don't forget to un-mount the volume before power cycling:

    nsh> mount
      /mnt/sdcard type vfat
      /proc type procfs
    nsh> umount /mnt/sdcard
    nsh> mount
      /proc type procfs

NOTE:  The is no card detect signal so the microSD card should be placed
in the card slot before the system is started.

