/*
 * gfx.c
 *
 *  Created on: 24.05.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stm32f4xx_dma2d.h"
#include "ff.h"
#include "font.h"
#include "gfx.h"
#include "jpeg.h"
#include "lcd.h"
#include "main.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

#define abs(X) (((X) < 0 ) ? -(X) : (X))
#define sgn(X) (((X) < 0 ) ? -1 : 1)

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

gfx_obj_t* gfx_objects[GFX_MAX_OBJECTS];

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*
 * Map ISO8859-1 character to font character index
 *
 * @param[in]	chr	Character to map
 * @return		Mapped character
 */
static inline char gfx_map_char (char chr)
{
	switch (chr)
	{
		case 0xE4:
			// ae
			return (char)128;

		case 0xF6:
			// oe
			return (char)129;

		case 0xFC:
			// ue
			return (char)130;

		case 0xC4:
			// AE
			return (char)131;

		case 0xD6:
			// OE
			return (char)132;

		case 0xDC:
			// UE
			return (char)133;

		case 0xDF:
			// ss
			return (char)134;

		case 0xA7:
			// Euro
			return (char)135;

		default:
			return chr;
	}
}

/*
 * Convert pixel format into color mode
 *
 * @param[in]	pixel_format	Pixel format to convert
 * @return		Converted color mode
 */
static uint32_t gfx_convert_colormode (gfx_pixel_format_t pixel_format)
{
	switch (pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			return DMA2D_RGB565;

		case GFX_PIXEL_FORMAT_ARGB8888:
			return DMA2D_ARGB8888;

		default:
			fatal_error ("unknown pixel format");
			return 0;
	}
}

/*
 * Returns the number of bytes per pixel, depending on the pixel format
 *
 * @param[in]	img	Image
 * @return		Bytes per pixel
 */
static inline uint8_t gfx_bytes_per_pixel (gfx_image_t* img)
{
	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			return 2;

		case GFX_PIXEL_FORMAT_ARGB8888:
			return 4;

		default:
			fatal_error ("unknown pixel format");
			return 0;
	}
}

/*
 * Draw a single pixel to an image
 *
 * @param[in]	img		Image to draw onto
 * @param[in]	x		X
 * @param[in]	y		Y
 * @param[in]	color	Color
 */
static inline void gfx_draw_pixel (gfx_image_t* img, uint16_t x, uint16_t y, uint32_t color)
{
	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			((uint16_t*)img->pixel_data)[y * img->width + x] = (uint16_t)(color & 0xFFFF);
			break;

		case GFX_PIXEL_FORMAT_ARGB8888:
			((uint32_t*)img->pixel_data)[y * img->width + x] = color;
			break;

		default:
			fatal_error ("unknown pixel format");
	}
}

/*
 * Blend pixel with background (using both alpha channels)
 * The resulting color is the blending of background and foreground color
 * Porter-Duff algorithm
 *
 * @param[in]	img		Image
 * @param[in]	x		X
 * @param[in]	y		Y
 * @param[in]	color	Foreground color (alpha ignored)
 * @param[in]	alpha	Foreground alpha channel value
 */
#if 1
static inline void gfx_blend_pixel (gfx_image_t* img, uint16_t x, uint16_t y, uint32_t color, uint8_t alpha)
{
	uint32_t color_back;

	uint8_t rf;
	uint8_t gf;
	uint8_t bf;

	uint8_t ab;
	uint8_t rb;
	uint8_t gb;
	uint8_t bb;

	uint8_t r;
	uint8_t g;
	uint8_t b;

	uint8_t a_mult;
	uint8_t a_out;

	color_back = gfx_get_pixel (img, x, y);

	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			rf = GFX_RGB565_R(color);
			gf = GFX_RGB565_G(color);
			bf = GFX_RGB565_B(color);

			rb = GFX_RGB565_R(color_back);
			gb = GFX_RGB565_G(color_back);
			bb = GFX_RGB565_B(color_back);
			ab = GFX_OPAQUE;
			break;

		case GFX_PIXEL_FORMAT_ARGB8888:
			rf = GFX_ARGB8888_R(color);
			gf = GFX_ARGB8888_G(color);
			bf = GFX_ARGB8888_B(color);

			rb = GFX_ARGB8888_R(color_back);
			gb = GFX_ARGB8888_G(color_back);
			bb = GFX_ARGB8888_B(color_back);
			ab = GFX_ARGB8888_A(color_back);
			break;

		default:
			fatal_error ("unknown pixel format");
			return;
	}

	a_mult = (uint8_t)((uint16_t)(alpha * ab) / 255);
	a_out = alpha + ab - a_mult;

	r = (uint8_t)((uint16_t)((rf * alpha) + (rb * ab) - (rb * a_mult)) / a_out);
	g = (uint8_t)((uint16_t)((gf * alpha) + (gb * ab) - (gb * a_mult)) / a_out);
	b = (uint8_t)((uint16_t)((bf * alpha) + (bb * ab) - (bb * a_mult)) / a_out);

	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			color = GFX_RGB565(r, g, b);
			break;

		case GFX_PIXEL_FORMAT_ARGB8888:
			color = GFX_ARGB8888(r, g, b, a_out);
			break;

		default:
			return;
	}

	gfx_draw_pixel (img, x, y, color);
}
#else
/*
 * Blend pixel with background (using only foreground alpha channel)
 * The resulting color is the blending of background and foreground color
 *
 * @param[in]	img		Image
 * @param[in]	x		X
 * @param[in]	y		Y
 * @param[in]	color	Foreground color
 */
static inline void gfx_blend_pixel (gfx_image_t* img, uint16_t x, uint16_t y, uint32_t color)
{
	uint32_t color_back;

	uint8_t mask;
	uint8_t temp;
	uint8_t rf;
	uint8_t rb;
	uint8_t gf;
	uint8_t gb;
	uint8_t bf;
	uint8_t bb;

	color_back = gfx_get_pixel (img, x, y);

	rf = GFX_ARGB8888_R(color);
	gf = GFX_ARGB8888_G(color);
	bf = GFX_ARGB8888_B(color);
	mask = (uint8_t)!GFX_ARGB8888_A(color);

	rb = GFX_ARGB8888_R(color_back);
	gb = GFX_ARGB8888_G(color_back);
	bb = GFX_ARGB8888_B(color_back);

	temp = ((255 - mask) * rf) / 255 + (mask * rb / 255);
	rf = temp & 0xFF;

	temp = ((255 - mask) * gf) / 255 + (mask * gb / 255);
	gf = temp & 0xFF;

	temp = ((255 - mask) * bf) / 255 + (mask * bb / 255);
	bf = temp & 0xFF;

	color = GFX_ARGB8888(rf, gf, bf, GFX_OPAQUE);

	gfx_set_pixel (img, x, y, color);
}
#endif

/*
 * Draw an RGB565 image to the frame buffer
 *
 * @param[in]	img		Image to draw
 * @param[in]	coord	Coordinates
 */
static void gfx_draw_img_rgb565 (gfx_image_t* img, gfx_coord_t* coord)
{
	DMA2D_InitTypeDef DMA2D_InitStruct;
	DMA2D_FG_InitTypeDef DMA2D_FG_InitStruct;

	uint32_t dest_address;
	uint32_t source_address;
	uint32_t offset;
	uint16_t picture_width;
	uint16_t picture_height;

	picture_width = img->width;
	picture_height = img->height;

	// check for dimensions
	if (coord->source_w == 0) return;
	if (coord->source_h == 0) return;
	if (coord->source_x + coord->source_w > picture_width) return;
	if (coord->source_y + coord->source_h > picture_height) return;
	if (coord->dest_x + coord->source_w > GFX_MAX_WIDTH) return;
	if (coord->dest_y + coord->source_h > GFX_MAX_HEIGHT) return;

	// target address in display RAM
	dest_address = lcd_frame_buffer + 2 * (GFX_MAX_WIDTH * coord->dest_y + coord->dest_x);

	// source address in image
	offset = 2 * (picture_width * coord->source_y + coord->source_x);
	source_address = (uint32_t)&img->pixel_data[offset];

	DMA2D_DeInit ();

	DMA2D_StructInit (&DMA2D_InitStruct);
	DMA2D_InitStruct.DMA2D_Mode = DMA2D_M2M;
	DMA2D_InitStruct.DMA2D_CMode = DMA2D_RGB565;
	DMA2D_InitStruct.DMA2D_OutputMemoryAdd = dest_address;
	DMA2D_InitStruct.DMA2D_OutputGreen = 0;
	DMA2D_InitStruct.DMA2D_OutputBlue = 0;
	DMA2D_InitStruct.DMA2D_OutputRed = 0;
	DMA2D_InitStruct.DMA2D_OutputAlpha = 0;
	DMA2D_InitStruct.DMA2D_OutputOffset = GFX_MAX_WIDTH - coord->source_w;
	DMA2D_InitStruct.DMA2D_NumberOfLine = coord->source_h;
	DMA2D_InitStruct.DMA2D_PixelPerLine = coord->source_w;
	DMA2D_Init (&DMA2D_InitStruct);

	DMA2D_FG_StructInit (&DMA2D_FG_InitStruct);
	DMA2D_FG_InitStruct.DMA2D_FGMA = source_address;
	DMA2D_FG_InitStruct.DMA2D_FGO = picture_width - coord->source_w;
	DMA2D_FG_InitStruct.DMA2D_FGCM = CM_RGB565;
	DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
	DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_VALUE = 0;
	DMA2D_FGConfig (&DMA2D_FG_InitStruct);

	DMA2D_StartTransfer ();

	while (DMA2D_GetFlagStatus (DMA2D_FLAG_TC) == RESET);
}

/*
 * Draw an ARGB888 image to the frame buffer
 *
 * @param[in]	img		Image to draw
 * @param[in]	coord	Coordinates
 */
static void gfx_draw_img_argb8888 (gfx_image_t* img, gfx_coord_t* coord)
{
	DMA2D_InitTypeDef DMA2D_InitStruct;
	DMA2D_FG_InitTypeDef DMA2D_FG_InitStruct;
	DMA2D_BG_InitTypeDef DMA2D_BG_InitStruct;

	uint32_t dest_address;
	uint32_t source_address;
	uint32_t offset;
	uint16_t picture_width;
	uint16_t picture_height;

	picture_width = img->width;
	picture_height = img->height;

	// check for dimensions
	if (coord->source_w == 0) return;
	if (coord->source_h == 0) return;
	if (coord->source_x + coord->source_w > picture_width) return;
	if (coord->source_y + coord->source_h > picture_height) return;
	if (coord->dest_x + coord->source_w > GFX_MAX_WIDTH) return;
	if (coord->dest_y + coord->source_h > GFX_MAX_HEIGHT) return;

	// target address in display RAM
	dest_address = lcd_frame_buffer + 2 * (GFX_MAX_WIDTH * coord->dest_y + coord->dest_x);

	// source address in image
	offset = 4 * (picture_width * coord->source_y + coord->source_x);
	source_address = (uint32_t)&img->pixel_data[offset];

	DMA2D_DeInit ();

	DMA2D_StructInit (&DMA2D_InitStruct);
	DMA2D_InitStruct.DMA2D_Mode = DMA2D_M2M_BLEND;
	DMA2D_InitStruct.DMA2D_CMode = DMA2D_RGB565;
	DMA2D_InitStruct.DMA2D_OutputMemoryAdd = dest_address;
	DMA2D_InitStruct.DMA2D_OutputGreen = 0;
	DMA2D_InitStruct.DMA2D_OutputBlue = 0;
	DMA2D_InitStruct.DMA2D_OutputRed = 0;
	DMA2D_InitStruct.DMA2D_OutputAlpha = 0;
	DMA2D_InitStruct.DMA2D_OutputOffset = GFX_MAX_WIDTH - coord->source_w;
	DMA2D_InitStruct.DMA2D_NumberOfLine = coord->source_h;
	DMA2D_InitStruct.DMA2D_PixelPerLine = coord->source_w;
	DMA2D_Init (&DMA2D_InitStruct);

	DMA2D_FG_StructInit (&DMA2D_FG_InitStruct);
	DMA2D_FG_InitStruct.DMA2D_FGMA = source_address;
	DMA2D_FG_InitStruct.DMA2D_FGO = picture_width - coord->source_w;
	DMA2D_FG_InitStruct.DMA2D_FGCM = CM_ARGB8888;
	DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
	DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_VALUE = 0x00;
	DMA2D_FGConfig (&DMA2D_FG_InitStruct);

	DMA2D_BG_StructInit (&DMA2D_BG_InitStruct);
	DMA2D_BG_InitStruct.DMA2D_BGMA = dest_address;
	DMA2D_BG_InitStruct.DMA2D_BGO = GFX_MAX_WIDTH - coord->source_w;
	DMA2D_BG_InitStruct.DMA2D_BGCM = CM_RGB565;
	DMA2D_BG_InitStruct.DMA2D_BGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
	DMA2D_BG_InitStruct.DMA2D_BGPFC_ALPHA_VALUE = 0x00;
	DMA2D_BGConfig (&DMA2D_BG_InitStruct);

	DMA2D_StartTransfer ();

	while (DMA2D_GetFlagStatus (DMA2D_FLAG_TC) == RESET);
}

/*
 * Get font's character dimensions
 *
 * @param[in]	char	Character
 * @param[in]	font	Font
 * @param[out]	width	Character's width
 * @param[out]	height	Character's height
 */
static void gfx_get_char_dimensions (const char chr, const gfx_font_t* font, uint16_t* width, uint16_t* height)
{
	uint16_t char_offset;

	if (font->bitmap_font)
	{
		/* get the font dimensions */
		*width = font->char_width;
		*height = font->char_height;
	}
	else
	{
		/* get individual character dimensions */
		char_offset = font->offsets[gfx_map_char (chr ) - 32];

		*width = font->chars[char_offset + 1];
		*height = font->chars[char_offset + 2] + font->chars[char_offset + 0]; // height + y_offset
	}
}

/*
 * Measure string's dimensions
 *
 * @param[in]	str			String to measure
 * @param[in]	font		Font
 * @param[in]	word_only	Only measure the first word of the string
 * @param[out]	width		String's width
 * @param[out]	height		String's height
 * @return		Number of characters in the measured string
 */
static uint16_t gfx_measure_string (const char* str, const gfx_font_t* font, bool word_only, uint16_t* width, uint16_t* height)
{
	uint16_t chars;
	uint16_t char_width;
	uint16_t char_height;

	*width = 0;
	*height = 0;

	chars = 0;

	while ((str[chars] != '\0') && (str[chars] != '\n'))
	{
		if (word_only && (str[chars] == ' '))
		{
			break;
		}

		gfx_get_char_dimensions (str[chars], font, &char_width, &char_height);

		*width += (char_width + font->char_space);

		if (char_height > *height)
		{
			*height = char_height;
		}

		chars++;
	}

	// remove last distance
	*width -= font->char_space;

	return chars;
}

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

/*
 * Initialize and fill the screen white
 */
void gfx_init (void)
{
	gfx_clear_screen (RGB565_WHITE);
}

/*
 * Add graphics object to collection of graphical objects
 *
 * @param[in]	obj	Graphics object
 * @return		True on success, otherwise false
 */
bool gfx_add_object (gfx_obj_t* obj)
{
	uint8_t i;

	for (i = 0; i < GFX_MAX_OBJECTS; i++)
	{
		if (gfx_objects[i] == NULL)
		{
			gfx_objects[i] = obj;
			return true;
		}
	}

	return false;
}

/*
 * Delete a graphics object
 * This does not free its memory
 *
 * @param[in]	obj	Graphics object
 * True on success, otherwise false
 */
bool gfx_delete_object (gfx_obj_t* obj)
{
	uint8_t i;

	for (i = 0; i < GFX_MAX_OBJECTS; i++)
	{
		if (gfx_objects[i] == obj)
		{
			gfx_objects[i] = NULL;
			return true;
		}
	}

	return false;
}

/*
 * Delete all graphical objects
 * This does not free their memory
 */
void gfx_delete_objects (void)
{
	uint8_t i;

	for (i = 0; i < GFX_MAX_OBJECTS; i++)
	{
		gfx_objects[i] = NULL;
	}
}

/*
 * Draw all graphical objects
 */
void gfx_draw_objects (void)
{
	uint8_t i;

	for (i = 0; i < GFX_MAX_OBJECTS; i++)
	{
		if ((gfx_objects[i] != NULL) && gfx_objects[i]->enabled)
		{
			switch (gfx_objects[i]->obj_type)
			{
				case GFX_OBJ_IMG:
					gfx_draw_img ((gfx_image_t*)gfx_objects[i]->data, &gfx_objects[i]->coords);
					break;

				default:
					break;
			}
		}
	}
}

/*
 * Refresh display
 * This swaps the frame buffers
 */
void gfx_refresh (void)
{
	lcd_refresh ();
}

/*
 * Clear the whole screen with selected color
 *
 * @param[in]	color	Color
 */
void gfx_clear_screen (uint16_t color)
{
	DMA2D_InitTypeDef DMA2D_InitStruct;

	DMA2D_DeInit ();

	DMA2D_StructInit (&DMA2D_InitStruct);
	DMA2D_InitStruct.DMA2D_Mode = DMA2D_R2M;
	DMA2D_InitStruct.DMA2D_CMode = DMA2D_RGB565;
	DMA2D_InitStruct.DMA2D_OutputRed = GFX_RGB565_R(color);
	DMA2D_InitStruct.DMA2D_OutputGreen = GFX_RGB565_G(color);
	DMA2D_InitStruct.DMA2D_OutputBlue = GFX_RGB565_B(color);
	DMA2D_InitStruct.DMA2D_OutputAlpha = 0xFF;
	DMA2D_InitStruct.DMA2D_OutputMemoryAdd = lcd_frame_buffer;
	DMA2D_InitStruct.DMA2D_OutputOffset = 0;
	DMA2D_InitStruct.DMA2D_NumberOfLine = GFX_MAX_HEIGHT;
	DMA2D_InitStruct.DMA2D_PixelPerLine = GFX_MAX_WIDTH;
	DMA2D_Init (&DMA2D_InitStruct);

	DMA2D_StartTransfer ();

	while (DMA2D_GetFlagStatus (DMA2D_FLAG_TC) == RESET);
}

/*
 * Draw image to screen
 *
 * @param[in]	img		Image
 * @param[in]	coord	Coordinates
 */
void gfx_draw_img (gfx_image_t* img, gfx_coord_t* coord)
{
	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			gfx_draw_img_rgb565 (img, coord);
			break;

		case GFX_PIXEL_FORMAT_ARGB8888:
			gfx_draw_img_argb8888 (img, coord);
			break;

		default:
			break;
	}
}

/*
 * Load image from file
 *
 * @param[in]	file_name	File path and name
 * @param[out]	img			Image
 * @return		True on success, otherwise false
 */
bool gfx_load_img (const char* file_name, gfx_image_t* img)
{
	FIL file;
	FRESULT fres;
	uint32_t file_size;
	uint8_t* data;
	uint32_t read;
	uint16_t tmp;
	uint32_t i;
	bool jpeg;

	if (f_open (&file, file_name, FA_OPEN_EXISTING | FA_READ) == FR_OK)
	{
		file_size = f_size (&file);

		// read first two bytes to detect file type
		fres = f_readn (&file, (uint8_t*)&tmp, 2, &read);

		if (fres != FR_OK)
		{
			printf ("could not read from file %s\n", file_name);
			f_close (&file);
			return false;
		}

		if (tmp == 0xD8FF)
		{
			// JPEG
			jpeg = true;

			// rewind file pointer to read file from the beginning
			f_lseek (&file, 0);

			img->pixel_format = GFX_PIXEL_FORMAT_RGB565;

			data = (uint8_t*)malloc (file_size);

			fres = f_readn (&file, data, file_size, &read);
		}
		else
		{
			// image
			jpeg = false;

			// first two bytes were already read as width
			img->width = tmp;

			// read height
			fres = f_readn (&file, (uint8_t*)&tmp, 2, &read);
			img->height = tmp;

			// read pixel format
			fres = f_readn (&file, (uint8_t*)&tmp, 2, &read);
			img->pixel_format = tmp;

			// dummy read next two bytes
			fres = f_readn (&file, (uint8_t*)&tmp, 2, &read);

			data = (uint8_t*)malloc (file_size - 8);

			fres = f_readn (&file, data, file_size - 8, &read);
		}

		f_close (&file);
	}
	else
	{
		printf ("could not open file %s\n", file_name);
		return false;
	}

	if (jpeg)
	{
		// decode JPEG
		uint8_t* decoded;

		// find width and height
		for (i = 0; i < file_size; i++)
		{
			if ((data[i] == 0xFF) && (data[i + 1] == 0xC0))
			{
				img->width = (data[i + 7] << 8) | (data[i + 8]);
				img->height = (data[i + 5] << 8) | (data[i + 6]);

				break;
			}
		}

		decoded = (uint8_t*)malloc (img->width * img->height * 2);
		jpeg_decode (data, decoded, 0, 0);

		free (data);

		img->pixel_data = decoded;
	}
	else
	{
		img->pixel_data = data;
	}

	return true;
}

/*
 * Clean up memory used by an image
 *
 * @param[in]	img	Image
 */
void gfx_unload_img (gfx_image_t* img)
{
	free (img->pixel_data);
	img->pixel_data = NULL;
}

/*
 * Draw a single character to an image
 *
 * @param[in]	chr		Character to draw
 * @param[in]	img		Image to draw onto
 * @param[in]	x0		X
 * @param[in]	y0		Y
 * @param[in]	font	Font
 * @param[in]	color	Color
 * @return		Character's width
 */
uint8_t gfx_draw_character (char chr, gfx_image_t* img, uint16_t x0, uint16_t y0, const gfx_font_t* font, uint32_t color)
{
	uint16_t x, y;
	uint8_t cols;
	uint8_t rows;
	uint8_t mask;
	uint8_t pixel_row;
	uint16_t char_offset;
	uint8_t y_offset;
	bool first_nibble = true;
	uint16_t index;

	chr = gfx_map_char (chr);

	if (chr < 0x20)
	{
		return 0;
	}

	if (font->bitmap_font)
	{
		/* get the font dimensions */
		cols = font->char_width;
		rows = font->char_height;

		for (y = 0; y < rows; y++)
		{
			/* get pixel row */
			pixel_row = font->chars[rows * (chr - 32) + y];
			mask = 0x80;

			for (x = 0; x < cols; x++)
			{
				if ((pixel_row & mask) != 0)
				{
					/* character pixel */
					gfx_draw_pixel (img, x0 + y * img->width + x, y0, color);
				}

				mask = mask >> 1;
			}
		}
	}
	else
	{
		index = 3;
		char_offset = font->offsets[chr - 32];

		y_offset = font->chars[char_offset + 0];
		cols = font->chars[char_offset + 1];
		rows = font->chars[char_offset + 2];

		for (x = 0; x < cols; x++)
		{
			for (y = 0; y < rows; y++)
			{
				// extract 4 bit defining the transparency (0xF: transparent, 0x0: opaque)
				if (first_nibble)
				{
					mask = (font->chars[char_offset + index] & 0xF0) << 0;
				}
				else
				{
					mask = (font->chars[char_offset + index] & 0x0F) << 4;
					index++;
				}

				if (mask != 0xF0)
				{
					if (mask != 0x00)
					{
						// semi-transparent color
						gfx_blend_pixel (img, x0 + y * img->width + x, y0 + y_offset, color, (uint8_t)~mask);
					}
					else
					{
						// solid color
						gfx_draw_pixel (img, x0 + y * img->width + x, y0 + y_offset, color);
					}
				}

				first_nibble = !first_nibble;
			}
		}
	}

	return cols;
}

/*
 * Draw character centered
 *
 * @param[in]	chr		Character to draw
 * @param[in]	img		Image to draw onto
 * @param[in]	font	Font
 * @param[in]	coords	Coordinates
 * @param[in]	color	Color
 */
void gfx_draw_character_centered (const char chr, gfx_image_t* img, const gfx_font_t* font, gfx_coord_t* coords, uint32_t color)
{
	uint16_t char_width;
	uint16_t char_height;

	gfx_get_char_dimensions(chr, font, &char_width, &char_height);

	gfx_draw_character (chr, img, coords->dest_x + ((coords->source_w - char_width) / 2), coords->dest_y, font, color);
}

/*
 * Draw string and break line after character
 *
 * @param[in]	str			String to draw
 * @param[in]	font		Font
 * @param[in]	img			Image to draw onto
 * @param[in]	x			X
 * @param[in]	y			Y
 * @param[in]	max_width	Maximum width of font rectangle. 0 = ignore
 * @param[in]	color		Color
 */
void gfx_draw_string (const char* str, const gfx_font_t* font, gfx_image_t* img, uint16_t x, uint16_t y, uint16_t max_width, uint32_t color)
{
	uint16_t str_index;
	uint16_t char_width;
	uint16_t char_height;
	uint16_t cur_x;
	uint16_t cur_y;

	str_index = 0;
	cur_x = x;
	cur_y = y;

	while (str[str_index] != '\0')
	{
		/* get word dimensions and length */
		gfx_get_char_dimensions (str[str_index], font, &char_width, &char_height);

		if ((cur_x + char_width > img->width) || ((max_width != 0) && (cur_x + char_width > max_width)))
		{
			/* char does not fit in line, start new line */
			cur_x = x;
			cur_y += font->char_height + font->line_space;
		}

		if (str[str_index] == '\n')
		{
			cur_x = x;
			cur_y += font->char_height + font->line_space;
		}
		else if (str[str_index] == '\r')
		{
			/* ignore */
		}
		else
		{
			if (cur_y + char_height > img->height)
			{
				printf ("text too high: %s. max. height = %d, calculated = %d\n", str, img->height, cur_y + char_height);
				return;
			}

			cur_x += gfx_draw_character (str[str_index], img, cur_x, cur_y, font, color) + font->char_space;
		}

		str_index++;
	}
}

/*
 * Draw string and break line after word
 *
 * @param[in]	str			String to draw
 * @param[in]	font		Font
 * @param[in]	img			Image to draw onto
 * @param[in]	x			X
 * @param[in]	y			Y
 * @param[in]	max_width	Maximum width of font rectangle. 0 = ignore
 * @param[in]	color		Color
 */
void gfx_draw_string_wrapped (const char* str, const gfx_font_t* font, gfx_image_t* img, uint16_t x, uint16_t y, uint16_t max_width, uint32_t color)
{
	uint16_t str_index;
	uint16_t word_length;
	uint16_t word_width;
	uint16_t word_height;
	uint16_t i;
	uint16_t cur_x;
	uint16_t cur_y;

	str_index = 0;
	cur_x = x;
	cur_y = y;

	while (str[str_index] != '\0')
	{
		if (str[str_index] == '\n')
		{
			/* new line */
			cur_x = x;
			cur_y += font->char_height + font->line_space;
			str_index++;
			continue;
		}

		/* get word dimensions and length */
		word_length = gfx_measure_string (&str[str_index], font, true, &word_width, &word_height);

		if ((cur_x + word_width > img->width) || ((max_width != 0) && (cur_x + word_width > max_width)))
		{
			/* word does not fit in line, start new line */
			cur_x = x;
			cur_y += font->char_height + font->line_space;
		}

		if (cur_y + word_height > img->height)
		{
			printf ("text too high: %s. max. height = %d, calculated = %d\n", str, img->height, cur_y + word_height);
			return;
		}

		for (i = 0; i < word_length; i++)
		{
			cur_x += gfx_draw_character (str[str_index + i], img, cur_x, cur_y, font, color) + font->char_space;
		}

		str_index += word_length;

		if (str[str_index] == ' ')
		{
			cur_x += gfx_draw_character (' ', img, cur_x, cur_y, font, color) + font->char_space;
			str_index++;
		}
	}
}

/*
 * Draw string vertically centered on image
 *
 * @param[in]	str		String to draw
 * @param[in]	font	Font
 * @param[in]	img		Image to draw onto
 * @param[in]	y		Y
 * @param[in]	color	Color
 */
void gfx_draw_string_centered (const char* str, const gfx_font_t* font, gfx_image_t* img, uint16_t y, uint32_t color)
{
	uint16_t str_width;
	uint16_t str_height;

	gfx_measure_string (str, font, false, &str_width, &str_height);
	gfx_draw_string_wrapped (str, font, img, (img->width - str_width) / 2, y, 0, color);
}

#if 1
/*
 * Rotate image using integer arithmetic
 *
 * @param[in]	src			Source image
 * @param[in]	dst			Destination image
 * @param[in]	radians		Rotation in radians
 * @param[in]	back_color	Background color for areas not covered by source image
 */
void gfx_rotate (gfx_image_t* src, gfx_image_t* dst, float radians, uint32_t back_color)
{
	int16_t x;
	int16_t y;
	int16_t src_x;
	int16_t src_y;
	int16_t src_width;
	int16_t src_height;
	int16_t dst_width;
	int16_t dst_height;
	int16_t src_center_x;
	int16_t src_center_y;
	int16_t dst_center_x;
	int16_t dst_center_y;
	int16_t cosine;
	int16_t sine;

	cosine = (int16_t)(cos (radians) * 1000.0);
	sine = (int16_t)(sin (radians) * 1000.0);

	src_width = src->width;
	src_height = src->height;

	dst_width = dst->width;
	dst_height = dst->height;

	src_center_x = src_width / 2;
	src_center_y = src_height / 2;

	dst_center_x = dst_width / 2;
	dst_center_y = dst_height / 2;

	for (y = 0; y < dst_height; y++)
	{
		for (x = 0; x < dst_width; x++)
		{
			src_x = (int16_t)((((x - dst_center_x) * cosine) + ((y - dst_center_y) * sine)) / 1000) + src_center_x;
			src_y = (int16_t)((((y - dst_center_y) * cosine) - ((x - dst_center_x) * sine)) / 1000) + src_center_y;

			if ((src_x > 0) && (src_x < src_width) && (src_y > 0) && (src_y < src_height))
			{
				((uint32_t*)dst->pixel_data)[y * dst_width + x] = ((uint32_t*)src->pixel_data)[src_y * src_width + src_x];
			}
			else
			{
				((uint32_t*)dst->pixel_data)[y * dst_width + x] = back_color;
			}
		}
	}
}
#else
/*
 * Rotate image using float arithmetic
 *
 * @param[in]	src			Source image
 * @param[in]	dst			Destination image
 * @param[in]	radians		Rotation in radians
 * @param[in]	back_color	Background color for areas not covered by source image
 */
void gfx_rotate (gfx_image_t* src, gfx_image_t* dst, float radians, uint32_t back_color)
{
	int16_t x;
	int16_t y;
	int16_t src_x;
	int16_t src_y;
	int16_t src_width;
	int16_t src_height;
	int16_t dst_width;
	int16_t dst_height;
	int16_t src_center_x;
	int16_t src_center_y;
	int16_t dst_center_x;
	int16_t dst_center_y;
	float cosine;
	float sine;

	cosine = (float)cos (radians);
	sine = (float)sin (radians);

	src_width = src->width;
	src_height = src->height;
	
	dst_width = dst->width;
	dst_height = dst->height;
	
	src_center_x = src_width / 2;
	src_center_y = src_height / 2;
	
	dst_center_x = dst_width / 2;
	dst_center_y = dst_height / 2;

	for (y = 0; y < dst_height; y++)
	{
		for (x = 0; x < dst_width; x++)
		{
			src_x = (int16_t)((x - dst_center_x) * cosine + (y - dst_center_y) * sine) + src_center_x;
			src_y = (int16_t)((y - dst_center_y) * cosine - (x - dst_center_x) * sine) + src_center_y;

			if ((src_x > 0) && (src_x < src_width) && (src_y > 0) && (src_y < src_height))
			{
				((uint32_t*)dst->pixel_data)[y * dst_width + x] = ((uint32_t*)src->pixel_data)[src_y * src_width + src_x];
			}
			else
			{
				((uint32_t*)dst->pixel_data)[y * dst_width + x] = back_color;
			}
		}
	}
}
#endif

/*
 * Flip an image by 90 degrees clockwise.
 * The object's and image's width and height are adjusted as well.
 *
 * @param[in]	obj		Image object to flip
 */
void gfx_flip90 (gfx_obj_t* obj)
{
	uint32_t* tmp;
	uint16_t size_tmp;
	uint16_t x;
	uint16_t y;
	uint32_t size;
	uint16_t src_width;
	uint16_t src_height;
	gfx_image_t* img_src;

	img_src = (gfx_image_t*)obj->data;

	src_width = img_src->width;
	src_height = img_src->height;
	size = src_width * src_height * gfx_bytes_per_pixel (img_src);

	tmp = (uint32_t*)malloc (size);

	for (y = 0; y < src_height; y++)
	{
		for (x = 0; x < src_width; x++)
		{
			tmp[x * src_height + (src_height - y - 1)] = ((uint32_t*)img_src->pixel_data)[y * src_width + x];
		}
	}

	memcpy (img_src->pixel_data, tmp, size);

	free (tmp);

	size_tmp = img_src->width;
	img_src->width = img_src->height;
	img_src->height = size_tmp;

	obj->coords.source_w = img_src->width;
	obj->coords.source_h = img_src->height;
}

/*
 * Copy image from source to destination
 * Destination memory is allocated here
 *
 * @param[in]	source	Source image
 * @param[in]	dest	Destination image
 */
void gfx_copy_image (gfx_image_t* source, gfx_image_t* dest)
{
	uint32_t size;

	size = source->width * source->height * gfx_bytes_per_pixel (source);

	if (dest->pixel_data != NULL)
	{
		free (dest->pixel_data);
	}

	dest->pixel_data = malloc (size);

	memcpy (dest->pixel_data, source->pixel_data, size);
	dest->width = source->width;
	dest->height = source->height;
	dest->pixel_format = source->pixel_format;
}

/*
 * Draw a single pixel to an image
 *
 * @param[in]	img		Image to draw onto
 * @param[in]	x		X
 * @param[in]	y		Y
 * @param[in]	color	Color
 */
void gfx_set_pixel (gfx_image_t* img, uint16_t x, uint16_t y, uint32_t color)
{
	gfx_draw_pixel (img, x, y, color);
}

/*
 * Get the color of a single pixel
 *
 * @param[in]	img		Image
 * @param[in]	x		X
 * @param[in]	y		Y
 * @return		Pixel color
 */
uint32_t gfx_get_pixel (gfx_image_t* img, uint16_t x, uint16_t y)
{
	uint32_t color;

	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			color = ((uint16_t*)img->pixel_data)[y * img->width + x];
			break;

		case GFX_PIXEL_FORMAT_ARGB8888:
			color = ((uint32_t*)img->pixel_data)[y * img->width + x];
			break;

		default:
			fatal_error ("unknown pixel format");
	}

	return color;
}

/*
 * Draw a line
 *
 * @param[in]	img		Image to draw onto
 * @param[in]	x1		X start
 * @param[in]	y1		Y start
 * @param[in]	x2		X end
 * @param[in]	y2		Y end
 * @param[in]	color	Color
 */
void gfx_draw_line (gfx_image_t* img, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
	uint16_t i;
    uint16_t dx,    dy;
    uint16_t dxabs, dyabs;
    uint16_t sdx,   sdy;
    uint16_t x,     y;
    uint16_t px,    py;

    if (y1 > y2)
	{
		/* we assume y2 > y1 and invert if not */
		i = y2;
		y2 = y1;
		y1 = i;
	}

    if (x1 > x2)
    {
    	/* we assume x2 > x1 and we swap them if not */
        i = x2;
        x2 = x1;
        x1 = i;
    }

    /* the horizontal distance of the line */
    dx = x2 - x1;

    if (dx == 0)
    {
    	/* vertical line */
        gfx_fill_rect (img, x1, y1, 1, y2 - y1 + 1, color);
        return;
    }

    /* the vertical distance of the line */
    dy = y2 - y1;

    if (dy == 0)
    {
    	/* horizontal line */
        gfx_fill_rect (img, x1, y1, x2 - x1 + 1, 1, color);
        return;
    }

    dxabs = abs(dx);
    dyabs = abs(dy);
    sdx = sgn(dx);
    sdy = sgn(dy);
    x = dyabs >> 1;
    y = dxabs >> 1;
    px = x1;
    py = y1;

    /* start pixel */
    gfx_draw_pixel (img, x1, y1, color);

    if (dxabs >= dyabs)
    {
    	/* the line is more horizontal than vertical */
        for (i = 0; i < dxabs; i++)
        {
            y += dyabs;

            if (y >= dxabs)
            {
                y -= dxabs;
                py += sdy;
            }

            px += sdx;

            gfx_draw_pixel (img, px, py, color);
        }
    }
    else
    {
    	/* the line is more vertical than horizontal */
        for (i = 0; i < dyabs; i++)
        {
            x += dxabs;

            if (x >= dyabs)
            {
                x -= dyabs;
                px += sdx;
            }

            py += sdy;

            gfx_draw_pixel (img, px, py, color);
        }
    }
}

/*
 * Fill rectangle with color (hardware accelerated)
 *
 * @param[in]	img		Image to draw onto
 * @param[in]	x		X
 * @param[in]	y		Y
 * @param[in]	width	Width
 * @param[in]	height	Height
 * @param[in]	color	Color
 */
void gfx_fill_rect (gfx_image_t* img, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
	DMA2D_InitTypeDef DMA2D_InitStruct;
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
	uint32_t offset;
	uint32_t address;

	if (x + width > img->width) return;
	if (y + height > img->height) return;

	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			r = GFX_RGB565_R(color);
			g = GFX_RGB565_G(color);
			b = GFX_RGB565_B(color);
			a = GFX_OPAQUE;
			break;

		case GFX_PIXEL_FORMAT_ARGB8888:
			r = GFX_ARGB8888_R(color);
			g = GFX_ARGB8888_G(color);
			b = GFX_ARGB8888_B(color);
			a = GFX_ARGB8888_A(color);
			break;

		default:
			fatal_error ("unknown pixel format");
			return;
	}

	offset = gfx_bytes_per_pixel (img) * (img->width * y + x);
	address = (uint32_t)&img->pixel_data[offset];

	DMA2D_DeInit ();

	DMA2D_StructInit (&DMA2D_InitStruct);
	DMA2D_InitStruct.DMA2D_Mode = DMA2D_R2M;
	DMA2D_InitStruct.DMA2D_CMode = gfx_convert_colormode (img->pixel_format);
	DMA2D_InitStruct.DMA2D_OutputRed = r;
	DMA2D_InitStruct.DMA2D_OutputGreen = g;
	DMA2D_InitStruct.DMA2D_OutputBlue = b;
	DMA2D_InitStruct.DMA2D_OutputAlpha = a;
	DMA2D_InitStruct.DMA2D_OutputMemoryAdd = address;
	DMA2D_InitStruct.DMA2D_OutputOffset = img->width - width;
	DMA2D_InitStruct.DMA2D_NumberOfLine = height;
	DMA2D_InitStruct.DMA2D_PixelPerLine = width;
	DMA2D_Init (&DMA2D_InitStruct);

	DMA2D_StartTransfer ();

	while (DMA2D_GetFlagStatus (DMA2D_FLAG_TC) == RESET);
}

/*
 * Draw image onto another image (hardware accelerated)
 *
 * @param[in]	img_src		Source image
 * @param[in]	img_dst		Destination image
 * @param[in]	coord		Coordinates
 */
void gfx_draw_img_on_img (gfx_image_t* img_src, gfx_image_t* img_dst, gfx_coord_t* coord)
{
	DMA2D_InitTypeDef DMA2D_InitStruct;
	DMA2D_FG_InitTypeDef DMA2D_FG_InitStruct;
	DMA2D_BG_InitTypeDef DMA2D_BG_InitStruct;

	uint32_t address_src;
	uint32_t address_dst;
	uint32_t offset;
	uint16_t width_src;
	uint16_t width_dst;

	width_src = img_src->width;
	width_dst = img_dst->width;

	// check for dimensions
	if (coord->source_w == 0) return;
	if (coord->source_h == 0) return;
	if (coord->source_x + coord->source_w > width_src) return;
	if (coord->source_y + coord->source_h > img_src->height) return;
	if (coord->dest_x + coord->source_w > width_dst) return;
	if (coord->dest_y + coord->source_h > img_dst->height) return;

	// address in source image
	offset = gfx_bytes_per_pixel (img_src) * (width_src * coord->source_y + coord->source_x);
	address_src = (uint32_t)&img_src->pixel_data[offset];

	// address in destination image
	offset = gfx_bytes_per_pixel (img_dst) * (width_dst * coord->dest_y + coord->dest_x);
	address_dst = (uint32_t)&img_dst->pixel_data[offset];

	DMA2D_DeInit ();

	// destination
	DMA2D_StructInit (&DMA2D_InitStruct);
	DMA2D_InitStruct.DMA2D_Mode = DMA2D_M2M_BLEND;
	DMA2D_InitStruct.DMA2D_CMode = gfx_convert_colormode (img_dst->pixel_format);
	DMA2D_InitStruct.DMA2D_OutputMemoryAdd = address_dst;
	DMA2D_InitStruct.DMA2D_OutputGreen = 0;
	DMA2D_InitStruct.DMA2D_OutputBlue = 0;
	DMA2D_InitStruct.DMA2D_OutputRed = 0;
	DMA2D_InitStruct.DMA2D_OutputAlpha = 0;
	DMA2D_InitStruct.DMA2D_OutputOffset = width_dst - coord->source_w;
	DMA2D_InitStruct.DMA2D_NumberOfLine = coord->source_h;
	DMA2D_InitStruct.DMA2D_PixelPerLine = coord->source_w;
	DMA2D_Init (&DMA2D_InitStruct);

	// source
	DMA2D_FG_StructInit (&DMA2D_FG_InitStruct);
	DMA2D_FG_InitStruct.DMA2D_FGMA = address_src;
	DMA2D_FG_InitStruct.DMA2D_FGO = width_src - coord->source_w;
	DMA2D_FG_InitStruct.DMA2D_FGCM = gfx_convert_colormode (img_src->pixel_format);
	DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
	DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_VALUE = 0x00;
	DMA2D_FGConfig (&DMA2D_FG_InitStruct);

	// destination
	DMA2D_BG_StructInit (&DMA2D_BG_InitStruct);
	DMA2D_BG_InitStruct.DMA2D_BGMA = address_dst;
	DMA2D_BG_InitStruct.DMA2D_BGO = width_dst - coord->source_w;
	DMA2D_BG_InitStruct.DMA2D_BGCM = gfx_convert_colormode (img_dst->pixel_format);
	DMA2D_BG_InitStruct.DMA2D_BGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
	DMA2D_BG_InitStruct.DMA2D_BGPFC_ALPHA_VALUE = 0x00;
	DMA2D_BGConfig (&DMA2D_BG_InitStruct);

	DMA2D_StartTransfer ();

	while (DMA2D_GetFlagStatus (DMA2D_FLAG_TC) == RESET);
}

/*
 * Fill image with color (hardware accelerated)
 *
 * @param[in]	img		Image
 * @param[in]	color	Color
 */
void gfx_fill_img (gfx_image_t* img, uint32_t color)
{
	gfx_fill_rect (img, 0, 0, img->width, img->height, color);
}

/*
 * Initialize image dimensions, pixel format and allocate its memory
 *
 * @param[in]	img				Image to initialize
 * @param[in]	width			Width
 * @param[in]	height			Height
 * @param[in]	pixel_format	Pixel format
 * @param[in]	color			Color to fill image with
 */
void gfx_init_img (gfx_image_t* img, uint16_t width, uint16_t height, gfx_pixel_format_t pixel_format, uint32_t color)
{
	img->width = width;
	img->height = height;
	img->pixel_format = pixel_format;
	img->pixel_data = malloc (width * height * gfx_bytes_per_pixel (img));

	gfx_fill_img (img, color);
}

/*
 * Initialize object with image
 *
 * @param[in]	obj	Object to initialize
 * @param[in]	img	Image to apply to object
 */
void gfx_init_img_obj (gfx_obj_t* obj, gfx_image_t* img)
{
	obj->coords.dest_x = 0;
	obj->coords.dest_y = 0;
	obj->coords.source_w = img->width;
	obj->coords.source_h = img->height;
	obj->coords.source_x = 0;
	obj->coords.source_y = 0;
	obj->obj_type = GFX_OBJ_IMG;
	obj->data = img;
	obj->enabled = true;
}

/*
 * Initialize coordinates with image's dimensions
 *
 * @param[in]	coord	Coordinates to initialize
 * @param[in]	img		Image to use for dimensions
 */
void gfx_init_img_coord (gfx_coord_t* coord, gfx_image_t* img)
{
	coord->dest_x = 0;
	coord->dest_y = 0;
	coord->source_w = img->width;
	coord->source_h = img->height;
	coord->source_x = 0;
	coord->source_y = 0;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
