/*
 * led.c
 *
 *  Created on: 16.05.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"

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

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void led_init (void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* enable GPIO clock */
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOG, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init (GPIOG, &GPIO_InitStructure);
}

void led_set (led_t led, led_state_t led_state)
{
	uint16_t pin;

	switch (led)
	{
		case LED_GREEN:
			pin = GPIO_Pin_13;
			break;

		case LED_RED:
			pin = GPIO_Pin_14;
			return;

		default:
			return;
	}

	switch (led_state)
	{
		case LED_STATE_ON:
			GPIO_SetBits (GPIOG, pin);
			break;

		case LED_STATE_OFF:
			GPIO_ResetBits (GPIOG, pin);
			break;

		default:
			return;
	}
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
