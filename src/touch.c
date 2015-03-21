/*
 * touch.c
 *
 * Driver for STMPE811 touch sensor
 *
 *  Created on: 09.06.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_misc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_syscfg.h"
#include "i2c.h"
#include "lcd.h"
#include "main.h"
#include "touch.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

#define TOUCH_I2C_ADDR				0x82
#define TOUCH_ID					0x0811

/* functions */
#define TOUCH_FCT_ADC				0x01
#define TOUCH_FCT_TSC				0x02
#define TOUCH_FCT_GPIO				0x04
#define TOUCH_FCT_TS				0x08

/* GPIOs */
#define TOUCH_IO_PIN_0				0x01
#define TOUCH_IO_PIN_1				0x02
#define TOUCH_IO_PIN_2				0x04
#define TOUCH_IO_PIN_3				0x08
#define TOUCH_IO_PIN_4				0x10
#define TOUCH_IO_PIN_5				0x20
#define TOUCH_IO_PIN_6				0x40
#define TOUCH_IO_PIN_7				0x80
#define TOUCH_IO_PIN_ALL			0xFF

/* registers */
#define TOUCH_REG_CHIP_ID1			0x00
#define TOUCH_REG_CHIP_ID2			0x01
#define TOUCH_REG_ID_VER			0x02
#define TOUCH_REG_SYS_CTRL1			0x03
#define TOUCH_REG_SYS_CTRL2			0x04
#define TOUCH_REG_SPI_CFG			0x08
#define TOUCH_REG_INT_CTRL			0x09
#define TOUCH_REG_INT_EN			0x0A
#define TOUCH_REG_INT_STA			0x0B
#define TOUCH_REG_GPIO_INT_EN		0x0C
#define TOUCH_REG_GPIO_INT_STA		0x0D
#define TOUCH_REG_GPIO_SET_PIN		0x10
#define TOUCH_REG_GPIO_CLR_PIN		0x11
#define TOUCH_REG_GPIO_MP_STA		0x12
#define TOUCH_REG_GPIO_DIR			0x13
#define TOUCH_REG_GPIO_ED			0x14
#define TOUCH_REG_GPIO_RE			0x15
#define TOUCH_REG_GPIO_FE			0x16
#define TOUCH_REG_GPIO_AF			0x17
#define TOUCH_REG_ADC_INT_EN		0x0E
#define TOUCH_REG_ADC_INT_STA		0x0F
#define TOUCH_REG_ADC_CTRL1			0x20
#define TOUCH_REG_ADC_CTRL2			0x21
#define TOUCH_REG_ADC_CAPT			0x22
#define TOUCH_REG_ADC_DATA_CH0		0x30
#define TOUCH_REG_ADC_DATA_CH1		0x32
#define TOUCH_REG_ADC_DATA_CH2		0x34
#define TOUCH_REG_ADC_DATA_CH3		0x36
#define TOUCH_REG_ADC_DATA_CH4		0x38
#define TOUCH_REG_ADC_DATA_CH5		0x3A
#define TOUCH_REG_ADC_DATA_CH6		0x3B
#define TOUCH_REG_ADC_DATA_CH7		0x3C
#define TOUCH_REG_TSC_CTRL			0x40
#define TOUCH_REG_TSC_CFG			0x41
#define TOUCH_REG_WDM_TR_X			0x42
#define TOUCH_REG_WDM_TR_Y			0x44
#define TOUCH_REG_WDM_BL_X			0x46
#define TOUCH_REG_WDM_BL_Y			0x48
#define TOUCH_REG_FIFO_TH			0x4A
#define TOUCH_REG_FIFO_STA			0x4B
#define TOUCH_REG_FIFO_SIZE			0x4C
#define TOUCH_REG_TSC_DATA_X		0x4D
#define TOUCH_REG_TSC_DATA_Y		0x4F
#define TOUCH_REG_TSC_DATA_Z		0x51
#define TOUCH_REG_TSC_FRACT_Z		0x56
#define TOUCH_REG_TSC_DATA			0x57
#define TOUCH_REG_TSC_I_DRIVE		0x58
#define TOUCH_REG_TSC_SHIELD		0x59
#define TOUCH_REG_TEMP_CTRL			0x60
#define TOUCH_REG_TEMP_DATA			0x61
#define TOUCH_REG_TEMP_TH			0x63
#define TOUCH_REG_TSC_DATA_XYZ		0xD7

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

touch_state_t touch_state;

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

static bool int_received;

static uint32_t int_timestamp;

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*
 * Enable or disable specific function
 */
static void touch_function (uint8_t function, FunctionalState new_state)
{
	uint8_t value;

	value = i2c_read_byte (TOUCH_I2C_ADDR, TOUCH_REG_SYS_CTRL2);

	if (new_state == ENABLE)
	{
		value &= ~(uint8_t)function;
	}
	else
	{
		value |= (uint8_t)function;
	}

	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_SYS_CTRL2, value);
}

/*
 * Enable or disable GPIO
 */
static void touch_io_af (uint8_t pin, FunctionalState new_state)
{
	uint8_t value;

	value = i2c_read_byte (TOUCH_I2C_ADDR, TOUCH_REG_GPIO_AF);

	if (new_state == ENABLE)
	{
		value |= (uint8_t)pin;
	}
	else
	{
		value &= ~(uint8_t)pin;
	}

	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_GPIO_AF, value);
}

static void touch_read_xy (uint16_t* xy)
{
	uint8_t buf[4];
	int32_t x, y, xr, yr;

	i2c_read_bytes (TOUCH_I2C_ADDR, TOUCH_REG_TSC_DATA_X, buf, 4);
	x = (buf[0] << 8) | buf[1];
	y = (buf[2] << 8) | buf[3];

	// fix x
	if (x <= 3000)
	{
		x = 3870 - x;
	}
	else
	{
		x = 3800 - x;
	}

	// convert x
	xr = x / 15;

	if (xr <= 0)
	{
		xr = 0;
	}
	else if (xr >= 240)
	{
		xr = 239;
	}

	// fix and convert y
	y -= 360;
	yr = y / 11;

	if (yr <= 0)
	{
		yr = 0;
	}
	else if (yr >= 320)
	{
		yr = 319;
	}

	xy[0] = (uint16_t)xr;
	xy[1] = (uint16_t)yr;
}

/*
 * Clear FIFO
 */
static void touch_clear_fifo (void)
{
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_FIFO_STA, 0x01);
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_FIFO_STA, 0x00);
}

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void touch_init (void)
{
	uint8_t id[2];

	// PA15 as external interrupt
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable GPIOA clock */
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);

	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_SYSCFG, ENABLE);

	/* Configure PA15 pin as input floating */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init (GPIOA, &GPIO_InitStructure);

	/* Connect EXTI Line15 to PA15 pin */
	SYSCFG_EXTILineConfig (EXTI_PortSourceGPIOA, EXTI_PinSource15);

	/* Configure EXTI Line15 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line15;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init (&EXTI_InitStructure);

	/* Enable and set EXTI Line15 interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init (&NVIC_InitStructure);

	int_received = false;

	touch_state.status = TOUCH_RELEASED;
	touch_state.x = 0;
	touch_state.y = 0;

	// identify chip
	i2c_read_bytes (TOUCH_I2C_ADDR, TOUCH_REG_CHIP_ID1, id, 2);

	if (((id[0] << 8) | id[1]) != TOUCH_ID)
	{
		fatal_error ("touch sensor not found!");
	}

	// reset
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_SYS_CTRL1, 0x02);
	sleep_ms (5);

	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_SYS_CTRL1, 0x00);

	// enable functions
	touch_function (TOUCH_FCT_ADC, ENABLE);
	touch_function (TOUCH_FCT_TSC, ENABLE);
	touch_function (TOUCH_FCT_TS, ENABLE);

	// enable touch interrupt
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_INT_EN, 0x01);

	// 12 bit ADC, ADC sample time: 80 clocks, internal reference
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_ADC_CTRL1, 0x48);
	sleep_ms (5);

	// set ADC frequency to 3.25 MHz
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_ADC_CTRL2, 0x01);

	// disable all GPIOs
	touch_io_af (TOUCH_IO_PIN_ALL, DISABLE);

	// average control: 4 samples, touch detect delay: 500 us, panel driver settling time: 500 us
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_TSC_CFG, 0x9A);

	// FIFO threshold to trigger interrupt: 0
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_FIFO_TH, 0x00);

	// clear FIFO
	touch_clear_fifo ();

	// pressure measurement accuracy
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_TSC_FRACT_Z, 0x07); //0x01

	// touch screen driver current limit: 50 mA
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_TSC_I_DRIVE, 0x01);

	// touch screen controller: X, Y only, enable TSC
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_TSC_CTRL, 0x03);

	// clear all interrupts
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_INT_STA, 0xFF);

	// enable interrupts
	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_INT_CTRL, 0x01);
}

void touch_main (void)
{
	if (int_received && ((systime - int_timestamp) > 3))
	{
		int_received = false;

		// clear interrupts
		i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_INT_STA, 0xFF);

		// read coordinates
		touch_read ();
	}
}

void touch_read (void)
{
	uint16_t xy[2];
	uint32_t x_diff, y_diff, x, y;
	uint8_t value;

	static uint32_t _x = 0, _y = 0;

	value = i2c_read_byte (TOUCH_I2C_ADDR, TOUCH_REG_TSC_CTRL);

	if ((value & 0x80) == 0)
	{
		touch_state.status = TOUCH_RELEASED;
	}
	else
	{
		touch_state.status = TOUCH_PRESSED;
	}

	if (touch_state.status == TOUCH_PRESSED)
	{
		touch_read_xy (xy);

		x = xy[0];
		y = xy[1];

		x_diff = x > _x ? (x - _x) : (_x - x);
		y_diff = y > _y ? (y - _y) : (_y - y);

		if (x_diff + y_diff > 5)
		{
			_x = x;
			_y = y;
		}
	}

#ifdef LCD_UPSIDE_DOWN
	touch_state.x = LCD_MAX_X - _x;
	touch_state.y = LCD_MAX_Y - _y;
#else
	touch_state.x = _x;
	touch_state.y = _y;
#endif

	touch_clear_fifo ();
}

uint8_t touch_read_temperature (void)
{
	uint16_t temp;

	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_TEMP_CTRL, 0x03);

	sleep_ms (10);

	temp = i2c_read_byte (TOUCH_I2C_ADDR, TOUCH_REG_TEMP_DATA) << 8;
	temp = temp | i2c_read_byte (TOUCH_I2C_ADDR, TOUCH_REG_TEMP_DATA + 1);

	printf ("%d\n", temp);
	return (uint8_t)(((float)temp * 3.0) / 75.1); // is this correct?
}

void EXTI15_10_IRQHandler (void)
{
	if (EXTI_GetITStatus (EXTI_Line15) != RESET)
	{
		EXTI_ClearITPendingBit (EXTI_Line15);

		int_received = true;
		int_timestamp = systime;
	}
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
