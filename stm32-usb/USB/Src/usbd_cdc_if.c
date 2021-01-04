/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @version        : v2.0_Cube
  * @brief          : Usb device for Virtual Com Port.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_CDC_IF
  * @{
  */

/** @defgroup USBD_CDC_IF_Private_TypesDefinitions USBD_CDC_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Defines USBD_CDC_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Macros USBD_CDC_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

extern USBD_HandleTypeDef hUsbDeviceFS;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/** @defgroup USBD_CDC_IF_Private_Variables USBD_CDC_IF_Private_Variables
  * @brief Private variables.
  * @{
  */
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/** Received data over USB are stored in this buffer      */
uint8_t UserRxBufferFS[3][APP_RX_DATA_SIZE];

/** Data to send over USB CDC are stored in this buffer   */
uint8_t UserTxBufferFS[3][APP_TX_DATA_SIZE];

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Variables USBD_CDC_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_FunctionPrototypes USBD_CDC_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CDC_Init_FS(USBD_CDC_Function function);
static int8_t CDC_DeInit_FS(USBD_CDC_Function function);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t recipient, uint16_t index, uint8_t *pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len, USBD_CDC_Function function);
static void CDC_UART_SetLineCoding(UART_HandleTypeDef *uart, uint32_t bps, uint8_t stop, uint8_t parity, uint8_t data);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @param  function: CDC function code
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(USBD_CDC_Function function)
{
  /* USER CODE BEGIN 3 */
  /* Set Application Buffers */
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS[function], 0, function);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS[function], function);
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @param  function: CDC function code
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(USBD_CDC_Function function)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
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
  /* USER CODE BEGIN 5 */
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

    break;

    case CDC_SET_CONTROL_LINE_STATE:

    break;

    case CDC_SEND_BREAK:

    break;

  default:
    break;
  }

  return (USBD_OK);
  /* USER CODE END 5 */
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
	/* USER CODE BEGIN 6 */
	switch (function)
	{
		case CDC1:
			break;
		case CDC2:
			break;
		case CDC3:
			break;
		default:
			break;
	}
	USBD_CDC_SetRxBuffer(&hUsbDeviceFS, Buf, function);
	USBD_CDC_ReceivePacket(&hUsbDeviceFS, function);
	return (USBD_OK);
	/* USER CODE END 6 */
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
  /* USER CODE BEGIN 7 */
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  if (hcdc->function[function].TxState != 0){
    return USBD_BUSY;
  }
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len, function);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS, function);
  /* USER CODE END 7 */
  return result;
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

	// 7, 8, 9 supported: anything else falls back to 8 data bits
	switch (data) {
		case 7:
			cr1 |= UART_WORDLENGTH_7B;
			break;
		case 9:
			cr1 |= UART_WORDLENGTH_9B;
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

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
