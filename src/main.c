/*
 * main.c
 *
 *  Created on: 13.05.2014
 *      Author: Florian
 */

/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "button.h"
#include "debug.h"
#include "fatfs.h"
#include "gfx.h"
#include "i2c.h"
#include "jpeg.h"
#include "led.h"
#include "main.h"
#include "sdram.h"
#include "spi.h"
#include "images.h"
#include "touch.h"
#include "usb_msc_host.h"

extern void D_DoomMain (void);

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

#define USB_RETRIES 100
 
/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

volatile uint32_t systime;

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

static void hw_init (void)
{
	/* configure SysTick for 1 ms interrupt */
	if (SysTick_Config (168000000 / 1000))
	{
		/* capture error */
		while (1);
	}

	/* configure the SysTick handler priority */
	NVIC_SetPriority (SysTick_IRQn, 0x0);
}

static void show_image (const uint8_t* img)
{
	gfx_image_t title;
	gfx_obj_t obj_title;

	title.width = 240;
	title.height = 320;
	title.pixel_format = GFX_PIXEL_FORMAT_RGB565;
	title.pixel_data = malloc (title.width * title.height * 2);

	obj_title.obj_type = GFX_OBJ_IMG;
	obj_title.coords.dest_x = 0;
	obj_title.coords.dest_y = 0;
	obj_title.coords.source_x = 0;
	obj_title.coords.source_y = 0;
	obj_title.coords.source_w = title.width;
	obj_title.coords.source_h = title.height;
	obj_title.data = &title;

	jpeg_decode ((uint8_t*)img, title.pixel_data, 0, 0);

	gfx_objects[0] = &obj_title;

	gfx_draw_objects ();

	sleep_ms (500);

	gfx_delete_objects ();

	free (title.pixel_data);
}

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

int main(void)
{
	uint32_t start;
	usb_msc_host_status_t usb_status;

	SystemInit ();
	hw_init ();
	sdram_init ();
	debug_init ();

	printf ("\n\033[1;31m\r\nSTM32Doom\033[0m\n");

	spi_init ();
	i2c_init ();
	lcd_init ();
	gfx_init ();

	// show title as soon as possible
	show_image (img_loading);

	fatfs_init ();
	usb_msc_host_init ();
	led_init ();
	touch_init ();
	button_init ();

	// wait for USB to be connected
	start = systime;
	usb_status = USB_MSC_DEV_CONNECTED;
	
	while (systime - start < 20000)
	{
		usb_status = usb_msc_host_main ();
		
		if (usb_status == USB_MSC_DEV_CONNECTED)
		{
			break;
		}
	}
	
	if (usb_status != USB_MSC_DEV_CONNECTED)
	{
		show_image (img_no_usb);
		fatal_error ("USB not connected\n");
	}

	if (fatfs_mount (DISK_USB))
	{
		printf ("USB mounted\n");
	}
	else
	{
		show_image (img_no_usb);
		fatal_error ("USB mount failed\n");
	}
	
	led_set (LED_GREEN, LED_STATE_ON);

	D_DoomMain ();

	while (1)
	{
	}
}

/*
 * Sleep for the specified number of milliseconds
 */
void sleep_ms (uint32_t ms)
{
	uint32_t wait_start;

	wait_start = systime;

	while (systime - wait_start < ms)
	{
		nop ();
	}
}

/*
 * System tick interrupt handler
 */
void SysTick_Handler (void)
{
	systime++;
}

/*
 * Show fatal error message and stop in endless loop
 */
void fatal_error (const char* message)
{
	printf ("FATAL ERROR: %s\n", message);

	while (1)
	{
	}
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
