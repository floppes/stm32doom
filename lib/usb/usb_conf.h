/*
 * usb_conf.h
 *
 *  Created on: 21.03.2015
 *      Author: Florian
 */


#ifndef LIB_USB_USB_CONF_H_
#define LIB_USB_USB_CONF_H_

/*---------------------------------------------------------------------*
 *  additional includes                                                *
 *---------------------------------------------------------------------*/

#include "stm32f4xx.h"

/*---------------------------------------------------------------------*
 *  global definitions                                                 *
 *---------------------------------------------------------------------*/

#define RX_FIFO_HS_SIZE		512
#define TXH_NP_HS_FIFOSIZ	256
#define TXH_P_HS_FIFOSIZ	256

#define USB_OTG_HS_INTERNAL_DMA_ENABLED
#define	USE_USB_OTG_HS
#define	USB_OTG_EMBEDDED_PHY_ENABLED
#define	USB_OTG_EXTERNAL_VBUS_ENABLED
#define	USB_OTG_HS_CORE
#define	USB_HOST_MODE
#define	USE_HOST_MODE
#undef	USB_SUPPORT_USER_STRING_DESC
#undef	USE_USB_OTG_FS
#undef	USB_OTG_ULPI_PHY_ENABLED
#undef	USE_DEVICE_MODE
#undef	DUAL_ROLE_MODE_ENABLED
#undef	USE_OTG_MODE
#undef	USB_OTG_FS_CORE
#undef	VBUS_SENSING_ENABLED
#undef	USB_OTG_HS_SOF_OUTPUT_ENABLED
#undef	USB_OTG_HS_LOW_PWR_MGMT_SUPPORT
#undef	USB_OTG_INTERNAL_VBUS_ENABLED

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
	#define __ALIGN_END    __attribute__ ((aligned (4)))
	#define __ALIGN_BEGIN
#else
	#define __ALIGN_BEGIN
	#define __ALIGN_END
#endif

#ifndef __packed
	#define __packed    __attribute__ ((__packed__))
#endif

/*---------------------------------------------------------------------*
 *  type declarations                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  function prototypes                                                *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  global data                                                        *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  inline functions and function-like macros                          *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/

#endif /* LIB_USB_USB_CONF_H_ */
