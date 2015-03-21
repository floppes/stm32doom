/*
 * lcd.c
 *
 *  Created on: 07.06.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_ltdc.h"
#include "stm32f4xx_misc.h"
#include "stm32f4xx_rcc.h"
#include "gfx.h"
#include "lcd.h"
#include "main.h"
#include "sdram.h"
#include "spi.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

// CS pin (chip select)
#define LCD_CS_PIN					GPIO_Pin_2
#define LCD_CS_PORT					GPIOC
#define LCD_CS_CLK					RCC_AHB1Periph_GPIOC

// WRX pin (write enable)
#define LCD_WRX_PIN					GPIO_Pin_13
#define LCD_WRX_PORT				GPIOD
#define LCD_WRX_CLK					RCC_AHB1Periph_GPIOD

// commands
#define LCD_CMD_RESET				0x01 // Software reset
#define LCD_CMD_ID					0x04 // Display ID
#define LCD_CMD_SLEEP_OUT			0x11 // Sleep out register
#define LCD_CMD_DISP_INV_OFF		0x20 // Display inversion off
#define LCD_CMD_GAMMA				0x26 // Gamma register
#define LCD_CMD_DISPLAY_OFF			0x28 // Display off register
#define LCD_CMD_DISPLAY_ON			0x29 // Display on register
#define LCD_CMD_COLUMN_ADDR			0x2A // Column address register
#define LCD_CMD_PAGE_ADDR			0x2B // Page address register
#define LCD_CMD_GRAM				0x2C // GRAM register
#define LCD_CMD_MAC					0x36 // Memory Access Control register
#define LCD_CMD_PIXEL_FORMAT		0x3A // Pixel Format register
#define LCD_CMD_WDB					0x51 // Write Brightness Display register
#define LCD_CMD_WCD					0x53 // Write Control Display register
#define LCD_CMD_RGB_INTERFACE		0xB0 // RGB Interface Signal Control
#define LCD_CMD_FRC					0xB1 // Frame Rate Control register
#define LCD_CMD_BPC					0xB5 // Blanking Porch Control register
#define LCD_CMD_DFC					0xB6 // Display Function Control register
#define LCD_CMD_POWER1				0xC0 // Power Control 1 register
#define LCD_CMD_POWER2				0xC1 // Power Control 2 register
#define LCD_CMD_VCOM1				0xC5 // VCOM Control 1 register
#define LCD_CMD_VCOM2				0xC7 // VCOM Control 2 register
#define LCD_CMD_POWERA				0xCB // Power control A register
#define LCD_CMD_POWERB				0xCF // Power control B register
#define LCD_CMD_PGAMMA				0xE0 // Positive Gamma Correction register
#define LCD_CMD_NGAMMA				0xE1 // Negative Gamma Correction register
#define LCD_CMD_DTCA				0xE8 // Driver timing control A
#define LCD_CMD_DTCB				0xEA // Driver timing control B
#define LCD_CMD_POWER_SEQ			0xED // Power on sequence register
#define LCD_CMD_3GAMMA_EN			0xF2 // 3 Gamma enable register
#define LCD_CMD_INTERFACE			0xF6 // Interface control register
#define LCD_CMD_PRC					0xF7 // Pump ratio control register

#define LCD_INIT_DELAY				200
#define LCD_SPI_DELAY				10

/* background layer's address */
#define LCD_FRAME_BUFFER			SDRAM_START_ADR

/* layer size */
#define LCD_FRAME_SIZE				((uint32_t)(LCD_MAX_X * LCD_MAX_Y * 2))

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

/*
 * address of current frame buffer to draw to
 */
uint32_t lcd_frame_buffer;

/*
 * currently selected (invisible) layer to draw to
 */
lcd_layers_t lcd_layer;

/*
 * VSYNC enabled/disabled
 */
bool lcd_vsync;

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

static volatile bool lcd_refreshed;

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

static inline void lcd_delay (uint32_t delay)
{
	uint32_t i;

	for (i = 0; i < delay; i++)
	{
		nop ();
	}
}

/*
 * Send command to controller
 *
 * @param[in]	cmd		Command
 */
static void lcd_cmd (uint8_t cmd)
{
	// WRX low
	GPIO_ResetBits (LCD_WRX_PORT, LCD_WRX_PIN);

	// CS low
	GPIO_ResetBits (LCD_CS_PORT, LCD_CS_PIN);

	spi_send_receive (cmd);

	lcd_delay (LCD_SPI_DELAY);

	// CS high
	GPIO_SetBits (LCD_CS_PORT, LCD_CS_PIN);
}

/*
 * Send data to controller
 *
 * @param[in]	data	Data
 */
static void lcd_data (uint8_t data)
{
	// WRX high
	GPIO_SetBits (LCD_WRX_PORT, LCD_WRX_PIN);

	// CS low
	GPIO_ResetBits (LCD_CS_PORT, LCD_CS_PIN);

	spi_send_receive (data);

	lcd_delay (LCD_SPI_DELAY);

	// CS high
	GPIO_SetBits (LCD_CS_PORT, LCD_CS_PIN);
}

/*
 * initialize I/O
 */
static void lcd_init_io (void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// WRX pin
	RCC_AHB1PeriphClockCmd (LCD_WRX_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Pin = LCD_WRX_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init (LCD_WRX_PORT, &GPIO_InitStructure);

	// CS pin
	RCC_AHB1PeriphClockCmd (LCD_CS_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Pin = LCD_CS_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init (LCD_CS_PORT, &GPIO_InitStructure);

	GPIO_SetBits (LCD_CS_PORT, LCD_CS_PIN);
}

/*
 * initialize IL9341 controller
 */
static void lcd_init_controller (void)
{
	lcd_cmd (LCD_CMD_DISP_INV_OFF);

	lcd_cmd (0xCA);
	lcd_data (0xC3);
	lcd_data (0x08);
	lcd_data (0x50);

	lcd_cmd (LCD_CMD_POWERB);
	lcd_data (0x00);
	lcd_data (0xC1);
	lcd_data (0x30); // ESD protection enabled

	lcd_cmd (LCD_CMD_POWER_SEQ);
	lcd_data (0x64);
	lcd_data (0x03);
	lcd_data (0x12);
	lcd_data (0x81);

	lcd_cmd (LCD_CMD_DTCA);
	lcd_data (0x85);
	lcd_data (0x00);
	lcd_data (0x78);

	lcd_cmd (LCD_CMD_POWERA);
	lcd_data (0x39);
	lcd_data (0x2C);
	lcd_data (0x00);
	lcd_data (0x34);
	lcd_data (0x02);

	lcd_cmd (LCD_CMD_PRC);
	lcd_data (0x20);

	lcd_cmd (LCD_CMD_DTCB);
	lcd_data (0x00);
	lcd_data (0x00);

	lcd_cmd (LCD_CMD_FRC);
	lcd_data (0x00);
	lcd_data (0x1B);

	lcd_cmd (LCD_CMD_DFC);
	lcd_data (0x0A);
	lcd_data (0xA2);

	lcd_cmd (LCD_CMD_POWER1);
	lcd_data (0x10);

	lcd_cmd (LCD_CMD_POWER2);
	lcd_data (0x10);

	lcd_cmd (LCD_CMD_VCOM1);
	lcd_data (0x45);
	lcd_data (0x15);

	lcd_cmd (LCD_CMD_VCOM2);
	lcd_data (0x90);

	lcd_cmd (LCD_CMD_MAC);
#ifdef LCD_UPSIDE_DOWN
	lcd_data (0x08); // upside down
#else
	lcd_data (0xC8);
#endif

	lcd_cmd (LCD_CMD_3GAMMA_EN);
	lcd_data (0x00);

	lcd_cmd (LCD_CMD_RGB_INTERFACE);
	lcd_data (0xC2);

	lcd_cmd (LCD_CMD_DFC);
	lcd_data (0x0A);
	lcd_data (0xA7);
	lcd_data (0x27);
	lcd_data (0x04);

	lcd_cmd (LCD_CMD_COLUMN_ADDR);
	lcd_data (0x00);
	lcd_data (0x00);
	lcd_data (0x00);
	lcd_data (0xEF);

	lcd_cmd (LCD_CMD_PAGE_ADDR);
	lcd_data (0x00);
	lcd_data (0x00);
	lcd_data (0x01);
	lcd_data (0x3F);

	lcd_cmd (LCD_CMD_INTERFACE);
	lcd_data (0x01);
	lcd_data (0x00);
	lcd_data (0x06);

	lcd_cmd (LCD_CMD_GRAM);
	lcd_delay (LCD_INIT_DELAY);

	lcd_cmd (LCD_CMD_GAMMA);
	lcd_data (0x01);

	lcd_cmd (LCD_CMD_PGAMMA);
	lcd_data (0x0F);
	lcd_data (0x29);
	lcd_data (0x24);
	lcd_data (0x0C);
	lcd_data (0x0E);
	lcd_data (0x09);
	lcd_data (0x4E);
	lcd_data (0x78);
	lcd_data (0x3C);
	lcd_data (0x09);
	lcd_data (0x13);
	lcd_data (0x05);
	lcd_data (0x17);
	lcd_data (0x11);
	lcd_data (0x00);

	lcd_cmd (LCD_CMD_NGAMMA);
	lcd_data (0x00);
	lcd_data (0x16);
	lcd_data (0x1B);
	lcd_data (0x04);
	lcd_data (0x11);
	lcd_data (0x07);
	lcd_data (0x31);
	lcd_data (0x33);
	lcd_data (0x42);
	lcd_data (0x05);
	lcd_data (0x0C);
	lcd_data (0x0A);
	lcd_data (0x28);
	lcd_data (0x2F);
	lcd_data (0x0F);

	lcd_cmd (LCD_CMD_SLEEP_OUT);
	lcd_delay (LCD_INIT_DELAY);

	lcd_cmd (LCD_CMD_DISPLAY_ON);

	lcd_cmd (LCD_CMD_GRAM);
}

/*
 * initialize alternate functions
 */
static void lcd_init_af (void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);

	GPIO_PinAFConfig (GPIOA, GPIO_PinSource3, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource4, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource6, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource11, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource12, GPIO_AF_LTDC);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_6 | GPIO_Pin_11 | GPIO_Pin_12;

	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init (GPIOA, &GPIO_InitStruct);

	GPIO_PinAFConfig (GPIOB, GPIO_PinSource0, 0x09);
	GPIO_PinAFConfig (GPIOB, GPIO_PinSource1, 0x09);
	GPIO_PinAFConfig (GPIOB, GPIO_PinSource8, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOB, GPIO_PinSource9, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOB, GPIO_PinSource10, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOB, GPIO_PinSource11, GPIO_AF_LTDC);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;

	GPIO_Init (GPIOB, &GPIO_InitStruct);

	GPIO_PinAFConfig (GPIOC, GPIO_PinSource6, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOC, GPIO_PinSource7, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOC, GPIO_PinSource10, GPIO_AF_LTDC);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_10;

	GPIO_Init (GPIOC, &GPIO_InitStruct);

	GPIO_PinAFConfig (GPIOD, GPIO_PinSource3, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOD, GPIO_PinSource6, GPIO_AF_LTDC);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_6;

	GPIO_Init (GPIOD, &GPIO_InitStruct);

	GPIO_PinAFConfig (GPIOF, GPIO_PinSource10, GPIO_AF_LTDC);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;

	GPIO_Init (GPIOF, &GPIO_InitStruct);

	GPIO_PinAFConfig (GPIOG, GPIO_PinSource6, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOG, GPIO_PinSource7, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOG, GPIO_PinSource10, 0x09);
	GPIO_PinAFConfig (GPIOG, GPIO_PinSource11, GPIO_AF_LTDC);
	GPIO_PinAFConfig (GPIOG, GPIO_PinSource12, 0x09);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_10 |
	GPIO_Pin_11 | GPIO_Pin_12;

	GPIO_Init (GPIOG, &GPIO_InitStruct);
}

/*
 * initialize LTCD peripheral
 */
static void lcd_init_ltcd (void)
{
	LTDC_InitTypeDef LTDC_InitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	// Clock enable
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_LTDC, ENABLE);
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_DMA2D, ENABLE);

	lcd_init_af ();

	// set PLLSAI to 6 MHz
	RCC_PLLSAIConfig (192, 7, 4);
	RCC_LTDCCLKDivConfig (RCC_PLLSAIDivR_Div8);

	RCC_PLLSAICmd (ENABLE);

	while (RCC_GetFlagStatus (RCC_FLAG_PLLSAIRDY) == RESET);

	/* configure timer
	 *
	 * BP = back porch, FP = front porch
	 *
	 * HSync = 10       VSync = 2
	 * HBP   = 20       VBP   = 2
	 * HFP   = 10       VFP   = 4
	 *
	 * portrait mode: W = 240, H = 320
	 */
	LTDC_StructInit (&LTDC_InitStruct);

	LTDC_InitStruct.LTDC_HorizontalSync = 9;       // HSync - 1
	LTDC_InitStruct.LTDC_VerticalSync = 1;         // VSync - 1
	LTDC_InitStruct.LTDC_AccumulatedHBP = 29;      // HSync + HBP - 1
	LTDC_InitStruct.LTDC_AccumulatedVBP = 3;       // VSync + VBP - 1
	LTDC_InitStruct.LTDC_AccumulatedActiveW = 269; // HSync + HBP + W - 1
	LTDC_InitStruct.LTDC_AccumulatedActiveH = 323; // VSync + VBP + H - 1
	LTDC_InitStruct.LTDC_TotalWidth = 279;         // HSync + HBP + W + HFP - 1
	LTDC_InitStruct.LTDC_TotalHeigh = 327;         // VSync + VBP + H + VFP - 1

	LTDC_InitStruct.LTDC_HSPolarity = LTDC_HSPolarity_AL;
	LTDC_InitStruct.LTDC_VSPolarity = LTDC_VSPolarity_AL;
	LTDC_InitStruct.LTDC_DEPolarity = LTDC_DEPolarity_AL;
	LTDC_InitStruct.LTDC_PCPolarity = LTDC_PCPolarity_IPC;

	LTDC_InitStruct.LTDC_BackgroundRedValue = 0;
	LTDC_InitStruct.LTDC_BackgroundGreenValue = 0;
	LTDC_InitStruct.LTDC_BackgroundBlueValue = 0;

	LTDC_Init (&LTDC_InitStruct);

	/* configure interrupts */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = LTDC_IRQn;
	NVIC_Init (&NVIC_InitStructure);
}

/*
 * initialize both layers to full screen
 */
static void lcd_layer_fullscreen (void)
{
	LTDC_Layer_InitTypeDef LTDC_Layer_InitStruct;

	// background layer
	LTDC_Layer_InitStruct.LTDC_HorizontalStart = 30;
	LTDC_Layer_InitStruct.LTDC_HorizontalStop = LCD_MAX_X + 30 - 1;
	LTDC_Layer_InitStruct.LTDC_VerticalStart = 4;
	LTDC_Layer_InitStruct.LTDC_VerticalStop = LCD_MAX_Y + 4 - 1;

	LTDC_Layer_InitStruct.LTDC_PixelFormat = LTDC_Pixelformat_RGB565;
	LTDC_Layer_InitStruct.LTDC_ConstantAlpha = 0xFF; // opaque
	LTDC_Layer_InitStruct.LTDC_DefaultColorBlue = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorGreen = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorRed = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorAlpha = 0;

	LTDC_Layer_InitStruct.LTDC_CFBLineLength = (LCD_MAX_X * 2) + 3;
	LTDC_Layer_InitStruct.LTDC_CFBPitch = LCD_MAX_X * 2;
	LTDC_Layer_InitStruct.LTDC_CFBLineNumber = LCD_MAX_Y;

	LTDC_Layer_InitStruct.LTDC_CFBStartAdress = LCD_FRAME_BUFFER;
	LTDC_Layer_InitStruct.LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_CA;
	LTDC_Layer_InitStruct.LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_CA;
	LTDC_LayerInit (LTDC_Layer1, &LTDC_Layer_InitStruct);

	// foreground layer
	LTDC_Layer_InitStruct.LTDC_CFBStartAdress = LCD_FRAME_BUFFER + LCD_FRAME_SIZE;
	LTDC_LayerInit (LTDC_Layer2, &LTDC_Layer_InitStruct);

	LTDC_ReloadConfig (LTDC_IMReload);

	LTDC_LayerCmd (LTDC_Layer1, ENABLE);
	LTDC_LayerCmd (LTDC_Layer2, ENABLE);

	LTDC_ReloadConfig (LTDC_IMReload);

	LTDC_DitherCmd (DISABLE);

	LTDC_Cmd (ENABLE);
}

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void lcd_init (void)
{
	lcd_init_io ();
	lcd_init_controller ();
	lcd_init_ltcd ();
	lcd_layer_fullscreen ();

	lcd_vsync = true;

	lcd_set_layer (LCD_FOREGROUND);
}

/*
 * Switches layers and displays the old layer
 *
 * The new layer is not displayed and can be drawn to
 */
void lcd_refresh (void)
{
	switch (lcd_layer)
	{
		case LCD_BACKGROUND:
			// switch to foreground
			lcd_set_layer (LCD_FOREGROUND);

			// make foreground transparent to display background
			lcd_set_transparency (LCD_FOREGROUND, GFX_TRANSPARENT);
			break;

		case LCD_FOREGROUND:
			// switch to background
			lcd_set_layer (LCD_BACKGROUND);

			// make foreground opaque to display it
			lcd_set_transparency (LCD_FOREGROUND, GFX_OPAQUE);
			break;

		default:
			break;
	}
}

/*
 * Set the layer to draw to
 *
 * This has no effect on the LCD itself, only to drawing routines
 *
 * @param[in]	layer	layer to change to
 */
void lcd_set_layer (lcd_layers_t layer)
{
	switch (layer)
	{
		case LCD_BACKGROUND:
			lcd_frame_buffer = LCD_FRAME_BUFFER;
			lcd_layer = LCD_BACKGROUND;
			break;

		case LCD_FOREGROUND:
			lcd_frame_buffer = LCD_FRAME_BUFFER + LCD_FRAME_SIZE;
			lcd_layer = LCD_FOREGROUND;
			break;

		default:
			break;
	}
}

/*
 * Set transparency of layer
 *
 * @param[in]	layer			layer to change
 * @param[in]	transparency	0x00 is transparent, 0xFF is opaque
 */
void lcd_set_transparency (lcd_layers_t layer, uint8_t transparency)
{
	switch (layer)
	{
		case LCD_BACKGROUND:
			LTDC_LayerAlpha (LTDC_Layer1, transparency);
			break;

		case LCD_FOREGROUND:
			LTDC_LayerAlpha (LTDC_Layer2, transparency);
			break;

		default:
			break;
	}

	if (lcd_vsync)
	{
		lcd_refreshed = false;

		LTDC_ITConfig (LTDC_IT_RR, ENABLE);

		/* reload shadow register on next vertical blank */
		LTDC_ReloadConfig (LTDC_VBReload);

		/* wait for registers to be reloaded */
		while (!lcd_refreshed);
	}
	else
	{
		/* immediately reload shadow registers */
		LTDC_ReloadConfig (LTDC_IMReload);
	}
}

void LTDC_IRQHandler (void)
{
	LTDC_ClearITPendingBit (LTDC_IT_RR);
	LTDC_ITConfig (LTDC_IT_RR, DISABLE);

	lcd_refreshed = true;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
