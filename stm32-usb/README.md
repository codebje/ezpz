# USB driver for eZ-pZ

## Task sheet

  - [x] Configure peripherals in CubeMX
  - [x] Relocate USB middleware to prevent overwrites
  - [ ] Update middleware to support three CDC interfaces
      - [ ] Remove superfluous high-speed code
      - [ ] Modify device descriptor
      - [ ] Change callback mechanisms to support multiple interfaces
  - [ ] Support changing line configuration
  - [ ] Wire up two CDCs to UARTs
  - [ ] Implement ZDI
  - [ ] Implement a debug monitor on the third CDC
