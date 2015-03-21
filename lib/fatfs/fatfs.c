/*
 * fatfs.c
 *
 *  Created on: 20.02.2015
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "diskio.h"
#include "fatfs.h"
#include "fatfs_sdcard.h"
#include "fatfs_usbdisk.h"
#include "ff.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

static FATFS fso;

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*
 * Returns the volume name for media type as a string
 *
 * @param	dev	media device
 * @return	volume name
 */
static char* fatfs_get_vol (diskio_media_t dev)
{
	switch (dev)
	{
		case DISK_USB:
			return "0:";

		case DISK_SDCARD:
			return "1:";

		default:
			return "";
	}
}

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

/*
 * Initialization
 */
void fatfs_init (void)
{
	SDCard_Init ();
	USBDisk_Init ();
}

/*
 * Check if media exists
 *
 * @param	dev	media device
 * @return	true on success, otherwise false
 */
bool fatfs_check_media (diskio_media_t dev)
{
	switch (dev)
	{
		case DISK_USB:
			if (USBDrive_CheckMedia () == USB_PRESENT)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;

		case DISK_SDCARD:
			if (SDCard_CheckMedia () == SD_PRESENT)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;

		default:
			return false;
	}
}

/*
 * Mount media device
 *
 * @param	dev	media device
 * @return	true on success, otherwise false
 */
bool fatfs_mount (diskio_media_t dev)
{
	DWORD free_clusters;
	FATFS* fs;
	char* volume;

	volume = fatfs_get_vol (dev);

	if ((f_mount (&fso, volume, 1) == FR_OK) && (f_getfree (volume, &free_clusters, &fs) == FR_OK))
	{
		return true;
	}

	return false;
}

/*
 * Unmount media device
 *
 * @param	dev	media device
 * @return	true on success, otherwise false
 */
bool fatfs_unmount (diskio_media_t dev)
{
	if (f_mount (NULL, fatfs_get_vol (dev), 1) == FR_OK)
	{
		return true;
	}

	return false;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
