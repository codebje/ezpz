# USB driver for eZ-pZ

## Design notes

Virtual COM port activity is entirely interrupt driven: USB data is sent to UARTs, UART data is sent to USB. Nothing happens unless an interrupt makes it happen.

ZDI activity is more stateful and will constitute the main execution loop of the firmware: wait for user input (constructed via USB ISRs), process one command at a time.

Each CDC function has one CIC interface with one Interrupt IN endpoint, and one CDC interface with paired Bulk IN and OUT endpoints. The STM32L073 has eight IN endpoints and eight OUT endpoints; one IN endpoint is reserved for the control endpoint.

| Endpoint      | IN            | OUT           |
| ------------- | ------------- | ------------- |
| EP0           | Control       | Control       |
| EP1           | UART0 CDC     | UART0 CDC     |
| EP2           | UART0 CIC     |               |
| EP3           | UART1 CDC     | UART1 CDC     |
| EP4           | UART1 CIC     |               |
| EP5           | UART2 CDC     | UART2 CDC     |
| EP6           | UART2 CIC     |               |
| EP7           |               |               |

There's a 1024-byte buffer available, to be shared between a 64-byte buffer descriptor table and the buffers for received and transmitted data. All endpoints in a Full-speed device are limited to a maximum of 64 bytes per packet, but the low level driver will take care of multi-packet transmissions on an IN endpoint.

### ZDI protocol

The ZDI protocol is documented in the eZ80 family product specifications. It's a two-wire synchronous serial protocol, with a bidirectional data signal. This signal is push-pull: the maximum speed of the clock is 8MHz, and the line should be pulled up with a minimum 10kΩ resistor. The RC network against the CPU's input pin is too slow for this to be an open drain pin.  Therefore, care is required to prevent burning pins out with contention.

An application note recommends 33Ω series resistors on the line, which regrettably I did not read prior to finalising the hardware design. This would have limited current to 100mA, which still is enough to damage both devices anyway. Due to mixing up `str` and `ldr` I wound up driving the two ICs in competition anyway, but magically the protections in each seemed to save me from myself. Looks like I pulled a Homer.

The protocol idles clock and data lines high. To start a transaction, the data signal is driven low while the clock is held high. Every transaction begins by the master (the STM32) sending a register address and direction bit. Between each byte is a single-bit separator - the master drives this bit at least for the first byte, but it's not clear from documentation who drives it after a read byte. Logic analyser traces suggest the CPU stops driving after the final data bit, with the pull-up resistor bringing it high shortly thereafter.

It's not clear how repeated reads work. When does the CPU stop sending more data? If I do a read, then toggle the clock line again a few seconds later, will more reads happen? Is there a timeout? I assume a new start signal will terminate a read, but if the µC is interrupted after four bits of a read how would it re-synchronise to safely drive the start signal again?

The operations of the break control register are a bit opaque. Reading registers or memory requires the CPU to be in ZDI mode (ie, bit 7 of `ZDI_STAT` is set). I assumed bit 0 of `ZDI_BRK_CTRL`, the `SINGLE_STEP` flag, would cause the CPU to hit its next breakpoint faster than a single ZDI transaction. Writing `$81`{.z80asm} to the register does halt the CPU and allow the other ZDI operations to work, but writing `$01`{.z80asm} doesn't result in the CPU entering ZDI mode again. I might be able to work out more when it's not just excuting `rst $38`{.z80asm} endlessly.

### UART bridging

Bridging between the two UARTs and the USB interfaces largely takes place in `usbd_cdc_if.c`. Each bridge has a circular buffer for each data direction. The buffer from the USB to the UART is a packet buffer: each USB transaction receives up to 64 bytes of data.

DMA is used to receive UART data into a circular buffer. Each SoF event triggers a USB transfer of whatever data is currently available. At 115.2kbps the UART will be delivering approximately 12 bytes per frame - while USB is connected, overruns will not be a concern.

Any error causes the DMA transfer to stop, and invoke `HAL_UART_ErrorCallback()`. The current implementation puts the STM32 into an infinite loop, but some more tolerant approach to try and recover would be better. Simply restarting the STM32 might work well: it will force a disconnect of the USB VCPs, signalling the error condition and recovery to the user.

  - `CDC_Init_FS()` resets state and initiates data reception from both USB and UART
  - `CDC_DeInit_FS()` switches off DMA receive and resets the receive buffer, but allows TX to finish
  - `CDC_Receive_FS()` receives a packet from the USB device, starts UART transmit if idle, and prepares to receive another packet if space permits
  - `CDC_SoF_FS()` checks if any data is pending from the UART, and begins a USB transmit. If the transmit fails because the USB device is busy, the next FS check will try again
  - `HAL_UART_TxCpltCallback()` consumes one packet from the UART buffer. If more data is available, it will begin another transmission
  - `HAL_UART_RxCpltCallback()` and `HAL_UART_RxHalfCpltCallback()` have the opportunity to test for overrun conditions, but currently do not
  - `HAL_UART_ErrorCallback()` halts the processor

### Modules

  - `Core`
      - `main.c` calls out to initialise peripherals, then enters the main loop. Currently this does nothing: eventually it will run the ZDI interactions
      - `zdi.c` reads and writes data through the ZDI two-wire protocol
      - `stm32l0xx_it.c` invokes the USB peripheral controller driver (PCD) via `USB_IRQHandler()`
      - Other files configure peripherals or provide necessary stubs and are unmodified
  - `USB`
      - `usbd_conf.c` has the callbacks invoked by the HAL PCD code, and some bindings between Low-Level and PCD
      - `usbd_core.c` connects a `USBD_ClassTypeDef` to the ISR callbacks
      - `usbd_desc.c` contains the device descriptor, strings, VID, and PID
      - `usbd_ctlreq.c` handles Setup requests that are not Class/Vendor specific
      - `usbd_ioreq.c` sends and receives on the control endpoint
      - `usbd_cdc.c` implements the CDC device class
      - `usb_device.c` initialises the USB device
      - `usbd_cdc_if.c` is the controller for the three interfaces

## Issues

  - `stm32l0xx_hal_pcd.c:HAL_PCD_IRQHandler()` clears `USB_ISTR` flags erroneously by using a read-modify-write sequence.
  - `usbd_ctlreq.c:USBD_StdEPReq()` checks for CLASS requests twice. Only effect is mildly reduced efficiency.

## Changes made to the USB Device code (other than `usbd_cdc_if.c`)

 1. The STM32L073 only supports USB Full Speed. There's code to support some parts of High Speed, but they're unreachable. Removing the high speed and other-speed configuration descriptors and associated callbacks and invocations reduces code complexity.
 2. Change `usbd_desc.c:USBD_FS_DeviceDesc[]` to the Composite class.
 3. Change the configuration descriptor:
      - Change to bus powered
      - Change to 500mA max power
      - Change to Composite Device Class
      - Add an IAD for VCP 1
 4. Remove the USB middleware to prevent compilation conflicts. Header files need to change, and STM32CubeIDE isn't built to allow deep modifications of middleware. Turning it off creates `HAL_PCD_MspInit()` and `HAL_PCD_MspDeInit()` functions in Core that need adustments within user code areas.
 5. Support multiple CDC functions
      - Add `enum USBD_CDC_Function`
      - Modify `CDC_*_EP` macros to take an argument, add endpoint/interface to function translations
      - Modify `struct USBD_CDC_ItfTypeDef` to pass function or control message recipient data
      - Modify `struct USBD_CDC_HandleTypeDef` to have per-function buffers and state
      - Modify `USBD_CDC_Set{Rx,Tx}Buffer` and `USBD_CDC_{Transmit,Receive}Packet` to support separate functions
      - Modify `CDC_Transmit_FS` to support separate functions
      - Add `USB_CLASS_...` defines for symbolic configuration
      - Modify `USBD_CDC_Init` to open all functions' endpoints
      - Modify `USBD_CDC_DeInit` to close all functions' endpoints
      - Modify `USBD_CDC_Setup` to pass through recipient and index data
      - Modify `USBD_LL_Init` to set up the PMA memory map
      - Update `USBD_MAX_NUM_INTERFACES`

## Task sheet

  - [x] Configure peripherals in CubeMX
  - [x] Relocate USB middleware to prevent overwrites
  - [x] Update middleware to support three CDC interfaces
      - [x] Remove superfluous high-speed code
      - [x] Modify device descriptor
      - [x] Modify configurations
      - [x] Change USB device design to support multiple interfaces
  - [x] Support changing line configuration
  - [x] Wire up two CDCs to UARTs
  - [ ] Implement a debug monitor on the third CDC
      - [x] Basic single-character interactions
      - [ ] Command line or menu interface to 
  - [ ] Implement ZDI
      - [x] ZDI write single byte
      - [x] ZDI read single byte
      - [ ] ZDI write multiple
      - [ ] ZDI read multiple
