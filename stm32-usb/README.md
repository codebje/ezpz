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

### USB call tree

  - `stm32l0xx_it.c:USB_IRQHandler()` is invoked by hardware
      - `stm32l0xx_hal_pcd.c:HAL_PCD_IRQHandler()` checks `USB_ISTR` status register
          - If `USB_ISTR_CTR` is set, indicating an endpoint has completed a transfer:
              - `stm32l0xx_hal_pcd.c:PCD_EP_ISR_Handler()` - endpoint handler
                  - `usbd_conf.c:HAL_PCD_SetupStageCallback()` on SETUP
                      - `usbd_core.c:USBD_LL_SetupStage()`
                          - `usbd_ctlreq.c:USBD_StdDevReq()` on recipient DEVICE
                              - `usbd_cdc.c:USBD_CDC->Setup()` on type CLASS or VENDOR
                          - `usbd_ctlreq.c:USBD_StdItfReq()` on recipient INTERFACE
                              - `usbd_cdc.c:USBD_CDC->Setup()` on type CLASS, VENDOR, or STANDARD
                          - `usbd_ctlreq.c:USBD_StdEPReq()` on recipient ENDPOINT
                  - `usbd_conf.c:HAL_PCD_DataOutStageCallback()` on OUT
                      - `usbd_core.c:USBD_LL_DataOutStage()`
                  - `usbd_conf.c:HAL_PCD_DataInStageCallback()' on IN
                      - `usbd_core.c:USBD_LL_DataInStage()`
          - If `USB_ISTR_RESET` is set:
              - `usbd_conf.c:HAL_PCD_ResetCallback()`
                  - `usbd_core.c:USBD_LL_SetSpeed(USBD_SPEED_FULL)`
                  - `usbd_core.c:USBD_LL_Reset()`
                      - `usbd_conf.c:USBD_LL_OpenEP()`
                          - `stm32l0xx_hal_pcd.c:HAL_PCD_EP_Open()` for EP0 OUT
                          - `stm32l0xx_hal_pcd.c:HAL_PCD_EP_Open()` for EP0 IN
                          - Calls `DeInit` from `usbd_cdc.c:USBD_CDC`, which is `usbd_cdc.c:USBD_CDC_DeInit()`
                              - `stm32l0xx_hal_pcd.c:USBD_LL_CloseEP()` for `CDC_IN_EP`
                              - `stm32l0xx_hal_pcd.c:USBD_LL_CloseEP()` for `CDC_OUT_EP`
                              - `stm32l0xx_hal_pcd.c:USBD_LL_CloseEP()` for `CDC_CMD_EP`
                              - Calls `DeInit` from `usbd_cdc_if.c:USBD_Interface_fops_FS`
                              - Frees the class data
              - `stm32l0xx_hal_pcd.c:HAL_PCD_SetAddress()` to 0
          - If `USB_ISTR_WKUP` is set:
              - `stm32l0xx_hal_pcd_ex.c:HAL_PCDEx_LPM_Callback()` if `LPM_State` is `LPM_L1`
                  - `__weak` stub, does nothing
              - `usbd_conf.c:HAL_PCD_ResumeCallback()`
                  - If low power mode is enabled, clears sleep on exit bit
                  - `usbd_core.c:USBD_LL_Resume()` sets state to what it was before suspend
          - If `USB_ISTR_SUSP` is set:
              - `usbd_conf.c:HAL_PCD_SuspendCallback()`
                  - `usbd_core.c:USBD_LL_Suspend()` sets state to suspended
                  - If low power mode is enabled, puts µC to sleep on exit from ISR
          - If `USB_ISTR_L1REQ` is set:
              - `stm32l0xx_hal_pcd_ex.c:HAL_PCDEx_LPM_Callback()` if `LPM_State` is `LPM_L0`
              - `usbd_conf.c:HAL_PCD_SuspendCallback()` otherwise
          - If `USB_ISTR_SOF` is set:
              - `usbd_conf.c:HAL_PCD_SOFCallback()`
                  - `usbd_core.c:USBD_LL_SOF()`
                      - Looks up class callback in `usbd_cdc.c:USBD_CDC`, which is `NULL`

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
  - [ ] Implement ZDI
