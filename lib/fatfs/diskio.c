/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various existing       */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "fatfs_usbdisk.h"#include "fatfs_sdcard.h"

/*-----------------------------------------------------------------------*/
/* Initialize a drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (BYTE pdrv)
{
	DSTATUS stat;
	int result;

	switch (pdrv)
	{
		case DISK_USB:
			result = USB_disk_initialize ();

			if (result == 0)
			{
				stat = 0;
			}
			else
			{
				stat = STA_NOINIT;
			}

			return stat;

		case DISK_SDCARD:
			result = SD_disk_initialize ();

			if (result == 0)
			{
				stat = 0;
			}
			else
			{
				stat = STA_NOINIT;
			}

			return stat;
	}

	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE pdrv)
{
	DSTATUS stat;
	int result;

	switch (pdrv)
	{
		case DISK_USB:
			result = USB_disk_status ();

			if (result == 0)
			{
				stat = 0;
			}
			else
			{
				stat = STA_NODISK | STA_NOINIT;
			}

			return stat;

		case DISK_SDCARD:
			result = SD_disk_status ();

			if (result == 0)
			{
				stat = 0;
			}
			else
			{
				stat = STA_NODISK | STA_NOINIT;
			}

			return stat;
	}

	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
	DRESULT res;
	int result;

	switch (pdrv)
	{
		case DISK_USB:
			result = USB_disk_read (buff, sector, count);

			if (result == 0)
			{
				res = RES_OK;
			}
			else
			{
				res = RES_ERROR;
			}

			return res;

		case DISK_SDCARD:
			result = SD_disk_read (buff, sector, count);

			if (result == 0)
			{
				res = RES_OK;
			}
			else
			{
				res = RES_ERROR;
			}

			return res;

	}

	return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Write sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
	DRESULT res;
	int result;

	switch (pdrv)
	{
		case DISK_USB:
			result = USB_disk_write (buff, sector, count);

			if (result == 0)
			{
				res = RES_OK;
			}
			else
			{
				res = RES_ERROR;
			}

			return res;

		case DISK_SDCARD:
			result = SD_disk_write (buff, sector, count);

			if (result == 0)
			{
				res = RES_OK;
			}
			else
			{
				res = RES_ERROR;
			}

			return res;
	}

	return RES_PARERR;
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous functions                                               */
/*-----------------------------------------------------------------------*/

DWORD get_fattime (void)
{
	return ((2006UL - 1980) << 25)	// Year = 2006
		  | (2UL << 21)				// Month = Feb
		  | (9UL << 16)				// Day = 9
		  | (22U << 11)				// Hour = 22
		  | (30U << 5)				// Min = 30
		  | (0U >> 1)				// Sec = 0
	;
}

#if _USE_IOCTL
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff)
{
	DRESULT res;
	int result;

	switch (pdrv)
	{
		case DISK_USB:
			result = USB_disk_ioctl (cmd, buff);

			if (result == 0)
			{
				res = RES_OK;
			}
			else
			{
				res = RES_ERROR;
			}

			return res;

		case DISK_SDCARD:
			result = SD_disk_ioctl (cmd, buff);

			if (result == 0)
			{
				res = RES_OK;
			}
			else
			{
				res = RES_ERROR;
			}

			return res;
	}

	return RES_PARERR;
}
#endif
