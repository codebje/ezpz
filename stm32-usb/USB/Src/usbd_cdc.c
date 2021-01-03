/**
  ******************************************************************************
  * @file    usbd_cdc.c
  * @author  MCD Application Team
  * @brief   This file provides the high layer firmware functions to manage the
  *          following functionalities of the USB CDC Class:
  *           - Initialization and Configuration of high and low layer
  *           - Enumeration as CDC Device (and enumeration for each implemented memory interface)
  *           - OUT/IN data transfer
  *           - Command IN transfer (class requests management)
  *           - Error management
  *
  *  @verbatim
  *
  *          ===================================================================
  *                                CDC Class Driver Description
  *          ===================================================================
  *           This driver manages the "Universal Serial Bus Class Definitions for Communications Devices
  *           Revision 1.2 November 16, 2007" and the sub-protocol specification of "Universal Serial Bus
  *           Communications Class Subclass Specification for PSTN Devices Revision 1.2 February 9, 2007"
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as CDC device with 2 data endpoints (IN and OUT) and 1 command endpoint (IN)
  *             - Requests management (as described in section 6.2 in specification)
  *             - Abstract Control Model compliant
  *             - Union Functional collection (using 1 IN endpoint for control)
  *             - Data interface class
  *
  *           These aspects may be enriched or modified for a specific user application.
  *
  *            This driver doesn't implement the following aspects of the specification
  *            (but it is possible to manage these features with some modifications on this driver):
  *             - Any class-specific aspect relative to communication classes should be managed by user application.
  *             - All communication classes other than PSTN are not managed
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc.h"
#include "usbd_ctlreq.h"


/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_CDC
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_CDC_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_CDC_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_CDC_Private_Macros
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_CDC_Private_FunctionPrototypes
  * @{
  */


static uint8_t  USBD_CDC_Init(USBD_HandleTypeDef *pdev,
                              uint8_t cfgidx);

static uint8_t  USBD_CDC_DeInit(USBD_HandleTypeDef *pdev,
                                uint8_t cfgidx);

static uint8_t  USBD_CDC_Setup(USBD_HandleTypeDef *pdev,
                               USBD_SetupReqTypedef *req);

static uint8_t  USBD_CDC_DataIn(USBD_HandleTypeDef *pdev,
                                uint8_t epnum);

static uint8_t  USBD_CDC_DataOut(USBD_HandleTypeDef *pdev,
                                 uint8_t epnum);

static uint8_t  USBD_CDC_EP0_RxReady(USBD_HandleTypeDef *pdev);

static uint8_t  *USBD_CDC_GetFSCfgDesc(uint16_t *length);

uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor(uint16_t *length);

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CDC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/**
  * @}
  */

/** @defgroup USBD_CDC_Private_Variables
  * @{
  */


/* CDC interface class callbacks structure */
USBD_ClassTypeDef  USBD_CDC =
{
  USBD_CDC_Init,
  USBD_CDC_DeInit,
  USBD_CDC_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_CDC_EP0_RxReady,
  USBD_CDC_DataIn,
  USBD_CDC_DataOut,
  NULL,
  NULL,
  NULL,
  USBD_CDC_GetFSCfgDesc,
  USBD_CDC_GetDeviceQualifierDescriptor,
};

/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_CDC_CfgFSDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /* Configuration Descriptor */
  0x09,   								/* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,      	/* bDescriptorType: Configuration */
  USB_CDC_CONFIG_DESC_SIZ,      		/* wTotalLength:no of returned bytes */
  0x00,
  0x02,   								/* bNumInterfaces: 2 interface */
  0x01,   								/* bConfigurationValue: Configuration value */
  0x00,   								/* iConfiguration: Index of string descriptor describing the configuration */
  0x80,   								/* bmAttributes: bus powered */
  0xFA,   								/* MaxPower 500 mA */

  /*---------------------------------------------------------------------------*/
  /* Interface Association Descriptor: CDC interface #1 */
  0x08, 								/* bLength */
  USB_DESC_TYPE_INTERFACE_ASSOCIATION, 	/* bDescriptorType: Interface Association */
  0x00,									/* bFirstInterface */
  0x02,									/* bInterfaceCount */
  USB_CLASS_CIC,						/* bFunctionClass: Communications Interface Class */
  USB_CLASS_CIC_SUBCLASS_ACM,			/* bFunctionSubClass: Abstract Control Model */
  USB_CLASS_CIC_PROTOCOL_NONE,			/* bFunctionProtocol: none */
  0x00,									/* iFunction: Index of string descriptor describing the function */

  /*---------------------------------------------------------------------------*/

  /* Interface Descriptor */
  0x09,   								/* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  			/* bDescriptorType: Interface */
  0x00,   								/* bInterfaceNumber: Number of Interface */
  0x00,   								/* bAlternateSetting: Alternate setting */
  0x01,   								/* bNumEndpoints: One endpoints used */
  USB_CLASS_CIC,   						/* bInterfaceClass: Communication Interface Class */
  USB_CLASS_CIC_SUBCLASS_ACM,   		/* bInterfaceSubClass: Abstract Control Model */
  USB_CLASS_CIC_PROTOCOL_ITUV250,   	/* bInterfaceProtocol: Common AT commands */
  0x00,   								/* iInterface: */

  /* Header Functional Descriptor */
  0x05,   								/* bLength: Endpoint Descriptor size */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x00,   								/* bDescriptorSubtype: Header Func Desc */
  0x10,   								/* bcdCDC: spec release number */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x01,   								/* bDescriptorSubtype: Call Management Func Desc */
  0x00,   								/* bmCapabilities: D0+D1 */
  0x01,   								/* bDataInterface: 1 */

  /* ACM Functional Descriptor*/
  0x04,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x02,   								/* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   								/* bmCapabilities */

  /* Union Functional Descriptor */
  0x05,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x06,   								/* bDescriptorSubtype: Union func desc */
  0x00,   								/* bMasterInterface: Communication class interface */
  0x01,   								/* bSlaveInterface0: Data Class Interface */

  /* Interrupt Endpoint Descriptor */
  0x07,                           		/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,   			/* bDescriptorType: Endpoint */
  CDC_CMD_EP(CDC1),                     /* bEndpointAddress */
  0x03,                           		/* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),     		/* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_FS_BINTERVAL,                     /* bInterval: */
  /*---------------------------------------------------------------------------*/

  /* Data class interface descriptor */
  0x09,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE, 			 	/* bDescriptorType: */
  0x01,   								/* bInterfaceNumber: Number of Interface */
  0x00,   								/* bAlternateSetting: Alternate setting */
  0x02,   								/* bNumEndpoints: Two endpoints used */
  0x0A,   								/* bInterfaceClass: CDC */
  0x00,   								/* bInterfaceSubClass: */
  0x00,   								/* bInterfaceProtocol: */
  0x00,   								/* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      			/* bDescriptorType: Endpoint */
  CDC_OUT_EP(CDC1),                    	/* bEndpointAddress */
  0x02,                              	/* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                              	/* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      			/* bDescriptorType: Endpoint */
  CDC_IN_EP(CDC1),                     	/* bEndpointAddress */
  0x02,                              	/* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                               	/* bInterval: ignore for Bulk transfer */

  /*---------------------------------------------------------------------------*/
  /* Interface Association Descriptor: CDC interface #2 */
  0x08, 								/* bLength */
  USB_DESC_TYPE_INTERFACE_ASSOCIATION, 	/* bDescriptorType: Interface Association */
  0x02,									/* bFirstInterface */
  0x02,									/* bInterfaceCount */
  USB_CLASS_CIC,						/* bFunctionClass: Communications Interface Class */
  USB_CLASS_CIC_SUBCLASS_ACM,			/* bFunctionSubClass: Abstract Control Model */
  USB_CLASS_CIC_PROTOCOL_NONE,			/* bFunctionProtocol: none */
  0x00,									/* iFunction: Index of string descriptor describing the function */

  /*---------------------------------------------------------------------------*/

  /* Interface Descriptor */
  0x09,   								/* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  			/* bDescriptorType: Interface */
  0x02,   								/* bInterfaceNumber: Number of Interface */
  0x00,   								/* bAlternateSetting: Alternate setting */
  0x01,   								/* bNumEndpoints: One endpoints used */
  USB_CLASS_CIC,   						/* bInterfaceClass: Communication Interface Class */
  USB_CLASS_CIC_SUBCLASS_ACM,   		/* bInterfaceSubClass: Abstract Control Model */
  USB_CLASS_CIC_PROTOCOL_ITUV250,   	/* bInterfaceProtocol: Common AT commands */
  0x00,   								/* iInterface: */

  /* Header Functional Descriptor */
  0x05,   								/* bLength: Endpoint Descriptor size */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x00,   								/* bDescriptorSubtype: Header Func Desc */
  0x10,   								/* bcdCDC: spec release number */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x01,   								/* bDescriptorSubtype: Call Management Func Desc */
  0x00,   								/* bmCapabilities: D0+D1 */
  0x01,   								/* bDataInterface: 1 */

  /* ACM Functional Descriptor*/
  0x04,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x02,   								/* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   								/* bmCapabilities */

  /* Union Functional Descriptor */
  0x05,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x06,   								/* bDescriptorSubtype: Union func desc */
  0x02,   								/* bMasterInterface: Communication class interface */
  0x03,   								/* bSlaveInterface0: Data Class Interface */

  /* Interrupt Endpoint Descriptor */
  0x07,                           		/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,   			/* bDescriptorType: Endpoint */
  CDC_CMD_EP(CDC2),                     /* bEndpointAddress */
  0x03,                           		/* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),     		/* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_FS_BINTERVAL,                     /* bInterval: */
  /*---------------------------------------------------------------------------*/

  /* Data class interface descriptor */
  0x09,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE, 			 	/* bDescriptorType: */
  0x03,   								/* bInterfaceNumber: Number of Interface */
  0x00,   								/* bAlternateSetting: Alternate setting */
  0x02,   								/* bNumEndpoints: Two endpoints used */
  0x0A,   								/* bInterfaceClass: CDC */
  0x00,   								/* bInterfaceSubClass: */
  0x00,   								/* bInterfaceProtocol: */
  0x00,   								/* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      			/* bDescriptorType: Endpoint */
  CDC_OUT_EP(CDC2),                    	/* bEndpointAddress */
  0x02,                              	/* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                              	/* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      			/* bDescriptorType: Endpoint */
  CDC_IN_EP(CDC2),                     	/* bEndpointAddress */
  0x02,                              	/* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                               	/* bInterval: ignore for Bulk transfer */


  /*---------------------------------------------------------------------------*/
  /* Interface Association Descriptor: CDC interface #3 */
  0x08, 								/* bLength */
  USB_DESC_TYPE_INTERFACE_ASSOCIATION, 	/* bDescriptorType: Interface Association */
  0x04,									/* bFirstInterface */
  0x02,									/* bInterfaceCount */
  USB_CLASS_CIC,						/* bFunctionClass: Communications Interface Class */
  USB_CLASS_CIC_SUBCLASS_ACM,			/* bFunctionSubClass: Abstract Control Model */
  USB_CLASS_CIC_PROTOCOL_NONE,			/* bFunctionProtocol: none */
  0x00,									/* iFunction: Index of string descriptor describing the function */

  /*---------------------------------------------------------------------------*/

  /* Interface Descriptor */
  0x09,   								/* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  			/* bDescriptorType: Interface */
  0x04,   								/* bInterfaceNumber: Number of Interface */
  0x00,   								/* bAlternateSetting: Alternate setting */
  0x01,   								/* bNumEndpoints: One endpoints used */
  USB_CLASS_CIC,   						/* bInterfaceClass: Communication Interface Class */
  USB_CLASS_CIC_SUBCLASS_ACM,   		/* bInterfaceSubClass: Abstract Control Model */
  USB_CLASS_CIC_PROTOCOL_ITUV250,   	/* bInterfaceProtocol: Common AT commands */
  0x00,   								/* iInterface: */

  /* Header Functional Descriptor */
  0x05,   								/* bLength: Endpoint Descriptor size */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x00,   								/* bDescriptorSubtype: Header Func Desc */
  0x10,   								/* bcdCDC: spec release number */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x01,   								/* bDescriptorSubtype: Call Management Func Desc */
  0x00,   								/* bmCapabilities: D0+D1 */
  0x01,   								/* bDataInterface: 1 */

  /* ACM Functional Descriptor*/
  0x04,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x02,   								/* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   								/* bmCapabilities */

  /* Union Functional Descriptor */
  0x05,   								/* bFunctionLength */
  0x24,   								/* bDescriptorType: CS_INTERFACE */
  0x06,   								/* bDescriptorSubtype: Union func desc */
  0x04,   								/* bMasterInterface: Communication class interface */
  0x05,   								/* bSlaveInterface0: Data Class Interface */

  /* Interrupt Endpoint Descriptor */
  0x07,                           		/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,   			/* bDescriptorType: Endpoint */
  CDC_CMD_EP(CDC3),                     /* bEndpointAddress */
  0x03,                           		/* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),     		/* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_FS_BINTERVAL,                     /* bInterval: */
  /*---------------------------------------------------------------------------*/

  /* Data class interface descriptor */
  0x09,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE, 			 	/* bDescriptorType: */
  0x05,   								/* bInterfaceNumber: Number of Interface */
  0x00,   								/* bAlternateSetting: Alternate setting */
  0x02,   								/* bNumEndpoints: Two endpoints used */
  0x0A,   								/* bInterfaceClass: CDC */
  0x00,   								/* bInterfaceSubClass: */
  0x00,   								/* bInterfaceProtocol: */
  0x00,   								/* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      			/* bDescriptorType: Endpoint */
  CDC_OUT_EP(CDC3),                    	/* bEndpointAddress */
  0x02,                              	/* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                              	/* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   								/* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      			/* bDescriptorType: Endpoint */
  CDC_IN_EP(CDC3),                     	/* bEndpointAddress */
  0x02,                              	/* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00                               	/* bInterval: ignore for Bulk transfer */

} ;

/**
  * @}
  */

/** @defgroup USBD_CDC_Private_Functions
  * @{
  */

/**
  * @brief  USBD_CDC_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0U;
  USBD_CDC_HandleTypeDef   *hcdc;

  if (pdev->dev_speed != USBD_SPEED_FULL)
  {
	  return USBD_FAIL;
  }

  for (USBD_CDC_Function fn = CDC1; fn <= CDC3; fn++) {

	  /* Open EP IN */
	  USBD_LL_OpenEP(pdev, CDC_IN_EP(fn), USBD_EP_TYPE_BULK,
			  CDC_DATA_FS_IN_PACKET_SIZE);

	  pdev->ep_in[CDC_IN_EP(fn) & 0xFU].is_used = 1U;

	  /* Open EP OUT */
	  USBD_LL_OpenEP(pdev, CDC_OUT_EP(fn), USBD_EP_TYPE_BULK,
			  CDC_DATA_FS_OUT_PACKET_SIZE);

	  pdev->ep_out[CDC_OUT_EP(fn) & 0xFU].is_used = 1U;

	  /* Open Command IN EP */
	  USBD_LL_OpenEP(pdev, CDC_CMD_EP(fn), USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
	  pdev->ep_in[CDC_CMD_EP(fn) & 0xFU].is_used = 1U;

  }

  pdev->pClassData = USBD_malloc(sizeof(USBD_CDC_HandleTypeDef));

  if (pdev->pClassData == NULL || pdev->dev_speed == USBD_SPEED_HIGH)
  {
    ret = 1U;
  }
  else
  {
    hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

    for (USBD_CDC_Function fn = CDC1; fn <= CDC3; fn++) {

    	/* Init  physical Interface components */
    	((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Init(fn);

    	/* Init Xfer states */
    	hcdc->function[fn].TxState = 0U;
    	hcdc->function[fn].RxState = 0U;

    	/* Prepare Out endpoint to receive next packet */
    	USBD_LL_PrepareReceive(pdev, CDC_OUT_EP(CDC1), hcdc->function[fn].RxBuffer,
    			CDC_DATA_FS_OUT_PACKET_SIZE);
    }
  }
  return ret;
}

/**
  * @brief  USBD_CDC_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0U;

  for (USBD_CDC_Function fn = CDC1; fn <= CDC3; fn++) {

	  /* Close EP IN */
	  USBD_LL_CloseEP(pdev, CDC_IN_EP(fn));
	  pdev->ep_in[CDC_IN_EP(fn) & 0xFU].is_used = 0U;

	  /* Close EP OUT */
	  USBD_LL_CloseEP(pdev, CDC_OUT_EP(fn));
	  pdev->ep_out[CDC_OUT_EP(fn) & 0xFU].is_used = 0U;

	  /* Close Command IN EP */
	  USBD_LL_CloseEP(pdev, CDC_CMD_EP(fn));
	  pdev->ep_in[CDC_CMD_EP(fn) & 0xFU].is_used = 0U;

  }

  /* DeInit  physical Interface components */
  if (pdev->pClassData != NULL)
  {
	  for (USBD_CDC_Function fn = CDC1; fn <= CDC3; fn++) {
		  ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->DeInit(fn);
	  }
	  USBD_free(pdev->pClassData);
	  pdev->pClassData = NULL;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_CDC_Setup(USBD_HandleTypeDef *pdev,
                               USBD_SetupReqTypedef *req)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;
  uint8_t ifalt = 0U;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS :
      if (req->wLength)
      {
        if (req->bmRequest & 0x80U)
        {
          ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest,
        		  req->bmRequest & USB_REQ_RECIPIENT_MASK,
				  req->wIndex,
				  (uint8_t *)(void *)hcdc->data,
				  req->wLength);

          USBD_CtlSendData(pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
        }
        else
        {
          hcdc->CmdOpCode = req->bRequest;
          hcdc->CmdLength = (uint8_t)req->wLength;
          hcdc->CmdRecipient = req->bmRequest & USB_REQ_RECIPIENT_MASK;
          hcdc->CmdIndex = req->wIndex;

          USBD_CtlPrepareRx(pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
        }
      }
      else
      {
        ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest,
        		req->bmRequest & USB_REQ_RECIPIENT_MASK,
				req->wIndex,
				(uint8_t *)(void *)req,
				0U);
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_STATUS:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, (uint8_t *)(void *)&status_info, 2U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, &ifalt, 1U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (pdev->dev_state != USBD_STATE_CONFIGURED)
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;

    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;
  PCD_HandleTypeDef *hpcd = pdev->pData;
  USBD_CDC_Function fn = CDC_EP_FN(epnum);

  if (pdev->pClassData != NULL)
  {
    if ((pdev->ep_in[epnum].total_length > 0U) && ((pdev->ep_in[epnum].total_length % hpcd->IN_ep[epnum].maxpacket) == 0U))
    {
      /* Update the packet total length */
      pdev->ep_in[epnum].total_length = 0U;

      /* Send ZLP */
      USBD_LL_Transmit(pdev, epnum, NULL, 0U);
    }
    else
    {
      hcdc->function[fn].TxState = 0U;
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;
  USBD_CDC_Function fn = CDC_EP_FN(epnum);

  /* Get the received data length */
  hcdc->function[fn].RxLength = USBD_LL_GetRxDataSize(pdev, epnum);

  /* USB data will be immediately processed, this allow next USB traffic being
  NAKed till the end of the application Xfer */
  if (pdev->pClassData != NULL)
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Receive(hcdc->function[fn].RxBuffer, &hcdc->function[fn].RxLength, fn);

    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_CDC_EP0_RxReady
  *         Handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_CDC_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  if ((pdev->pUserData != NULL) && (hcdc->CmdOpCode != 0xFFU))
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(hcdc->CmdOpCode,
    												  hcdc->CmdRecipient,
													  hcdc->CmdIndex,
                                                      (uint8_t *)(void *)hcdc->data,
                                                      (uint16_t)hcdc->CmdLength);
    hcdc->CmdOpCode = 0xFFU;

  }
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_GetFSCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_CDC_GetFSCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_CDC_CfgFSDesc);
  return USBD_CDC_CfgFSDesc;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor(uint16_t *length)
{
  *length = sizeof(USBD_CDC_DeviceQualifierDesc);
  return USBD_CDC_DeviceQualifierDesc;
}

/**
* @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t  USBD_CDC_RegisterInterface(USBD_HandleTypeDef   *pdev,
                                    USBD_CDC_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if (fops != NULL)
  {
    pdev->pUserData = fops;
    ret = USBD_OK;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_SetTxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Tx Buffer
  * @retval status
  */
uint8_t  USBD_CDC_SetTxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff,
                              uint16_t length,
							  USBD_CDC_Function function)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  hcdc->function[function].TxBuffer = pbuff;
  hcdc->function[function].TxLength = length;

  return USBD_OK;
}


/**
  * @brief  USBD_CDC_SetRxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Rx Buffer
  * @retval status
  */
uint8_t  USBD_CDC_SetRxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff,
							  USBD_CDC_Function function)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  hcdc->function[function].RxBuffer = pbuff;

  return USBD_OK;
}

/**
  * @brief  USBD_CDC_TransmitPacket
  *         Transmit packet on IN endpoint
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev, USBD_CDC_Function function)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  if (pdev->pClassData != NULL)
  {
    if (hcdc->function[function].TxState == 0U)
    {
      /* Tx Transfer in progress */
      hcdc->function[function].TxState = 1U;

      /* Update the packet total length */
      pdev->ep_in[CDC_IN_EP(function) & 0xFU].total_length = hcdc->function[function].TxLength;

      /* Transmit next packet */
      USBD_LL_Transmit(pdev, CDC_IN_EP(function), hcdc->function[function].TxBuffer,
                       (uint16_t)hcdc->function[function].TxLength);

      return USBD_OK;
    }
    else
    {
      return USBD_BUSY;
    }
  }
  else
  {
    return USBD_FAIL;
  }
}


/**
  * @brief  USBD_CDC_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev, USBD_CDC_Function function)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  /* Suspend or Resume USB Out process */
  if (pdev->pClassData != NULL && pdev->dev_speed == USBD_SPEED_FULL)
  {
    /* Prepare Out endpoint to receive next packet */
    USBD_LL_PrepareReceive(pdev,
                           CDC_OUT_EP(function),
                           hcdc->function[function].RxBuffer,
                           CDC_DATA_FS_OUT_PACKET_SIZE);
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
