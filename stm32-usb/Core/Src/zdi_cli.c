/*
 * zdi_cli.c
 *
 *  Created on: Jan 16, 2021
 *      Author: bje
 */

#include <string.h>
#include "stm32l0xx_hal.h"
#include "zdi_cli.h"

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

/* the line buffer */
static uint8_t big_ol_buffer[1024];
static uint16_t used_len = 0;

volatile uint8_t cli_line_ready = 0;
volatile uint8_t *cli_line_start = big_ol_buffer;

/* cli_line_add_bytes() adds data to the CLI line buffer. */
void cli_line_add_bytes(uint8_t *data, uint16_t len)
{
	uint16_t copy = MIN(1024 - used_len, len);

	memcpy(big_ol_buffer + used_len, data, copy);
	used_len += copy;

	// buffer overflow
	if (copy == 0) {

	}
}

/* mark the line as processed: will set cli_line_ready appropriately */
void cli_line_processed()
{

}
