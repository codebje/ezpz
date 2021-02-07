/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @version        : v2.0_Cube
  * @brief          : Usb device for Virtual Com Port.
  ******************************************************************************
  * @attention
  *
  * Portions <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"
#include "zdi.h"

/* Private define ------------------------------------------------------------*/

#define UART_BRIDGE_BUFFERSIZE	256			/* use a power of two */
#define UART_BRIDGE_PACKETCOUNT	16			/* use a power of two */

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
	UART_HandleTypeDef *huart;
	USBD_CDC_Function function;
	uint8_t usb_buffer[UART_BRIDGE_BUFFERSIZE];			/* Pending data to write to USB */
	__IO uint16_t i_usb_read;							/* The next byte to transmit is here */
	struct {
		uint8_t data[64];								/* packet data */
		uint32_t len;									/* packet size */
	} uart_buffer[UART_BRIDGE_PACKETCOUNT];				/* circular buffer of packets */
	__IO uint8_t i_uart_write;							/* next packet to receive into */
	__IO uint8_t i_uart_read;							/* next packet to transmit */
} UART_BridgeTypeDef;

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes ------------------------------------------------*/

static int8_t CDC_Init_FS(USBD_CDC_Function function);
static int8_t CDC_DeInit_FS(USBD_CDC_Function function);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t recipient, uint16_t index, uint8_t *pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t Len, USBD_CDC_Function function);
static int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t Len, USBD_CDC_Function function);
static int8_t CDC_SoF_FS(void);
static void CDC_UART_SetLineCoding(UART_HandleTypeDef *uart, uint32_t bps, uint8_t stop, uint8_t parity, uint8_t data);

/* External variables --------------------------------------------------------*/

extern USBD_HandleTypeDef hUsbDeviceFS;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* Private variables ---------------------------------------------------------*/

UART_BridgeTypeDef bridges[2] = {
		{ .huart = &huart1, .function = CDC1, .i_uart_write = 0, .i_uart_read = 0, .i_usb_read = 0 },
		{ .huart = &huart2, .function = CDC2, .i_uart_write = 0, .i_uart_read = 0, .i_usb_read = 0 },
};

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS,
  CDC_TransmitCplt_FS,
  CDC_SoF_FS,
};

/* Data received from CDC3 or pending to be sent to CDC3 */
static uint8_t cdc3_rx_buffer[UART_BRIDGE_BUFFERSIZE];
static uint8_t cdc3_tx_buffer[UART_BRIDGE_BUFFERSIZE];
__IO static uint16_t cdc3_rx_read, cdc3_rx_write, cdc3_tx_read, cdc3_tx_write;
__IO static uint8_t cdc3_rx_active;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @param  function: CDC function code
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(USBD_CDC_Function function)
{
	/* CDC3 will receive into this space; the data will be copied into cdc3_rx_buffer */
	static uint8_t buffer[64];

	switch (function) {
		case CDC1:
		case CDC2:
			/* Reset any in-progress transmission */
			HAL_UART_AbortTransmit_IT(bridges[function].huart);

			/* Reset state variables */
			bridges[function].i_uart_read = bridges[function].i_uart_write = 0;
			bridges[function].i_usb_read = 0;

			/* Prepare CDC receive buffers */
			USBD_CDC_SetRxBuffer(&hUsbDeviceFS, bridges[function].uart_buffer[0].data, function);
			USBD_CDC_ReceivePacket(&hUsbDeviceFS, function);

			/* Begin UART receive */
			HAL_UART_Receive_DMA(bridges[function].huart, bridges[function].usb_buffer, UART_BRIDGE_BUFFERSIZE);
			break;
		case CDC3:
			cdc3_rx_read = cdc3_rx_write = cdc3_tx_read = cdc3_tx_write = 0;
			cdc3_rx_active = 1;
			USBD_CDC_SetRxBuffer(&hUsbDeviceFS, buffer, function);
			USBD_CDC_ReceivePacket(&hUsbDeviceFS, function);
			break;
		default:
			break;
	}
	return (USBD_OK);
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @param  function: CDC function code
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(USBD_CDC_Function function)
{
	/* Reset the RX, there's nowhere to forward the data now */
	HAL_UART_DMAStop(&huart1);
	HAL_UART_DMAStop(&huart2);
	bridges[0].i_usb_read = bridges[1].i_usb_read = 0;

	/* Allow any TX in flight to finish */

	return (USBD_OK);
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t recipient, uint16_t index, uint8_t *pbuf, uint16_t length)
{
	switch(cmd)
	{
    	case CDC_SEND_ENCAPSULATED_COMMAND:
    		break;

    	case CDC_GET_ENCAPSULATED_RESPONSE:
    		break;

    	case CDC_SET_COMM_FEATURE:
    		break;

    	case CDC_GET_COMM_FEATURE:
    		break;

    	case CDC_CLEAR_COMM_FEATURE:
    		break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
    	case CDC_SET_LINE_CODING:
    		switch (CDC_IF_FN(index)) {
    			case CDC1:
    				CDC_UART_SetLineCoding(&huart1, *((uint32_t *)pbuf), pbuf[4], pbuf[5], pbuf[6]);
    				break;
    			case CDC2:
    				CDC_UART_SetLineCoding(&huart2, *((uint32_t *)pbuf), pbuf[4], pbuf[5], pbuf[6]);
    				break;
    			default:
    				break;
    		}
    		break;

    	case CDC_GET_LINE_CODING:
    		/* TODO allow this */
    		break;

		case CDC_SET_CONTROL_LINE_STATE:
			break;

		case CDC_SEND_BREAK:
			break;

		default:
			break;
	}

	return (USBD_OK);
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will issue a NAK packet on any OUT packet received on
  *         USB endpoint until exiting this function. If you exit this function
  *         before transfer is complete on CDC interface (ie. using DMA controller)
  *         it will result in receiving more data while previous ones are still
  *         not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t Len, USBD_CDC_Function function)
{
	uint16_t size;

	switch (function)
	{
		case CDC1:
		case CDC2:
			// This will fail with HAL_BUSY if a transmit is in progress. If so, the TX ISR
			// will continue working through the backlog of buffered data as it can.
			HAL_UART_Transmit_IT(bridges[function].huart, Buf, Len);

			bridges[function].uart_buffer[bridges[function].i_uart_write].len = Len;

			// Increment the UART write pointer
			bridges[function].i_uart_write = (bridges[function].i_uart_write + 1) % UART_BRIDGE_PACKETCOUNT;

			// If there's a free packet buffer, start a new receive
			if ((bridges[function].i_uart_write+1) % UART_BRIDGE_PACKETCOUNT != bridges[function].i_uart_read) {
				USBD_CDC_SetRxBuffer(&hUsbDeviceFS,
						bridges[function].uart_buffer[bridges[function].i_uart_write].data,
						function);
				USBD_CDC_ReceivePacket(&hUsbDeviceFS, function);
			}
			break;
		case CDC3:
			// copy data into the cdc3_rx_buffer, which might need to wrap
			size = MIN(UART_BRIDGE_BUFFERSIZE - cdc3_rx_write, Len);			// how much fits at the end of the buffer?
			memcpy(cdc3_rx_buffer + cdc3_rx_write, Buf, size);					// copy from the start of Buf
			cdc3_rx_write = (cdc3_rx_write + size) % UART_BRIDGE_BUFFERSIZE;	// update write pointer
			memcpy(cdc3_rx_buffer + cdc3_rx_write, Buf + size, Len - size);		// copy anything left over to the start of the buffer
			cdc3_rx_write = cdc3_rx_write + (Len - size);						// update again

			if ((cdc3_rx_read + UART_BRIDGE_BUFFERSIZE - 1 - cdc3_rx_write) % UART_BRIDGE_BUFFERSIZE > 64) {
				// begin a new receive if there's space to store it
				USBD_CDC_ReceivePacket(&hUsbDeviceFS, function);
			} else {
				// otherwise, flag that RX has been suspended
				cdc3_rx_active = 0;
			}
			HAL_GPIO_TogglePin(TX_LED_GPIO_Port, TX_LED_Pin);
			break;
		default:
			break;
	}

	return (USBD_OK);
}

/**
 * @brief	CDC_TransmitCplt_FS
 * 			Called when a transmission has completed.
 *
 * @param   Buf: Buffer of data that was sent
 * @param   Len: Number of data sent (in bytes)
 * @param	function: the CDC function of the transfer
 * @retval	USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t Len, USBD_CDC_Function function)
{
	if (function == CDC3) {
		// update read pointer based on sent data
		cdc3_tx_read = (cdc3_tx_read + Len) % UART_BRIDGE_BUFFERSIZE;

		/* Send any more pending data */
		if (cdc3_tx_read > cdc3_tx_write) {
			// transmit up to end of buffer
			CDC_Transmit_FS(cdc3_tx_buffer + cdc3_tx_read, UART_BRIDGE_BUFFERSIZE - cdc3_tx_read, CDC3);
		} else if (cdc3_tx_read < cdc3_tx_write) {
			CDC_Transmit_FS(cdc3_tx_buffer + cdc3_tx_read, cdc3_tx_write - cdc3_tx_read, CDC3);
		}
	}

	return USBD_OK;
}

/**
  * @brief  CDC_Transmit_FS
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len, USBD_CDC_Function function)
{
	uint8_t result = USBD_OK;
	USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
	if (hcdc->function[function].TxState != 0) {
		return USBD_BUSY;
	}
	USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len, function);
	result = USBD_CDC_TransmitPacket(&hUsbDeviceFS, function);
	return result;
}

static int8_t CDC_SoF_FS(void)
{
	uint8_t did_change = 0;

	for (USBD_CDC_Function i = CDC1; i < CDC3; i++) {
		uint16_t i_usb_write = UART_BRIDGE_BUFFERSIZE - __HAL_DMA_GET_COUNTER(bridges[i].huart->hdmarx);
		/* we might catch the DMA with its counter sitting at zero */
		if (i_usb_write == 1024) i_usb_write = 0;
		if (bridges[i].i_usb_read > i_usb_write) {
			did_change = 1;
			/* read pointer is ahead of write pointer: transmit up to end of buffer */
			if (CDC_Transmit_FS(bridges[i].usb_buffer + bridges[i].i_usb_read,
					UART_BRIDGE_BUFFERSIZE - bridges[i].i_usb_read, i) == USBD_OK) {
				bridges[i].i_usb_read = 0;
			}
		} else if (bridges[i].i_usb_read < i_usb_write) {
			did_change = 1;
			/* read pointer is behind write pointer: transmit the data between them */
			if (CDC_Transmit_FS(bridges[i].usb_buffer + bridges[i].i_usb_read,
					i_usb_write - bridges[i].i_usb_read, i) == USBD_OK) {
				bridges[i].i_usb_read = i_usb_write;
			}
		}
	}

	if (did_change) {
		// XXX RX pin change
//		HAL_GPIO_TogglePin(RX_LED_GPIO_Port, RX_LED_Pin);
	}

	/* Let the transmit complete callback attempt to send more data */
	CDC_TransmitCplt_FS(NULL, 0, CDC3);

	return (USBD_OK);
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

static void CDC_UART_SetLineCoding(UART_HandleTypeDef *uart, uint32_t bps, uint8_t stop, uint8_t parity, uint8_t data)
{
	uint32_t cr1 = uart->Instance->CR1 & ~USART_CR1_UE;

	// disable the interface by resetting UE
	uart->Instance->CR1 = cr1;

	// clear out data bits and parity settings
	cr1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk);

	// mark and space are not supported, it's odd/even/none only
	switch (parity) {
		case 1:
			cr1 |= UART_PARITY_ODD;
			break;
		case 2:
			cr1 |= UART_PARITY_EVEN;
			break;
		default:
			break;
	}

	// 7 & 8 supported: anything else falls back to 8 data bits
	switch (data) {
		case 7:
			cr1 |= UART_WORDLENGTH_7B;
			break;
		default:
			cr1 |= UART_WORDLENGTH_8B;
			break;
	}

	// write new settings
	uart->Instance->CR1 = cr1;

	// all legal options supported, illegal values fall back to one stop bit
	switch (stop) {
		case 1:
			MODIFY_REG(uart->Instance->CR2, USART_CR2_STOP_Msk, UART_STOPBITS_1_5);
			break;
		case 2:
			MODIFY_REG(uart->Instance->CR2, USART_CR2_STOP_Msk, UART_STOPBITS_2);
			break;
		default:
			MODIFY_REG(uart->Instance->CR2, USART_CR2_STOP_Msk, UART_STOPBITS_1);
			break;
	}

	// Determine the clock source
	UART_ClockSourceTypeDef clocksource;
	uint32_t pclk;
	UART_GETCLOCKSOURCE(uart, clocksource);
	switch (clocksource) {
		case UART_CLOCKSOURCE_PCLK1:
			pclk = HAL_RCC_GetPCLK1Freq();
			break;
	    case UART_CLOCKSOURCE_PCLK2:
	        pclk = HAL_RCC_GetPCLK2Freq();
	        break;
	    case UART_CLOCKSOURCE_HSI:
	        if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIV) != 0U) {
	            pclk = (uint32_t)(HSI_VALUE >> 2U);
	        } else {
	            pclk = (uint32_t) HSI_VALUE;
	        }
	        break;
	    case UART_CLOCKSOURCE_SYSCLK:
	        pclk = HAL_RCC_GetSysClockFreq();
	        break;
	    case UART_CLOCKSOURCE_LSE:
	        pclk = (uint32_t) LSE_VALUE;
	        break;
	    default:
	        pclk = 0U;
	        break;
	}

	// Set the BRR. This does not support the LPUART.
	if (pclk != 0U) {
		uint16_t usartdiv = cr1 & USART_CR1_OVER8 ? UART_DIV_SAMPLING8(pclk, bps) : UART_DIV_SAMPLING16(pclk, bps);
	    if (usartdiv >= 16) {
	    	if (cr1 & USART_CR1_OVER8) {
	    		usartdiv = (usartdiv & 0xfff0) | ((usartdiv & 0xf) >> 1);
	    	}
	        uart->Instance->BRR = usartdiv;
	   }
	}

	// Re-enable the UART
	uart->Instance->CR1 = cr1 | USART_CR1_UE;

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	UART_BridgeTypeDef *bridge = huart == &huart1 ? &bridges[0] : &bridges[1];

	/* Recognise the data read has completed */
	bridge->i_uart_read = (bridge->i_uart_read + 1) % UART_BRIDGE_PACKETCOUNT;

	/* The transmit buffer is empty when the two pointers are equal */
	if (bridge->i_uart_read != bridge->i_uart_write) {
		/* Send the next USB packet */
		HAL_UART_Transmit_IT(huart,
				bridge->uart_buffer[bridge->i_uart_read].data,
				bridge->uart_buffer[bridge->i_uart_read].len);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// XXX RX pin change
//	HAL_GPIO_WritePin(RX_LED_GPIO_Port, RX_LED_Pin, GPIO_PIN_RESET);

	// TODO check for overrun
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
//	HAL_GPIO_WritePin(RX_LED_GPIO_Port, RX_LED_Pin, GPIO_PIN_RESET);
}


/* There's not much to be done to recover from an error.
 * One option would be to reset here with NVIC_SystemReset(), but whatever's causing the fault
 * will have persisted and will continue to cause faults. Entering the fault handler might allow
 * a debugger to be connected and backtrace the cause. */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	// XXX RX pin change
	HAL_GPIO_WritePin(RX_LED_GPIO_Port, RX_LED_Pin, GPIO_PIN_RESET);
}

/**
 * CDC3 is mapped to IO putchar/getchar
 */
int __io_putchar(int ch) {
	if ((cdc3_tx_write + 1) % UART_BRIDGE_BUFFERSIZE != cdc3_tx_read) {
		cdc3_tx_buffer[cdc3_tx_write] = (uint8_t)ch;
		cdc3_tx_write = (cdc3_tx_write + 1) % UART_BRIDGE_BUFFERSIZE;
	}
	return 0;
}

int __io_getchar(void) {
	while (cdc3_rx_read == cdc3_rx_write) {
	}
	uint8_t in = cdc3_rx_buffer[cdc3_rx_read];
	cdc3_rx_read = (cdc3_rx_read + 1) % UART_BRIDGE_BUFFERSIZE;

	if (!cdc3_rx_active && (cdc3_rx_read + UART_BRIDGE_BUFFERSIZE - 1 - cdc3_rx_write) % UART_BRIDGE_BUFFERSIZE > 64) {
		// begin a new receive if there's space to store it
		cdc3_rx_active = 1;
		USBD_CDC_ReceivePacket(&hUsbDeviceFS, CDC3);
	}

	return in;
}

