# The EZ-PZ

The EZ-PZ is a single-board computer based on the eZ80F91 microcontroller. Using the eZ80 strips away a large number of components needed for a Z80 or Z180 SBC:

  - Address decoding is on-die, with four chip select signals defined.
  - Booting is significantly easier, using internal Flash ROM.
  - Programming the ROM involves the well documented ZDI two-wire debug interface
  - Single-step, breakpoints, and processor state introspection are also possible over ZDI
  - There's an RTC, UARTs, SPI, I2C, GPIO, and even an Ethernet controller

The cost for all of this is that it comes in a TQFP-144 package, at 0.5mm pitch. Soldering it is a little fiddly.

The name is the obvious pun, forgoing Australian pronunciation of 'Z' to get a catchy project title.

## The sub-projects

  - [ezpz-main](ezpz-main) describes the main board.
  - [stm32-usb](stm32-usb) contains the USB interface IC project.
  - [zdi-lisp](zdi-lisp) contains µLisp code for controlling the eZ80.

## Programming the eZ80

The on-die Flash has specific timing requirements for writing all bytes of a row of memory. It's not realistic to expect to meet those requirements over ZDI - so writing to Flash remotely involves uploading into SRAM then running code on the eZ80 itself to write into Flash. [zdi-lisp/cpyflsh.S](Code to do this) is in the µLisp implementation.

## Status of the toolchain

The toolchain for building is a stock binutils-2.36 compiled for the Z80 family with the eZ80 as the default target. This provides an assembler and a suite of tools for working with ELF files, including of course the linker. The C compiler is a fork of [https://github.com/jacobly0/llvm-project](jacobly0's llvm for eZ80) which is part of a [TI CE-84+ toolchain](https://ce-programming.github.io/toolchain/). The standard library I'm using at present is a mix of the Zilog ZDS-II routines, my own implementation of 64-bit integer ops, a few routines from the CEdev source, and a C library from the NuttX project.

## Task list

  - [x] Build the motherboard
  - [x] Program the STM32 for UART and basic ZDI
  - [x] Program the STM32 for loading into RAM via ZDI
  - [ ] Toolchain to build code for the eZ80
      - [x] Assembler: `binutils` provides `as` for the eZ80
      - [x] Linker: `binutils` provides `ld` and other tools
      - [x] C compiler: modified ez80-clang from CE Development Tools to target `as` instead of `fasmg`
      - [x] RTL to support ez80-clang. Partially from ZDSII, but I64 ~~and F64~~ routines need to be supplied.
      - [ ] Tidy up into a "toolchain" package of some kind
  - [x] Test the SD card
  - [ ] Build NuttX
      - [x] Basic build: run from Flash, UARTs, `nsh` shell
      - [ ] Run basic build
      - [ ] Build RTC support
      - [ ] Build binfmt support
      - [ ] Build SPI support
      - [ ] Build EMAC support
      - [ ] Build I2C suport
      - [ ] Calibrate `CONFIG_BOARD_LOOPSPERMSEC`
      - [ ] Make `size_t` 24-bit, perhaps
  - [ ] Non-volatile storage
      - [ ] NuttX mount SD card
      - [ ] NuttX format and mount CP/M partition/file/something
  - [ ] CP/M execution
      - [ ] NuttX task to launch a CP/M instance

## Decision log

**December 15, 2020**

Let's do this stupid thing. My trs-20 project was fun, and is somewhat stuck on whether I should redesign the CPU board to accommoate what I've learned, or start in on expansion boards for storage, I/O, graphics, audio, etc. The eZ80 solves a handful of problems - Address & Data Long (ADL) mode seems to be a nicer overall solution to expanding the address range than the Z180's MMU. The `MBASE` register shifting the Z80 mode's 64k around in a 24-bit address space neatly solves the boot address problem. Having on-die SPI, I2C, and GPIO eliminates all of the uses of my cpu board FPGA. The ZDI protocol would've been a huge help when getting the intial boot states working, and when I broke my boot ROM with a bad Flash write.

There's not too many parts needed for a basic working board. The cost will likely be somewhere around $200 all in, for one or two PCB revisions ($50 ea), a few CPUs ($15 a pop), a few memory ICs ($5 for 512Kb, $20 for 1Mb), a USB serial+power (FT232H, $5) IC, some oscillators (50MHz, 16384Hz), power (500mA power budget), SD socket, and a few LEDs.

**December 17, 2020**

The FTDI ICs chew up _vast_ amounts of power. The CP210x devices use significantly less power, but their GPIO pins are fixed as input, PP, or OD. Trying to use any of the dedicated USB ICs to serve two UARTs and the debug wires just raises complications. Instead, I'll use an STM32L073RZ. This µC has a full-speed USB (12Mhz) driver, on-board clocks including automatic adjustment for the USB interface, and is very low power. I need to program the dang thing to do what I want though. I've ordered a Nucleo-64 board with this part on it to experiment on.

**December 21, 2020**

Soldering dozens of parts kind of sucks. I want to try out PCB assembly. This seems like a good time to give that a go. This restricts me to using parts available from LCSC, or supplying and soldering them myself. Most of what I need is available: the CPU itself is not, nor are USB sockets, SD card sockets, or any through-hole parts like headers.

This decision also pushes me to using EasyEDA. The theory is that EasyEDA implements JLC's design rules, and integrates well with its fabrication and part library. The main board sub-project is therefore now defunct, and will be removed.

**December 31, 2020**

What the hell, let's put the Ethernet interface back on. The power budget is looking okay: 190mA for the CPU, 50mA for the memory, 10mA for the STM32, 100mA for the SD card - we're at 350mA, plus LEDs, passives, clocks, and the like. This leaves me a decent 100mA left to chew up with an Ethernet PHY that needs 90mA. I don't know what the heck I'm doing with Ethernet though, so (1) there's a moderate chance the port simply won't work, and (2) I'll have to factor this into the software design somehow. A future revision, perhaps, should use PoE to power the whole board. This revision definitely does not: it's 48VAC, and also I don't have a PoE switch.
