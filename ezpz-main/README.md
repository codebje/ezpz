# The EZ-PZ main board

The EZ-PZ main board delivers:

  - The CPU
  - 1Mb of SRAM (as 2x512K ICs)
  - Power (via USB, 500mA budget, 3.3v linear regulator)
  - USB (power, 2xUART, ZDI)
  - Ethernet 

## Components

The CPU needs two time sources: a 50MHz system clock (or <=10MHz to use the PLL, which I am not) and a 32.768kHz RTC clock. For the 50MHz clock I'm using a CMOS oscillator, the Epson Q33310F70062200. This is a 50ppm part, using 4.5mA. Its rise time is 4ns, which does exceed the 3ns maximum of the eZ80. If this turns out to be a major issue I guess I get to desolder the oscillator and try to replace it with a better part. For the 32.768kHz clock source I'm using a crystal. Unfortunately, I have to use a honking great big piece for this: I'd like to be thoroughly in spec to give oscillation the best chance of starting, so I've chosen the YXC X803832768KGD4GI. It's enormous, 8mm by 3mm, but it's 50kΩ ESR, 12.5pF CL, 20ppm tolerance, and 1.0µW drive. I've paired it with 18pF capacitors.

The RTC gets a battery backup. A CR1225 provides 3V nominal, housed in a S8411-45R Harwin SMD mount. Using a diode ORing circuit will let supply come from Vcc when that's available, as per the eZ80 errata.

I would have liked an 8Mbit SRAM IC but settled for a 4Mbit instead. It's 10ns, it _should_ run fine with zero wait states, but I don't know how well I've laid out my PCB. These are faster signals than I've really worked with before, 50MHz SPI notwithstanding. The IC I'm using is the Lyontek LY61L5128AML-10I: 10ns access time, 50mA typical current. It might peak as high as 70mA, but that should be relatively rare.

### USB

The USB interface needs to:

 1. Support a bus-powered application
 2. Drive UART0 to provide USB serial console
 3. Drive the ZDI two-wire interface for USB debugging
 4. Optionally, drive UART1 for a secondary serial interface

The approach taken is an STM32L073RZ, which can run a USB 2.0 Full Speed link with enough interfaces to provide three UARTs, one each for UART0 and UART1 and an additional one as a debug interface for ZDI.

### Ethernet

A last-minute addition is an Ethernet PHY IC and RJ45 socket. Driving Ethernet from the eZ80 is straightforward: wire up the MII pins to the PHY IC and figure out the software later. Dealing with the PHY IC isn't quite as straightforward. Ethernet ports need a transformer to isolate the wires from the PHY IC - so I went simple and chose a socket with a built-in transformer. The remaining complexity comes from the need for many capacitors and careful routing - and another 25MHz crystal. I'm using the Texas Instruments DP83848CVVX for Ethernet. This part claims low power consumption at under 270mW (at 3.3V, this is around 80mA) and has plenty of information in its datasheet. Plus some obvious errors: it tells me to wire up PFBIN3 and PFBIN4, signals that don't exist on that IC, mapped to pins that don't exist on an LQFP-64 package. I used the schematic for the evaluation board to help guide me through this process.

The RJ45 port also has two built-in LEDs, green and yellow inverse parallel. The DP83848C has three LED outputs: by default, link status, 10/100Mbit status, and activity. I did a somewhat [experimental arrangement of MOSFETs][LEDs] to try to arrange for one LED to be green for a 100Mbit link, yellow for a 10Mbit link, and unlit for no link. Fingers crossed I won't fry anything with this.

https://au.rs-online.com/web/p/lan-ethernet-transformers/1349359

## Power budget

| Component     | Consumption   |
| ------------- | ------------- |
| CPU           | 190mA         |
| SRAM          | 50mA          |
| STM32L073RZ   | 10mA          |
| Misc. parts   | 50mA          |
| SD card       | 100mA         |
| Ethernet PHY  | 100mA         |
| ------------- | ------------- |
| Total         | 500mA         |

# Assembly notes

U2 rotation is incorrect
U3 rotation is incorrect

[LEDs]: https://www.falstad.com/circuit/circuitjs.html?ctz=CQAgjCAMB0l3BWcMBMcUHYMGZIA4UA2ATmIxAUgpABZsAoMQlEbBF3G27DyLlgCYBTAGYBDAK4AbAC4BaKUIFRwKmJDCNm3DtkKt2rPSEGjJshUtUQq6zQHcj+nlTYc0Ueo7q8uP1h6QXjpOIS6ejm6sfAbutsFR4YnGQQAyFBjOHoT4AbYq4lIAzkKs0Nie6WCZeeA0XOH5hSVlFUEAHuAoxKz14Dm95FxgXKkAkgByANL0nQh4SNh0IIQ8vfrDXADKAAoAonsAIvQAStH8uZz9+VQ0ruVqUNAI9ADm52ExNIRc8ZHGSRijQSQI8Vz0eAiH0wvlqQUcKDuIURrhSwX84RoeFcgXoIlYeAahI+2GJazA0BoSHUKDxBP4CGcMRQjKM4AeGGCCBq4W5zmJ8Iy-L8WPpUL5tQlLP0gqlrPqkOlngATkLalicfkUGgQX41lc7n8xUrSRdIYLTSZ5UwrTL6KqrmhIZanSowPALczLsykYKUSZvWbPPj-BqwmSKhS2rBaQ6vvrif4qO74Akyem9SwLRnaDbLX6kRj41n7SYka7-TC3R66dUslR-UkWBSqU9ILSEeWMM2alXBXWTN3wDURr90eS+gPR8GQtP-EqkS3qTHggPwgOw-2amHQ9ioY2fQ2h0EAPbDla-WiQUjUOwslQ91j0IA

# Rev. A notes

 1. Several silkscreen notes are wrong:
      - SWDIO and SWCLK are swapped
      - Write Protection ON and OFF are swapped
 2. Some things proved to be unnecessary:
      - 3V3 and NRST pins on the SWD (H2) header
      - SS is fine for the SD card, so H5 to choose PB1 isn't needed
      - The SWD header is enough to program the STM32, the boot header isn't needed
      - The ZDI header is unnecessary, as is the exposure of ZDA/ZCL on H4
 3. Some things are missing:
      - Connecting SYS RESET to the STM32 would let it reboot the CPU directly
      - Being able to expand the board and deliver more power isn't feasible
      - A power switch for the SD card might be nice
      - Card detect also might be nice
 4. Some things proved to be flat out wrong:
      - Without MISO pulled high a driver looking for 0xFF will never find it prior to card init
      - MDIO is supposed to be pulled up @1.5kΩ
      - RBIAS (via R1) should be pulled low, not high
      - FBOUT, FBIN1, FBIN2 should all be directly connected, with caps to GND each
 5. Some experiments worked nicely:
      - The SRAM operates at 50MHz just fine
      - The crystals for PHY, RTC work just fine
      - The LED driver array of FETs works just fine
      - The dual reset button thing is very handy
      - Power, UART, and debug over the one USB port is very good

Patches done:

  - Add 1.5kΩ resistor to MDIO
  - Correct PHY power feedback wiring
  - TODO: correct RBIAS voltage

# Rev. B fixes and improvements

  - [x] Silkscreen labels
  - [x] Remove H5 SS selection
  - [x] Pull MISO high
  - [x] Remove boot header
  - [x] Fix connections for PHY feedback pins
  - [x] Add MDIO pull-up
  - [x] Fix R1 to pull low
  - [ ] Replace ZCA/ZDL header with Zilog debug port 6-pin thingo
  - [x] Add RESET line to STM32
  - [x] Expose VBUS on H4
  - [ ] Can I shut off U4 regulator and deliver 3V3 via H4?
