//--------------------------------------------------------------
// File     : stm32_ub_usbdisk.c
// Datum    : 11.04.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.0
// Module   : keine
// Funktion : FATFS-Dateisystem f�r USB-Medien
//            LoLevel-IO-Modul
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "fatfs_usbdisk.h"


//--------------------------------------------------------------
// Globale Variabeln
//--------------------------------------------------------------
extern USB_OTG_CORE_HANDLE   USB_OTG_Core;
extern USBH_HOST             USB_Host;


//--------------------------------------------------------------
// init der Hardware fuer die USB-Funktionen
// muss vor der Benutzung einmal gemacht werden
//--------------------------------------------------------------
void USBDisk_Init(void)
{
  
}


//--------------------------------------------------------------
// Check ob Medium eingelegt ist
// Return Wert :
//   USB_PRESENT      = wenn Medium eingelegt ist
//   USB_NOT_PRESENT  = wenn kein Medium eingelegt ist
//--------------------------------------------------------------
uint8_t USBDrive_CheckMedia(void)
{
  __IO uint8_t ret_wert = USB_NOT_PRESENT;

  if(HCD_IsDeviceConnected(&USB_OTG_Core) && (usb_msc_host_status==USB_MSC_DEV_CONNECTED)) {
    ret_wert=USB_PRESENT;
  }
  else {
    ret_wert=USB_NOT_PRESENT;
  }

  return(ret_wert);
}

//--------------------------------------------------------------
// init der Disk
// Return Wert :
//    0 = alles ok
//  < 0 = Fehler
//--------------------------------------------------------------
int USB_disk_initialize(void)
{
  int ret_wert=-1;

  if(HCD_IsDeviceConnected(&USB_OTG_Core) && (usb_msc_host_status==USB_MSC_DEV_CONNECTED)) {
    ret_wert=0; // OK
  }
  else {
    ret_wert=-1;
  }
  


  return(ret_wert);
}


//--------------------------------------------------------------
// Disk Status abfragen
// Return Wert :
//    0 = alles ok
//  < 0 = Fehler
//--------------------------------------------------------------
int USB_disk_status(void)
{
  int ret_wert=-1;

  if(HCD_IsDeviceConnected(&USB_OTG_Core) && (usb_msc_host_status==USB_MSC_DEV_CONNECTED)) {
    ret_wert=0;
  }
  else {
    ret_wert=-1;
  }

  return(ret_wert);
}


//--------------------------------------------------------------
// READ-Funktion
// Return Wert :
//    0 = alles ok
//  < 0 = Fehler
//--------------------------------------------------------------
int USB_disk_read(BYTE *buff, DWORD sector, BYTE count)
{
  int ret_wert=-1;
  BYTE status = USBH_MSC_OK;

  if(HCD_IsDeviceConnected(&USB_OTG_Core) && (usb_msc_host_status==USB_MSC_DEV_CONNECTED))
  {      
    do
    {
      status = USBH_MSC_Read10(&USB_OTG_Core, buff, sector, 512*count);
      USBH_MSC_HandleBOTXfer(&USB_OTG_Core ,&USB_Host);
      
      if(!HCD_IsDeviceConnected(&USB_OTG_Core))
      { 
        status=USBH_MSC_FAIL;
      }      
    }
    while(status == USBH_MSC_BUSY );
  } 
  else {
	  status=USBH_MSC_FAIL;
  }


  if(status==USBH_MSC_OK) {
    ret_wert=0;
  }
  else {
    ret_wert=-1;
  }

  return(ret_wert);
}


//--------------------------------------------------------------
// WRITE-Funktion
// Return Wert :
//    0 = alles ok
//  < 0 = Fehler
//--------------------------------------------------------------
int USB_disk_write(const BYTE *buff, DWORD sector, BYTE count)
{
  int ret_wert=-1;
  BYTE status = USBH_MSC_OK;


  if(HCD_IsDeviceConnected(&USB_OTG_Core) && (usb_msc_host_status==USB_MSC_DEV_CONNECTED))
  {  
    do
    {
      status = USBH_MSC_Write10(&USB_OTG_Core,(BYTE*)buff, sector, 512*count);
      USBH_MSC_HandleBOTXfer(&USB_OTG_Core, &USB_Host);
      
      if(!HCD_IsDeviceConnected(&USB_OTG_Core))
      { 
        status=USBH_MSC_FAIL;
      }
    }    
    while(status == USBH_MSC_BUSY );    
  }
  else {
	  status=USBH_MSC_FAIL;
  }


  if(status==USBH_MSC_OK) {
    ret_wert=0;
  }
  else {
    ret_wert=-1;
  }

  return(ret_wert);
}


//--------------------------------------------------------------
// IOCTL-Funktion
// Return Wert :
//    0 = alles ok
//  < 0 = Fehler
//--------------------------------------------------------------
int USB_disk_ioctl(BYTE cmd, void *buff)
{
  int ret_wert=0; // immer ok  
  
  switch (cmd) {    
    case GET_SECTOR_COUNT :  // Get number of sectors on the disk (DWORD)
      *(DWORD*)buff = (DWORD) USBH_MSC_Param.MSCapacity;
      ret_wert=0;
    break;  
    case GET_SECTOR_SIZE :   // Get R/W sector size (WORD)
      *(WORD*)buff = 512;
      ret_wert=0;
    break;
    case GET_BLOCK_SIZE :    // Get erase block size in unit of sector (DWORD)
      *(DWORD*)buff = 512;
      ret_wert=0;
    break;
    case CTRL_SYNC :         // Make sure that no pending write process
      ret_wert=0;
    break;
  }

  return(ret_wert);
}


