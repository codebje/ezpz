/*
 * io_uart.c
 *
 *  Created on: Jan 3, 2021
 *      Author: bje
 */

#include "stm32l0xx_hal.h"

extern UART_HandleTypeDef huart2;

int __io_putchar(int ch) {
	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return 0;
}

int __io_getchar(void) {
	uint8_t in;
	HAL_UART_Receive(&huart2, &in, 1, HAL_MAX_DELAY);
	return in;
}
