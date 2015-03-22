/*
 * i2c.c
 *
 *  Created on: 31.05.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "i2c.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

#define I2C_TIMEOUT		0x30000

// configuration for I2C3
#define I2C				I2C3
#define I2C_CLK			RCC_APB1Periph_I2C3
#define I2C_AF			GPIO_AF_I2C3

#define SCL_PORT		GPIOA
#define SCL_PIN			GPIO_Pin_8
#define SCL_CLK			RCC_AHB1Periph_GPIOA
#define SCL_SOURCE		GPIO_PinSource8

#define SDA_PORT		GPIOC
#define SDA_PIN			GPIO_Pin_9
#define SDA_CLK			RCC_AHB1Periph_GPIOC
#define SDA_SOURCE		GPIO_PinSource9

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

static void i2c_init_i2c (void)
{
	I2C_InitTypeDef I2C_InitStructure;

	I2C_DeInit (I2C);

	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 100000;

	I2C_Cmd (I2C, ENABLE);

	I2C_Init (I2C, &I2C_InitStructure);
}

static void i2c_reset (void)
{
	// stop and reset
	I2C_GenerateSTOP (I2C, ENABLE);
	I2C_SoftwareResetCmd (I2C, ENABLE);
	I2C_SoftwareResetCmd (I2C, DISABLE);

	i2c_init ();
	printf ("I2C reset\n");
}

static void i2c_error (const char* func, uint8_t error_code)
{
	printf ("I2C error %d in %s\n", error_code, func);

	i2c_reset ();
}

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void i2c_init (void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// enable I2C clock
	RCC_APB1PeriphClockCmd (I2C_CLK, ENABLE);

	// enable pin clocks
	RCC_AHB1PeriphClockCmd (SCL_CLK, ENABLE);
	RCC_AHB1PeriphClockCmd (SDA_CLK, ENABLE);

	// set pins to alternate function
	GPIO_PinAFConfig (SCL_PORT, SCL_SOURCE, I2C_AF);
	GPIO_PinAFConfig (SDA_PORT, SDA_SOURCE, I2C_AF);

	// configure pins
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	// SCL pin
	GPIO_InitStructure.GPIO_Pin = SCL_PIN;
	GPIO_Init (SCL_PORT, &GPIO_InitStructure);

	// SDA pin
	GPIO_InitStructure.GPIO_Pin = SDA_PIN;
	GPIO_Init (SDA_PORT, &GPIO_InitStructure);

	// reset I2C
	RCC_APB1PeriphResetCmd (I2C_CLK, ENABLE);
	RCC_APB1PeriphResetCmd (I2C_CLK, DISABLE);

	// initialize I2C
	i2c_init_i2c ();
}

uint8_t i2c_read_byte (uint8_t slave_adr, uint8_t adr)
{
	int16_t data;
	uint32_t timeout;

	// start sequence
	I2C_GenerateSTART (I2C, ENABLE);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_SB))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 0);
			return 0x00;
		}
	}

	// ACK disable
	I2C_AcknowledgeConfig (I2C, DISABLE);

	// send slave address with write bit
	I2C_Send7bitAddress (I2C, slave_adr, I2C_Direction_Transmitter);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_ADDR))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 1);
			return 0x00;
		}
	}

	// remove address
	I2C->SR2;

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 2);
			return 0x00;
		}
	}

	// send register address
	I2C_SendData (I2C, adr);

	timeout = I2C_TIMEOUT;

	while ((!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE))
		|| (!I2C_GetFlagStatus (I2C, I2C_FLAG_BTF)))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 3);
			return 0x00;
		}
	}

	// start sequence
	I2C_GenerateSTART (I2C, ENABLE);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_SB))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 4);
			return 0x00;
		}
	}

	// send slave address with read bit
	I2C_Send7bitAddress (I2C, slave_adr, I2C_Direction_Receiver);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_ADDR))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 5);
			return 0x00;
		}
	}

	// remove address
	I2C->SR2;

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_RXNE))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 6);
			return 0x00;
		}
	}

	// stop sequence
	I2C_GenerateSTOP (I2C, ENABLE);

	// read data
	data = I2C_ReceiveData (I2C);

	// ACK enable
	I2C_AcknowledgeConfig (I2C, ENABLE);

	return data;
}

bool i2c_write_byte (uint8_t slave_adr, uint8_t adr, uint8_t data)
{
	uint32_t timeout;

	// start sequence
	I2C_GenerateSTART (I2C, ENABLE);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_SB))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 0);
			return false;
		}
	}

	// send slave address with write bit
	I2C_Send7bitAddress (I2C, slave_adr, I2C_Direction_Transmitter);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_ADDR))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 1);
			return false;
		}
	}

	// remove address
	I2C->SR2;

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 2);
			return false;
		}
	}

	// send register address
	I2C_SendData (I2C, adr);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 3);
			return false;
		}
	}

	// send data
	I2C_SendData (I2C, data);

	timeout = I2C_TIMEOUT;

	while ((!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus (I2C, I2C_FLAG_BTF)))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 4);
			return false;
		}
	}

	// stop sequence
	I2C_GenerateSTOP (I2C, ENABLE);

	return true;
}

bool i2c_read_bytes (uint8_t slave_adr, uint8_t adr, uint8_t* buf, uint8_t cnt)
{
	uint32_t timeout;
	uint8_t i;

	if (cnt == 0)
	{
		i2c_error (__func__, 0);
		return false;
	}

	// start sequence
	I2C_GenerateSTART (I2C, ENABLE);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_SB))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 1);
			return false;
		}
	}

	if (cnt == 1)
	{
		// ACK disable
		I2C_AcknowledgeConfig (I2C, DISABLE);
	}
	else
	{
		// ACK enable
		I2C_AcknowledgeConfig (I2C, ENABLE);
	}

	// send slave address with write bit
	I2C_Send7bitAddress (I2C, slave_adr, I2C_Direction_Transmitter);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_ADDR))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 2);
			return false;
		}
	}

	// remove address
	I2C->SR2;

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 3);
			return false;
		}
	}

	// send register address
	I2C_SendData (I2C, adr);

	timeout = I2C_TIMEOUT;

	while ((!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE))
		|| (!I2C_GetFlagStatus (I2C, I2C_FLAG_BTF)))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 4);
			return false;
		}
	}

	// start sequence
	I2C_GenerateSTART (I2C, ENABLE);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_SB))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 5);
			return false;
		}
	}

	// send slave address with read bit
	I2C_Send7bitAddress (I2C, slave_adr, I2C_Direction_Receiver);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_ADDR))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 6);
			return false;
		}
	}

	// remove address
	I2C->SR2;

	// read data
	for (i = 0; i < cnt; i++)
	{
		if ((i + 1) >= cnt)
		{
			// ACK disable
			I2C_AcknowledgeConfig (I2C, DISABLE);

			// stop sequence
			I2C_GenerateSTOP (I2C, ENABLE);
		}

		timeout = I2C_TIMEOUT;

		while (!I2C_GetFlagStatus (I2C, I2C_FLAG_RXNE))
		{
			if (timeout != 0)
			{
				timeout--;
			}
			else
			{
				i2c_error (__func__, 7);
				return false;
			}
		}

		buf[i] = I2C_ReceiveData (I2C);
	}

	// ACK enable
	I2C_AcknowledgeConfig (I2C, ENABLE);

	return true;
}

bool i2c_write_bytes (uint8_t slave_adr, uint8_t adr, uint8_t* buf, uint8_t cnt)
{
	uint32_t timeout;
	uint8_t i;

	if (cnt == 0)
	{
		i2c_error (__func__, 0);
		return false;
	}

	// start sequence
	I2C_GenerateSTART (I2C, ENABLE);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_SB))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 1);
			return false;
		}
	}

	// send slave address with write bit
	I2C_Send7bitAddress (I2C, slave_adr, I2C_Direction_Transmitter);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_ADDR))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 2);
			return false;
		}
	}

	// remove address
	I2C->SR2;

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__, 3);
			return false;
		}
	}

	// send register address
	I2C_SendData (I2C, adr);

	timeout = I2C_TIMEOUT;

	while (!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE))
	{
		if (timeout != 0)
		{
			timeout--;
		}
		else
		{
			i2c_error (__func__,4);
			return false;
		}
	}

	// send data
	for (i = 0; i < cnt; i++)
	{
		I2C_SendData (I2C, buf[i]);

		timeout = I2C_TIMEOUT;

		while ((!I2C_GetFlagStatus (I2C, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus (I2C, I2C_FLAG_BTF)))
		{
			if (timeout != 0)
			{
				timeout--;
			}
			else
			{
				i2c_error (__func__, 5);
				return false;
			}
		}
	}

	// stop sequence
	I2C_GenerateSTOP (I2C, ENABLE);

	return true;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
