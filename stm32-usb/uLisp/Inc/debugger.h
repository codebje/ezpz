/*
 * debugger.h
 *
 *  Created on: 8 Feb 2021
 *      Author: bje
 */

#ifndef INC_DEBUGGER_H_
#define INC_DEBUGGER_H_

#include <stdint.h>

/* I/O ports */
uint8_t zdi_in(uint16_t port);
void zdi_out(uint16_t port, uint8_t value);

/* State examination */
void zdi_status();
void zdi_registers();
void zdi_stacktrace(int depth);
void zdi_memdump(uint32_t addr);

/* Data control */
uint32_t zdi_rw_read(uint8_t command);
void zdi_rw_write(uint8_t command, uint32_t value);

/* Code execution */
void zdi_exec(uint8_t *opcodes, uint8_t count);
void zdi_set_break(uint8_t bpnum, uint32_t address);
uint32_t zdi_get_break(uint8_t bpnum);
void zdi_clear_break(uint8_t bpnum);
void zdi_stop();
void zdi_run();
void zdi_stepinto();
void zdi_stepover();
void zdi_stepuntil(uint32_t addr);

#endif /* INC_DEBUGGER_H_ */
