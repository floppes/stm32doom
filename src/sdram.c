/*
 * sdram.c
 *
 *  Created on: 26.05.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "main.h"
#include "sdram.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

#define SDRAM_MEMORY_WIDTH							FMC_SDMemory_Width_16b
#define SDRAM_CLOCK_PERIOD							FMC_SDClock_Period_2
#define SDRAM_CAS_LATENCY							FMC_CAS_Latency_3
#define SDRAM_READBURST								FMC_Read_Burst_Disable
#define SDRAM_SIZE									0x800000

#define SDRAM_MODEREG_BURST_LENGTH_1				((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2				((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4				((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8				((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL			((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED		((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2					((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3					((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD		((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED	((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE		((uint16_t)0x0200)

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

static bool initialized = false;

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void sdram_init (void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	FMC_SDRAMInitTypeDef FMC_SDRAMInitStructure;
	FMC_SDRAMTimingInitTypeDef FMC_SDRAMTimingInitStructure;
	FMC_SDRAMCommandTypeDef FMC_SDRAMCommandStructure;
	uint32_t i;

	// only initialize once
	if (initialized)
	{
		return;
	}

    /* initialize clocks */
    RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
                            RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    /* GPIOB */
    GPIO_PinAFConfig (GPIOB, GPIO_PinSource5, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOB, GPIO_PinSource6, GPIO_AF_FMC);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_Init (GPIOB, &GPIO_InitStructure);

    /* GPIOC */
    GPIO_PinAFConfig (GPIOC, GPIO_PinSource0, GPIO_AF_FMC);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init (GPIOC, &GPIO_InitStructure);

    /* GPIOD */
    GPIO_PinAFConfig (GPIOD, GPIO_PinSource0, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOD, GPIO_PinSource1, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOD, GPIO_PinSource8, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOD, GPIO_PinSource9, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOD, GPIO_PinSource10, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOD, GPIO_PinSource14, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOD, GPIO_PinSource15, GPIO_AF_FMC);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1  | GPIO_Pin_8 |
                                  GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 |
                                  GPIO_Pin_15;
    GPIO_Init (GPIOD, &GPIO_InitStructure);

    /* GPIOE */
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource0, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource1, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource7, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource8, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource9, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource10, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource11, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource12, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource13, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource14, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOE, GPIO_PinSource15, GPIO_AF_FMC);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_7 |
                                  GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10 |
                                  GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                  GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init (GPIOE, &GPIO_InitStructure);

    /* GPIOF */
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource0, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource1, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource2, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource3, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource4, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource5, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource11, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource12, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource13, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource14, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOF, GPIO_PinSource15, GPIO_AF_FMC);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1 | GPIO_Pin_2 |
                                  GPIO_Pin_3  | GPIO_Pin_4 | GPIO_Pin_5 |
                                  GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                  GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOF, &GPIO_InitStructure);

    /* GPIOG */
    GPIO_PinAFConfig (GPIOG, GPIO_PinSource0, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOG, GPIO_PinSource1, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOG, GPIO_PinSource4, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOG, GPIO_PinSource5, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOG, GPIO_PinSource8, GPIO_AF_FMC);
    GPIO_PinAFConfig (GPIOG, GPIO_PinSource15, GPIO_AF_FMC);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 |
                                  GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_15;
    GPIO_Init (GPIOG, &GPIO_InitStructure);

	/* initialize FMC */
	RCC_AHB3PeriphClockCmd (RCC_AHB3Periph_FMC, ENABLE);

	// set FMC to 168 MHz / 2 = 84 MHz
	// 84 MHz = 11,9 ns
	// Alle Timings laut Datasheet und Speedgrade -7 (=7ns)
	FMC_SDRAMTimingInitStructure.FMC_LoadToActiveDelay = 2;    // tMRD=2CLK
	FMC_SDRAMTimingInitStructure.FMC_ExitSelfRefreshDelay = 7; // tXSR min=70ns
	FMC_SDRAMTimingInitStructure.FMC_SelfRefreshTime = 4;      // tRAS min=42ns
	FMC_SDRAMTimingInitStructure.FMC_RowCycleDelay = 7;        // tRC  min=63ns
	FMC_SDRAMTimingInitStructure.FMC_WriteRecoveryTime = 2;    // tWR =2CLK
	FMC_SDRAMTimingInitStructure.FMC_RPDelay = 2;              // tRP  min=15ns
	FMC_SDRAMTimingInitStructure.FMC_RCDDelay = 2;             // tRCD min=15ns

	FMC_SDRAMInitStructure.FMC_Bank = FMC_Bank2_SDRAM;
	FMC_SDRAMInitStructure.FMC_ColumnBitsNumber = FMC_ColumnBits_Number_8b;
	FMC_SDRAMInitStructure.FMC_RowBitsNumber = FMC_RowBits_Number_12b;
	FMC_SDRAMInitStructure.FMC_SDMemoryDataWidth = SDRAM_MEMORY_WIDTH;
	FMC_SDRAMInitStructure.FMC_InternalBankNumber = FMC_InternalBank_Number_4;
	FMC_SDRAMInitStructure.FMC_CASLatency = SDRAM_CAS_LATENCY;
	FMC_SDRAMInitStructure.FMC_WriteProtection = FMC_Write_Protection_Disable;
	FMC_SDRAMInitStructure.FMC_SDClockPeriod = SDRAM_CLOCK_PERIOD;
	FMC_SDRAMInitStructure.FMC_ReadBurst = SDRAM_READBURST;
	FMC_SDRAMInitStructure.FMC_ReadPipeDelay = FMC_ReadPipe_Delay_1;
	FMC_SDRAMInitStructure.FMC_SDRAMTimingStruct = &FMC_SDRAMTimingInitStructure;

	FMC_SDRAMInit (&FMC_SDRAMInitStructure);

	/* initialize sequence */
	FMC_SDRAMCommandStructure.FMC_CommandMode = FMC_Command_Mode_CLK_Enabled;
	FMC_SDRAMCommandStructure.FMC_CommandTarget = FMC_Command_Target_bank2;
	FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber = 1;
	FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

	while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET);

	FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

	sleep_ms (5);

	FMC_SDRAMCommandStructure.FMC_CommandMode = FMC_Command_Mode_PALL;
	FMC_SDRAMCommandStructure.FMC_CommandTarget = FMC_Command_Target_bank2;
	FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber = 1;
	FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

	while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET);

	FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

	FMC_SDRAMCommandStructure.FMC_CommandMode = FMC_Command_Mode_AutoRefresh;
	FMC_SDRAMCommandStructure.FMC_CommandTarget = FMC_Command_Target_bank2;
	FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber = 4;
	FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

	while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET);

	FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

	while (FMC_GetFlagStatus(FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET);

	FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

	FMC_SDRAMCommandStructure.FMC_CommandMode = FMC_Command_Mode_LoadMode;
	FMC_SDRAMCommandStructure.FMC_CommandTarget = FMC_Command_Target_bank2;
	FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber = 1;
	FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = SDRAM_MODEREG_BURST_LENGTH_2          |
														   SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
														   SDRAM_MODEREG_CAS_LATENCY_3           |
														   SDRAM_MODEREG_OPERATING_MODE_STANDARD |
														   SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET);

	FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

	/* FMC_CLK = PCLK / 2 = 168 MHz / 2 = 84 MHz
	 * Refresh_Rate = 7.5us
	 * Counter = (FMC_CLK * Refresh_Rate) - 20
	 */
	//FMC_SetRefreshCount (610); // 7.5 us
	FMC_SetRefreshCount (683); // 7.81 us

	while (FMC_GetFlagStatus(FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET);

	for (i = SDRAM_START_ADR; i < SDRAM_START_ADR + SDRAM_SIZE; i += 4)
	{
		*(uint32_t*)i = 0;
	}
}

void sdram_memtest (uint8_t* data, uint32_t length)
{
	uint32_t i;

	//printf ("memtest start at: 0x%08X, length %ld\n", &data[0], length);

	for (i = 0; i < length; i++)
	{
		data[i] = (uint8_t)(i & 0xFF);
	}

	for (i = 0; i < length; i++)
	{
		if (data[i] != (uint8_t)(i & 0xFF))
		{
			//printf ("memtest error at 0x%08X: expected 0x%02X, read 0x%02X\n", &data[i], i & 0xFF, data[i]);
			return;
		}
	}

	printf ("memtest ok!\n");
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
