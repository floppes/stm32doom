/*
 * usbh_usr.c
 *
 *  Created on: 21.03.2015
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include "usbh_usr.h"
#include "usb_msc_host.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

USBH_Usr_cb_TypeDef USR_Callbacks =
{
  USBH_USR_Init,
  USBH_USR_DeInit,
  USBH_USR_DeviceAttached,
  USBH_USR_ResetDevice,
  USBH_USR_DeviceDisconnected,
  USBH_USR_OverCurrentDetected,
  USBH_USR_DeviceSpeedDetected,
  USBH_USR_Device_DescAvailable,
  USBH_USR_DeviceAddressAssigned,
  USBH_USR_Configuration_DescAvailable,
  USBH_USR_Manufacturer_String,
  USBH_USR_Product_String,
  USBH_USR_SerialNum_String,
  USBH_USR_EnumerationDone,
  USBH_USR_UserInput,
  USBH_USR_MSC_Application,
  USBH_USR_DeviceNotSupported,
  USBH_USR_UnrecoveredError
};

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

static uint8_t USBH_USR_ApplicationState = USH_USR_FS_INIT;

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void USBH_USR_Init(void)
{
}

/*
 * Called after device was attached
 */
void USBH_USR_DeviceAttached(void)
{
	usb_msc_host_status = USB_MSC_DEV_DETACHED;
}

void USBH_USR_UnrecoveredError (void)
{
	usb_msc_host_status = USB_MSC_DEV_DETACHED;
}

/*
 * Called after device was detached
 */
void USBH_USR_DeviceDisconnected (void)
{
	usb_msc_host_status = USB_MSC_DEV_DETACHED;
}

/*
 * Called after device was attached
 */
void USBH_USR_ResetDevice (void)
{
}

/*
 * Called after device was attached
 */
void USBH_USR_DeviceSpeedDetected (uint8_t DeviceSpeed)
{
	if ((DeviceSpeed != HPRT0_PRTSPD_FULL_SPEED) && (DeviceSpeed != HPRT0_PRTSPD_LOW_SPEED))
	{
		usb_msc_host_status = USB_MSC_SPEED_ERROR;
	}
}

/*
 * Called after device was attached
 */
void USBH_USR_Device_DescAvailable (void* DeviceDesc)
{
}

/*
 * Called after device was attached
 */
void USBH_USR_DeviceAddressAssigned (void)
{
}

/*
 * Called after device was attached
 */
void USBH_USR_Configuration_DescAvailable (USBH_CfgDesc_TypeDef* cfgDesc, USBH_InterfaceDesc_TypeDef* itfDesc, USBH_EpDesc_TypeDef* epDesc)
{
}

/*
 * Called after device was attached
 */
void USBH_USR_Manufacturer_String (void *ManufacturerString)
{
}

/*
 * Called after device was attached
 */
void USBH_USR_Product_String (void *ProductString)
{
}

/*
 * Called after device was attached
 */
void USBH_USR_SerialNum_String (void *SerialNumString)
{
}

/*
 * Called after device was attached
 */
void USBH_USR_EnumerationDone (void)
{
}

/*
 * Called when device is not supported
 */
void USBH_USR_DeviceNotSupported (void)
{
	usb_msc_host_status = USB_MSC_DEV_NOT_SUPPORTED;
}


USBH_USR_Status USBH_USR_UserInput (void)
{
	return USBH_USR_RESP_OK;
}

/*
 * Called when overcurrent detected
 */
void USBH_USR_OverCurrentDetected (void)
{
	usb_msc_host_status = USB_MSC_OVER_CURRENT;
}

/*
 * Called cyclically when device is attached
 */
int USBH_USR_MSC_Application (void)
{
	switch (USBH_USR_ApplicationState)
	{
		case USH_USR_FS_INIT:
			usb_msc_host_status = USB_MSC_DEV_CONNECTED;

			if (USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
			{
				usb_msc_host_status = USB_MSC_DEV_WRITE_PROTECT;
			}

			USBH_USR_ApplicationState = USH_USR_FS_LOOP;
			break;

		case USH_USR_FS_LOOP:
			break;

		default:
			break;
	}

	return 0;
}

/*
 * Called when device was detached
 */
void USBH_USR_DeInit (void)
{
	USBH_USR_ApplicationState = USH_USR_FS_INIT;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
