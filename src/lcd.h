/*
 * lcd.h
 *
 *  Created on: 07.06.2014
 *      Author: Florian
 */


#ifndef LCD_H_
#define LCD_H_

/*---------------------------------------------------------------------*
 *  additional includes                                                *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  global definitions                                                 *
 *---------------------------------------------------------------------*/

/* rotate display by 180 degrees */
//#define	LCD_UPSIDE_DOWN

#define LCD_MAX_X					240	// LCD width
#define LCD_MAX_Y					320 // LCD height

/*---------------------------------------------------------------------*
 *  type declarations                                                  *
 *---------------------------------------------------------------------*/

typedef enum
{
	LCD_BACKGROUND,
	LCD_FOREGROUND
} lcd_layers_t;

/*---------------------------------------------------------------------*
 *  function prototypes                                                *
 *---------------------------------------------------------------------*/

void lcd_init (void);

void lcd_set_layer (lcd_layers_t layer);

void lcd_refresh (void);

void lcd_set_transparency (lcd_layers_t layer, uint8_t transparency);

/*---------------------------------------------------------------------*
 *  global data                                                        *
 *---------------------------------------------------------------------*/

extern uint32_t lcd_frame_buffer;

extern lcd_layers_t lcd_layer;

extern bool lcd_vsync;

/*---------------------------------------------------------------------*
 *  inline functions and function-like macros                          *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/

#endif /* LCD_H_ */
