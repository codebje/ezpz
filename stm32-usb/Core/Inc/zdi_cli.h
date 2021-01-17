/*
 * zdi_cli.h
 *
 *  Created on: Jan 16, 2021
 *      Author: bje
 */

#ifndef INC_ZDI_CLI_H_
#define INC_ZDI_CLI_H_

#include "stm32l0xx_hal.h"

/* cli_line_ready is set to 1 when a line is ready to be proessed, and set back to 0 when the line
 * has been processed. */
extern volatile uint8_t cli_line_ready;

/* cli_line_start is the start of the line to be processed */
extern volatile uint8_t *cli_line_start;

/* cli_line_add_bytes() adds data to the CLI line buffer. */
void cli_line_add_bytes(uint8_t *data, uint16_t len);

/* mark the line as processed: will set cli_line_ready appropriately */
void cli_line_processed();

#endif /* INC_ZDI_CLI_H_ */
