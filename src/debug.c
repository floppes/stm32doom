/*
 * debug.c
 *
 *  Created on: 24.11.2013
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "stm32f4xx.h"
#include "debug.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

/* character to invoke debug functions */
uint8_t debug_char;

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void debug_init (void)
{
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* enable GPIO clock */
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);

	/* enable USART clock */
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_USART1, ENABLE);

	/* connect PA9 to USART1_TX */
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource9, GPIO_AF_USART1);

	/* connect PA10 to USART1_RX */
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	/* configure USART TX and RX as alternate function push-pull */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init (GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init (GPIOA, &GPIO_InitStructure);

	/* USART1 configuration */
	USART_OverSampling8Cmd (USART1, ENABLE);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init (USART1, &USART_InitStructure);

	/* NVIC configuration */
	NVIC_PriorityGroupConfig (NVIC_PriorityGroup_2);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init (&NVIC_InitStructure);

	/* enable interrupts */
	USART_ITConfig (USART1, USART_IT_RXNE, ENABLE);

	USART_Cmd (USART1, ENABLE);
}

void debug_chr (char chr)
{
	while (!USART_GetFlagStatus (USART1, USART_FLAG_TXE))
	{
		/* wait for transmit register to become empty */
	}

	USART_SendData (USART1, chr);
}

void USART1_IRQHandler (void)
{
	if (USART_GetITStatus (USART1, USART_IT_RXNE) == SET)
	{
		/* data received */
		debug_char = USART_ReceiveData (USART1);
	}
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
