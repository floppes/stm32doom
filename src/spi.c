/*
 * spi.c
 *
 *  Created on: 07.06.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_spi.h"
#include "spi.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

// configuration for SPI5
#define SPI				SPI5
#define SPI_CLK			RCC_APB2Periph_SPI5
#define SPI_AF			GPIO_AF_SPI5

#define SCK_PORT		GPIOF
#define SCK_PIN			GPIO_Pin_7
#define SCK_CLK			RCC_AHB1Periph_GPIOF
#define SCK_SOURCE		GPIO_PinSource7

#define MISO_PORT		GPIOF
#define MISO_PIN		GPIO_Pin_8
#define MISO_CLK		RCC_AHB1Periph_GPIOF
#define MISO_SOURCE		GPIO_PinSource8

#define MOSI_PORT		GPIOF
#define MOSI_PIN		GPIO_Pin_9
#define MOSI_CLK		RCC_AHB1Periph_GPIOF
#define MOSI_SOURCE		GPIO_PinSource9

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

void spi_init (void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;

	// enable clock
	RCC_APB2PeriphClockCmd (SPI_CLK, ENABLE);

	// enable pin clocks
	RCC_AHB1PeriphClockCmd (SCK_CLK, ENABLE);
	RCC_AHB1PeriphClockCmd (MISO_CLK, ENABLE);
	RCC_AHB1PeriphClockCmd (MOSI_CLK, ENABLE);

	// set alternate functions
	GPIO_PinAFConfig (SCK_PORT, SCK_SOURCE, SPI_AF);
	GPIO_PinAFConfig (MISO_PORT, MISO_SOURCE, SPI_AF);
	GPIO_PinAFConfig (MOSI_PORT, MOSI_SOURCE, SPI_AF);

	// configure pins
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	// SCK
	GPIO_InitStructure.GPIO_Pin = SCK_PIN;
	GPIO_Init (SCK_PORT, &GPIO_InitStructure);

	// MISO
	GPIO_InitStructure.GPIO_Pin = MISO_PIN;
	GPIO_Init (MISO_PORT, &GPIO_InitStructure);

	// MOSI
	GPIO_InitStructure.GPIO_Pin = MOSI_PIN;
	GPIO_Init (MOSI_PORT, &GPIO_InitStructure);

	SPI_I2S_DeInit (SPI);

	// configure peripheral
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init (SPI, &SPI_InitStructure);

	// enable SPI
	SPI_Cmd (SPI, ENABLE);
}

/*
 * Write one byte to SPI and read one byte
 *
 * @param[in]	data	Byte to write
 * @return				Byte read
 */
uint8_t spi_send_receive (uint8_t data)
{
	uint8_t value;

	// wait for SPI to be ready
	while (SPI_I2S_GetFlagStatus (SPI, SPI_I2S_FLAG_TXE) == RESET);

	SPI_I2S_SendData (SPI, data);

	// wait for byte to be received
	while (SPI_I2S_GetFlagStatus (SPI, SPI_I2S_FLAG_RXNE) == RESET);

	value = SPI_I2S_ReceiveData (SPI);

	return value;
}

void spi_send (uint8_t data)
{
	// wait for SPI to be ready
	while (SPI_I2S_GetFlagStatus (SPI, SPI_I2S_FLAG_TXE) == RESET);

	SPI_I2S_SendData (SPI, data);
}

uint8_t spi_receive (void)
{
	uint8_t value;

	// wait for byte to be received
	while (SPI_I2S_GetFlagStatus (SPI, SPI_I2S_FLAG_RXNE) == RESET);

	value = SPI_I2S_ReceiveData (SPI);

	return value;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
