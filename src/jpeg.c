/*
 * jpeg.c
 *
 *  Created on: 19.05.2014
 *      Author: Florian
 */

/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

#define M_SOF0  0xc0
#define M_DHT   0xc4
#define M_EOI   0xd9
#define M_SOS   0xda
#define M_DQT   0xdb
#define M_DRI   0xdd
#define M_APP0  0xe0

#define W1 2841
#define W2 2676
#define W3 2408
#define W5 1609
#define W6 1108
#define W7 565

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))

#define FUNC_OK 0
#define FUNC_FORMAT_ERROR 3

typedef struct
{
	long CurX;
	long CurY;
	DWORD ImgWidth;
	DWORD ImgHeight;
	short SampRate_Y_H;
	short SampRate_Y_V;
	short SampRate_U_H;
	short SampRate_U_V;
	short SampRate_V_H;
	short SampRate_V_V;
	short H_YtoU;
	short V_YtoU;
	short H_YtoV;
	short V_YtoV;
	short Y_in_MCU;
	short U_in_MCU;
	short V_in_MCU;  // notwendig ??
	unsigned char *lpJpegBuf;
	unsigned char *lp;
	short qt_table[3][64];
	short comp_num;
	BYTE comp_index[3];
	BYTE YDcIndex;
	BYTE YAcIndex;
	BYTE UVDcIndex;
	BYTE UVAcIndex;
	BYTE HufTabIndex;
	short *YQtTable;
	short *UQtTable;
	short *VQtTable;
	short code_pos_table[4][16];
	short code_len_table[4][16];
	unsigned short code_value_table[4][256];
	unsigned short huf_max_value[4][16];
	unsigned short huf_min_value[4][16];
	short BitPos;
	short CurByte;
	short rrun;
	short vvalue;
	short MCUBuffer[10 * 64];
	short QtZzMCUBuffer[10 * 64];
	short BlockBuffer[64];
	short ycoef;
	short ucoef;
	short vcoef;
	BYTE IntervalFlag;
	short interval;
	short Y[4 * 64];
	short U[4 * 64];
	short V[4 * 64];
	DWORD sizei;
	DWORD sizej;
	short restart;
	long iclip[1024];
	long *iclp;
} GRAPHIC_JPG_t;

static GRAPHIC_JPG_t GRAPHIC_JPG;

static int InitTag (void);
static int Decode (uint8_t* buffer);
static void GetYUV (short flag);
static void StoreBuffer (uint8_t* buffer);
static int DecodeMCUBlock (void);
static int HufBlock (BYTE dchufindex, BYTE achufindex);
static int DecodeElement (void);
static void IQtIZzMCUComponent (short flag);
static void IQtIZzBlock (short* s, short* d, short flag);
static void Fast_IDCT (int* block);
static BYTE ReadByte (void);
static void Initialize_Fast_IDCT (void);
static void idctrow (int* blk);
static void idctcol (int* blk);

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

static const int GRAPHIC_JPG_ZZ[8][8] =
{
	{  0,  1,  5,  6, 14, 15, 27, 28 },
	{  2,  4,  7, 13, 16, 26, 29, 42 },
	{  3,  8, 12, 17, 25, 30, 41, 43 },
	{  9, 11, 18, 24, 37, 40, 44, 53 },
	{ 10, 19, 23, 32, 39, 45, 52, 54 },
	{ 20, 22, 33, 38, 46, 51, 55, 60 },
	{ 21, 34, 37, 47, 50, 56, 59, 61 },
	{ 35, 36, 48, 49, 57, 58, 62, 63 }
};

static const BYTE GRAPHIC_JPG_AND[9] = { 0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

static int InitTag (void)
{
	BYTE finish = 0;
	BYTE id;
	short llength;
	short i, j, k;
	short huftab1, huftab2;
	short huftabindex;
	BYTE hf_table_index;
	BYTE qt_table_index;
	BYTE comnum;
	unsigned char *lptemp;
	short colorount;

	GRAPHIC_JPG.lp = GRAPHIC_JPG.lpJpegBuf + 2;

	while (!finish)
	{
		id = *(GRAPHIC_JPG.lp + 1);
		GRAPHIC_JPG.lp += 2;
		switch (id)
		{
		case M_APP0:
			llength = MAKEWORD(*(GRAPHIC_JPG.lp + 1), *GRAPHIC_JPG.lp);
			GRAPHIC_JPG.lp += llength;
			break;
		case M_DQT:
			llength = MAKEWORD(*(GRAPHIC_JPG.lp + 1), *GRAPHIC_JPG.lp);
			qt_table_index = (*(GRAPHIC_JPG.lp + 2)) & 0x0f;
			lptemp = GRAPHIC_JPG.lp + 3;
			if (llength < 80)
			{
				for (i = 0; i < 64; i++)
				{
					GRAPHIC_JPG.qt_table[qt_table_index][i] =
							(short) *(lptemp++);
				}
			}
			else
			{
				for (i = 0; i < 64; i++)
				{
					GRAPHIC_JPG.qt_table[qt_table_index][i] =
							(short) *(lptemp++);
				}
				qt_table_index = (*(lptemp++)) & 0x0f;
				for (i = 0; i < 64; i++)
				{
					GRAPHIC_JPG.qt_table[qt_table_index][i] =
							(short) *(lptemp++);
				}
			}
			GRAPHIC_JPG.lp += llength;
			break;
		case M_SOF0:
			llength = MAKEWORD(*(GRAPHIC_JPG.lp + 1), *GRAPHIC_JPG.lp);
			GRAPHIC_JPG.ImgHeight = MAKEWORD(*(GRAPHIC_JPG.lp + 4),
					*(GRAPHIC_JPG.lp + 3));
			GRAPHIC_JPG.ImgWidth = MAKEWORD(*(GRAPHIC_JPG.lp + 6),
					*(GRAPHIC_JPG.lp + 5));
			GRAPHIC_JPG.comp_num = *(GRAPHIC_JPG.lp + 7);
			if ((GRAPHIC_JPG.comp_num != 1) && (GRAPHIC_JPG.comp_num != 3))
				return FUNC_FORMAT_ERROR;
			if (GRAPHIC_JPG.comp_num == 3)
			{
				GRAPHIC_JPG.comp_index[0] = *(GRAPHIC_JPG.lp + 8);
				GRAPHIC_JPG.SampRate_Y_H = (*(GRAPHIC_JPG.lp + 9)) >> 4;
				GRAPHIC_JPG.SampRate_Y_V = (*(GRAPHIC_JPG.lp + 9)) & 0x0f;
				GRAPHIC_JPG.YQtTable =
						(short *) GRAPHIC_JPG.qt_table[*(GRAPHIC_JPG.lp + 10)];
				GRAPHIC_JPG.comp_index[1] = *(GRAPHIC_JPG.lp + 11);
				GRAPHIC_JPG.SampRate_U_H = (*(GRAPHIC_JPG.lp + 12)) >> 4;
				GRAPHIC_JPG.SampRate_U_V = (*(GRAPHIC_JPG.lp + 12)) & 0x0f;
				GRAPHIC_JPG.UQtTable =
						(short *) GRAPHIC_JPG.qt_table[*(GRAPHIC_JPG.lp + 13)];

				GRAPHIC_JPG.comp_index[2] = *(GRAPHIC_JPG.lp + 14);
				GRAPHIC_JPG.SampRate_V_H = (*(GRAPHIC_JPG.lp + 15)) >> 4;
				GRAPHIC_JPG.SampRate_V_V = (*(GRAPHIC_JPG.lp + 15)) & 0x0f;
				GRAPHIC_JPG.VQtTable =
						(short *) GRAPHIC_JPG.qt_table[*(GRAPHIC_JPG.lp + 16)];
			}
			else
			{
				GRAPHIC_JPG.comp_index[0] = *(GRAPHIC_JPG.lp + 8);
				GRAPHIC_JPG.SampRate_Y_H = (*(GRAPHIC_JPG.lp + 9)) >> 4;
				GRAPHIC_JPG.SampRate_Y_V = (*(GRAPHIC_JPG.lp + 9)) & 0x0f;
				GRAPHIC_JPG.YQtTable =
						(short *) GRAPHIC_JPG.qt_table[*(GRAPHIC_JPG.lp + 10)];
				GRAPHIC_JPG.comp_index[1] = *(GRAPHIC_JPG.lp + 8);
				GRAPHIC_JPG.SampRate_U_H = 1;
				GRAPHIC_JPG.SampRate_U_V = 1;
				GRAPHIC_JPG.UQtTable =
						(short *) GRAPHIC_JPG.qt_table[*(GRAPHIC_JPG.lp + 10)];
				GRAPHIC_JPG.comp_index[2] = *(GRAPHIC_JPG.lp + 8);
				GRAPHIC_JPG.SampRate_V_H = 1;
				GRAPHIC_JPG.SampRate_V_V = 1;
				GRAPHIC_JPG.VQtTable =
						(short *) GRAPHIC_JPG.qt_table[*(GRAPHIC_JPG.lp + 10)];
			}
			GRAPHIC_JPG.lp += llength;
			break;
		case M_DHT:
			llength = MAKEWORD(*(GRAPHIC_JPG.lp + 1), *GRAPHIC_JPG.lp);
			if (llength < 0xd0)
			{
				huftab1 = (short) (*(GRAPHIC_JPG.lp + 2)) >> 4;
				huftab2 = (short) (*(GRAPHIC_JPG.lp + 2)) & 0x0f;
				huftabindex = huftab1 * 2 + huftab2;
				lptemp = GRAPHIC_JPG.lp + 3;
				for (i = 0; i < 16; i++)
				{
					GRAPHIC_JPG.code_len_table[huftabindex][i] =
							(short) (*(lptemp++));
				}
				j = 0;
				for (i = 0; i < 16; i++)
				{
					if (GRAPHIC_JPG.code_len_table[huftabindex][i] != 0)
					{
						k = 0;
						while (k < GRAPHIC_JPG.code_len_table[huftabindex][i])
						{
							GRAPHIC_JPG.code_value_table[huftabindex][k + j] =
									(short) (*(lptemp++));
							k++;
						}
						j += k;
					}
				}
				i = 0;
				while (GRAPHIC_JPG.code_len_table[huftabindex][i] == 0)
					i++;
				for (j = 0; j < i; j++)
				{
					GRAPHIC_JPG.huf_min_value[huftabindex][j] = 0;
					GRAPHIC_JPG.huf_max_value[huftabindex][j] = 0;
				}
				GRAPHIC_JPG.huf_min_value[huftabindex][i] = 0;
				GRAPHIC_JPG.huf_max_value[huftabindex][i] =
						GRAPHIC_JPG.code_len_table[huftabindex][i] - 1;
				for (j = i + 1; j < 16; j++)
				{
					GRAPHIC_JPG.huf_min_value[huftabindex][j] =
							(GRAPHIC_JPG.huf_max_value[huftabindex][j - 1] + 1)
									<< 1;
					GRAPHIC_JPG.huf_max_value[huftabindex][j] =
							GRAPHIC_JPG.huf_min_value[huftabindex][j]
									+ GRAPHIC_JPG.code_len_table[huftabindex][j]
									- 1;
				}
				GRAPHIC_JPG.code_pos_table[huftabindex][0] = 0;
				for (j = 1; j < 16; j++)
				{
					GRAPHIC_JPG.code_pos_table[huftabindex][j] =
							GRAPHIC_JPG.code_len_table[huftabindex][j - 1]
									+ GRAPHIC_JPG.code_pos_table[huftabindex][j
											- 1];
				}
				GRAPHIC_JPG.lp += llength;
			}
			else
			{
				hf_table_index = *(GRAPHIC_JPG.lp + 2);
				GRAPHIC_JPG.lp += 2;
				while (hf_table_index != 0xff)
				{
					huftab1 = (short) hf_table_index >> 4;
					huftab2 = (short) hf_table_index & 0x0f;
					huftabindex = huftab1 * 2 + huftab2;
					lptemp = GRAPHIC_JPG.lp + 1;
					colorount = 0;
					for (i = 0; i < 16; i++)
					{
						GRAPHIC_JPG.code_len_table[huftabindex][i] =
								(short) (*(lptemp++));
						colorount += GRAPHIC_JPG.code_len_table[huftabindex][i];
					}
					colorount += 17;
					j = 0;
					for (i = 0; i < 16; i++)
					{
						if (GRAPHIC_JPG.code_len_table[huftabindex][i] != 0)
						{
							k = 0;
							while (k
									< GRAPHIC_JPG.code_len_table[huftabindex][i])
							{
								GRAPHIC_JPG.code_value_table[huftabindex][k + j] =
										(short) (*(lptemp++));
								k++;
							}
							j += k;
						}
					}
					i = 0;
					while (GRAPHIC_JPG.code_len_table[huftabindex][i] == 0)
						i++;
					for (j = 0; j < i; j++)
					{
						GRAPHIC_JPG.huf_min_value[huftabindex][j] = 0;
						GRAPHIC_JPG.huf_max_value[huftabindex][j] = 0;
					}
					GRAPHIC_JPG.huf_min_value[huftabindex][i] = 0;
					GRAPHIC_JPG.huf_max_value[huftabindex][i] =
							GRAPHIC_JPG.code_len_table[huftabindex][i] - 1;
					for (j = i + 1; j < 16; j++)
					{
						GRAPHIC_JPG.huf_min_value[huftabindex][j] =
								(GRAPHIC_JPG.huf_max_value[huftabindex][j - 1]
										+ 1) << 1;
						GRAPHIC_JPG.huf_max_value[huftabindex][j] =
								GRAPHIC_JPG.huf_min_value[huftabindex][j]
										+ GRAPHIC_JPG.code_len_table[huftabindex][j]
										- 1;
					}
					GRAPHIC_JPG.code_pos_table[huftabindex][0] = 0;
					for (j = 1; j < 16; j++)
					{
						GRAPHIC_JPG.code_pos_table[huftabindex][j] =
								GRAPHIC_JPG.code_len_table[huftabindex][j - 1]
										+ GRAPHIC_JPG.code_pos_table[huftabindex][j
												- 1];
					}
					GRAPHIC_JPG.lp += colorount;
					hf_table_index = *GRAPHIC_JPG.lp;
				}
			}
			break;
		case M_DRI:
			llength = MAKEWORD(*(GRAPHIC_JPG.lp + 1), *GRAPHIC_JPG.lp);
			GRAPHIC_JPG.restart = MAKEWORD(*(GRAPHIC_JPG.lp + 3),
					*(GRAPHIC_JPG.lp + 2));
			GRAPHIC_JPG.lp += llength;
			break;
		case M_SOS:
			llength = MAKEWORD(*(GRAPHIC_JPG.lp + 1), *GRAPHIC_JPG.lp);
			comnum = *(GRAPHIC_JPG.lp + 2);
			if (comnum != GRAPHIC_JPG.comp_num)
				return FUNC_FORMAT_ERROR;
			lptemp = GRAPHIC_JPG.lp + 3;
			for (i = 0; i < GRAPHIC_JPG.comp_num; i++)
			{
				if (*lptemp == GRAPHIC_JPG.comp_index[0])
				{
					GRAPHIC_JPG.YDcIndex = (*(lptemp + 1)) >> 4;
					GRAPHIC_JPG.YAcIndex = ((*(lptemp + 1)) & 0x0f) + 2;
				}
				else
				{
					GRAPHIC_JPG.UVDcIndex = (*(lptemp + 1)) >> 4;
					GRAPHIC_JPG.UVAcIndex = ((*(lptemp + 1)) & 0x0f) + 2;
				}
				lptemp += 2;
			}
			GRAPHIC_JPG.lp += llength;
			finish = 1;
			break;
		case M_EOI:
			return FUNC_FORMAT_ERROR;
			break;
		default:
			if ((id & 0xf0) != 0xd0)
			{
				llength = MAKEWORD(*(GRAPHIC_JPG.lp + 1), *GRAPHIC_JPG.lp);
				GRAPHIC_JPG.lp += llength;
			}
			else
			{
				GRAPHIC_JPG.lp += 2;
			}
			break;
		}
	}
	return FUNC_OK;
}

#if 0
// not needed, memset does the same
static void InitTable (void)
{
	short i, j;

	GRAPHIC_JPG.sizei = 0;
	GRAPHIC_JPG.sizej = 0;
	GRAPHIC_JPG.ImgWidth = 0;
	GRAPHIC_JPG.ImgHeight = 0;
	GRAPHIC_JPG.rrun = 0;
	GRAPHIC_JPG.vvalue = 0;
	GRAPHIC_JPG.BitPos = 0;
	GRAPHIC_JPG.CurByte = 0;
	GRAPHIC_JPG.IntervalFlag = 0;
	GRAPHIC_JPG.restart = 0;
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 64; j++)
		{
			GRAPHIC_JPG.qt_table[i][j] = 0;
		}
	}
	GRAPHIC_JPG.comp_num = 0;
	GRAPHIC_JPG.HufTabIndex = 0;
	for (i = 0; i < 3; i++)
	{
		GRAPHIC_JPG.comp_index[i] = 0;
	}
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 16; j++)
		{
			GRAPHIC_JPG.code_len_table[i][j] = 0;
			GRAPHIC_JPG.code_pos_table[i][j] = 0;
			GRAPHIC_JPG.huf_max_value[i][j] = 0;
			GRAPHIC_JPG.huf_min_value[i][j] = 0;
		}
	}
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 256; j++)
		{
			GRAPHIC_JPG.code_value_table[i][j] = 0;
		}
	}

	for (i = 0; i < 10 * 64; i++)
	{
		GRAPHIC_JPG.MCUBuffer[i] = 0;
		GRAPHIC_JPG.QtZzMCUBuffer[i] = 0;
	}
	for (i = 0; i < 64; i++)
	{
		GRAPHIC_JPG.Y[i] = 0;
		GRAPHIC_JPG.U[i] = 0;
		GRAPHIC_JPG.V[i] = 0;
		GRAPHIC_JPG.BlockBuffer[i] = 0;
	}
	GRAPHIC_JPG.ycoef = 0;
	GRAPHIC_JPG.ucoef = 0;
	GRAPHIC_JPG.vcoef = 0;
}
#endif

static int Decode (uint8_t* buffer)
{
	int funcret;

	GRAPHIC_JPG.Y_in_MCU = GRAPHIC_JPG.SampRate_Y_H * GRAPHIC_JPG.SampRate_Y_V;
	GRAPHIC_JPG.U_in_MCU = GRAPHIC_JPG.SampRate_U_H * GRAPHIC_JPG.SampRate_U_V;
	GRAPHIC_JPG.V_in_MCU = GRAPHIC_JPG.SampRate_V_H * GRAPHIC_JPG.SampRate_V_V;
	GRAPHIC_JPG.H_YtoU = GRAPHIC_JPG.SampRate_Y_H / GRAPHIC_JPG.SampRate_U_H;
	GRAPHIC_JPG.V_YtoU = GRAPHIC_JPG.SampRate_Y_V / GRAPHIC_JPG.SampRate_U_V;
	GRAPHIC_JPG.H_YtoV = GRAPHIC_JPG.SampRate_Y_H / GRAPHIC_JPG.SampRate_V_H;
	GRAPHIC_JPG.V_YtoV = GRAPHIC_JPG.SampRate_Y_V / GRAPHIC_JPG.SampRate_V_V;

	Initialize_Fast_IDCT ();

	while ((funcret = DecodeMCUBlock ()) == FUNC_OK)
	{
		GRAPHIC_JPG.interval++;

		if ((GRAPHIC_JPG.restart) && (GRAPHIC_JPG.interval % GRAPHIC_JPG.restart == 0))
		{
			GRAPHIC_JPG.IntervalFlag = 1;
		}
		else
		{
			GRAPHIC_JPG.IntervalFlag = 0;
		}

		IQtIZzMCUComponent (0);
		IQtIZzMCUComponent (1);
		IQtIZzMCUComponent (2);
		GetYUV (0);
		GetYUV (1);
		GetYUV (2);
		StoreBuffer (buffer);
		GRAPHIC_JPG.sizej += GRAPHIC_JPG.SampRate_Y_H * 8;

		if (GRAPHIC_JPG.sizej >= GRAPHIC_JPG.ImgWidth)
		{
			GRAPHIC_JPG.sizej = 0;
			GRAPHIC_JPG.sizei += GRAPHIC_JPG.SampRate_Y_V * 8;
		}

		if ((GRAPHIC_JPG.sizej == 0) && (GRAPHIC_JPG.sizei >= GRAPHIC_JPG.ImgHeight))
			break;
	}
	return funcret;
}

static void GetYUV (short flag)
{
	short H, VV;
	short i, j, k, h;
	short *buf;
	short *pQtZzMCU;

	switch (flag)
	{
		case 0:
			H = GRAPHIC_JPG.SampRate_Y_H;
			VV = GRAPHIC_JPG.SampRate_Y_V;
			buf = GRAPHIC_JPG.Y;
			pQtZzMCU = GRAPHIC_JPG.QtZzMCUBuffer;
			break;

		case 1:
			H = GRAPHIC_JPG.SampRate_U_H;
			VV = GRAPHIC_JPG.SampRate_U_V;
			buf = GRAPHIC_JPG.U;
			pQtZzMCU = GRAPHIC_JPG.QtZzMCUBuffer + GRAPHIC_JPG.Y_in_MCU * 64;
			break;

		case 2:
			H = GRAPHIC_JPG.SampRate_V_H;
			VV = GRAPHIC_JPG.SampRate_V_V;
			buf = GRAPHIC_JPG.V;
			pQtZzMCU = GRAPHIC_JPG.QtZzMCUBuffer + (GRAPHIC_JPG.Y_in_MCU + GRAPHIC_JPG.U_in_MCU) * 64;
			break;

		default:
			return;
	}

	for (i = 0; i < VV; i++)
	{
		for (j = 0; j < H; j++)
		{
			for (k = 0; k < 8; k++)
			{
				for (h = 0; h < 8; h++)
				{
					buf[(i * 8 + k) * GRAPHIC_JPG.SampRate_Y_H * 8 + j * 8 + h] = *pQtZzMCU++;
				}
			}
		}
	}
}

static void StoreBuffer (uint8_t* buffer)
{
	short i = 0, j = 0;
	unsigned char R, G, B;
	int y, u, v, rr, gg, bb;
	long color;
	uint32_t offset;

	for (i = 0; i < GRAPHIC_JPG.SampRate_Y_V * 8; i++)
	{
		if ((GRAPHIC_JPG.sizei + i) < GRAPHIC_JPG.ImgHeight)
		{
			for (j = 0; j < GRAPHIC_JPG.SampRate_Y_H * 8; j++)
			{
				if ((GRAPHIC_JPG.sizej + j) < GRAPHIC_JPG.ImgWidth)
				{
					y = GRAPHIC_JPG.Y[i * 8 * GRAPHIC_JPG.SampRate_Y_H + j];
					u = GRAPHIC_JPG.U[(i / GRAPHIC_JPG.V_YtoU) * 8
									* GRAPHIC_JPG.SampRate_Y_H
									+ j / GRAPHIC_JPG.H_YtoU];
					v = GRAPHIC_JPG.V[(i / GRAPHIC_JPG.V_YtoV) * 8
									* GRAPHIC_JPG.SampRate_Y_H
									+ j / GRAPHIC_JPG.H_YtoV];
#if 1
					rr = ((y << 8) + 18 * u + 367 * v) >> 8;
					gg = ((y << 8) - 159 * u - 220 * v) >> 8;
					bb = ((y << 8) + 411 * u - 29 * v) >> 8;

					R = (unsigned char) rr;
					G = (unsigned char) gg;
					B = (unsigned char) bb;

					if (rr & 0xffffff00)
					{
						if (rr > 255)
						{
							R = 255;
						}
						else if (rr < 0)
						{
							R = 0;
						}
					}

					if (gg & 0xffffff00)
					{
						if (gg > 255)
						{
							G = 255;
						}
						else if (gg < 0)
						{
							G = 0;
						}
					}

					if (bb & 0xffffff00)
					{
						if (bb > 255)
						{
							B = 255;
						}
						else if (bb < 0)
						{
							B = 0;
						}
					}

					color = R >> 3;
					color = color << 6;
					color |= (G >> 2);
					color = color << 5;
					color |= (B >> 3);
#endif
					offset = (GRAPHIC_JPG.sizei + i + GRAPHIC_JPG.CurY) * GRAPHIC_JPG.ImgWidth + GRAPHIC_JPG.sizej + j + GRAPHIC_JPG.CurX;

					buffer[offset * 2] = color & 0xFF;
					buffer[offset * 2 + 1] = color >> 8;

					/*if (LCD_DISPLAY_MODE == PORTRAIT)
					{
						UB_Graphic_DrawPixel (
								GRAPHIC_JPG.sizej + j + GRAPHIC_JPG.CurX,
								GRAPHIC_JPG.sizei + i + GRAPHIC_JPG.CurY,
								color);
					}
					else
					{
						UB_Graphic_DrawPixel (
								GRAPHIC_JPG.ImgHeight - GRAPHIC_JPG.sizei - i
										+ GRAPHIC_JPG.CurX - 1,
								GRAPHIC_JPG.sizej + j + GRAPHIC_JPG.CurY,
								color);
					}*/
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			break;
		}
	}
}

static int DecodeMCUBlock (void)
{
	short *lpMCUBuffer;
	short i, j;
	int funcret;

	if (GRAPHIC_JPG.IntervalFlag == 1)
	{
		GRAPHIC_JPG.lp += 2;
		GRAPHIC_JPG.ycoef = 0;
		GRAPHIC_JPG.ucoef = 0;
		GRAPHIC_JPG.vcoef = 0;
		GRAPHIC_JPG.BitPos = 0;
		GRAPHIC_JPG.CurByte = 0;
	}

	switch (GRAPHIC_JPG.comp_num)
	{
		case 3:
			lpMCUBuffer = GRAPHIC_JPG.MCUBuffer;
			for (i = 0; i < GRAPHIC_JPG.SampRate_Y_H * GRAPHIC_JPG.SampRate_Y_V;
					i++)
			{
				funcret = HufBlock (GRAPHIC_JPG.YDcIndex, GRAPHIC_JPG.YAcIndex);
				if (funcret != FUNC_OK)
					return funcret;
				GRAPHIC_JPG.BlockBuffer[0] = GRAPHIC_JPG.BlockBuffer[0]
						+ GRAPHIC_JPG.ycoef;
				GRAPHIC_JPG.ycoef = GRAPHIC_JPG.BlockBuffer[0];
				for (j = 0; j < 64; j++)
				{
					*lpMCUBuffer++ = GRAPHIC_JPG.BlockBuffer[j];
				}
			}
			for (i = 0; i < GRAPHIC_JPG.SampRate_U_H * GRAPHIC_JPG.SampRate_U_V;
					i++)
			{
				funcret = HufBlock (GRAPHIC_JPG.UVDcIndex, GRAPHIC_JPG.UVAcIndex);
				if (funcret != FUNC_OK)
					return funcret;
				GRAPHIC_JPG.BlockBuffer[0] = GRAPHIC_JPG.BlockBuffer[0]
						+ GRAPHIC_JPG.ucoef;
				GRAPHIC_JPG.ucoef = GRAPHIC_JPG.BlockBuffer[0];
				for (j = 0; j < 64; j++)
				{
					*lpMCUBuffer++ = GRAPHIC_JPG.BlockBuffer[j];
				}
			}
			for (i = 0; i < GRAPHIC_JPG.SampRate_V_H * GRAPHIC_JPG.SampRate_V_V;
					i++)
			{
				funcret = HufBlock (GRAPHIC_JPG.UVDcIndex, GRAPHIC_JPG.UVAcIndex);
				if (funcret != FUNC_OK)
					return funcret;
				GRAPHIC_JPG.BlockBuffer[0] = GRAPHIC_JPG.BlockBuffer[0]
						+ GRAPHIC_JPG.vcoef;
				GRAPHIC_JPG.vcoef = GRAPHIC_JPG.BlockBuffer[0];
				for (j = 0; j < 64; j++)
				{
					*lpMCUBuffer++ = GRAPHIC_JPG.BlockBuffer[j];
				}
			}
			break;

		case 1:
			lpMCUBuffer = GRAPHIC_JPG.MCUBuffer;
			funcret = HufBlock (GRAPHIC_JPG.YDcIndex, GRAPHIC_JPG.YAcIndex);
			if (funcret != FUNC_OK)
				return funcret;
			GRAPHIC_JPG.BlockBuffer[0] = GRAPHIC_JPG.BlockBuffer[0]
					+ GRAPHIC_JPG.ycoef;
			GRAPHIC_JPG.ycoef = GRAPHIC_JPG.BlockBuffer[0];
			for (j = 0; j < 64; j++)
			{
				*lpMCUBuffer++ = GRAPHIC_JPG.BlockBuffer[j];
			}
			for (i = 0; i < 128; i++)
			{
				*lpMCUBuffer++ = 0;
			}
			break;

		default:
			return FUNC_FORMAT_ERROR;
	}

	return FUNC_OK;
}

static int HufBlock (BYTE dchufindex, BYTE achufindex)
{
	short count = 0;
	short i;
	int funcret;

	GRAPHIC_JPG.HufTabIndex = dchufindex;
	funcret = DecodeElement ();

	if (funcret != FUNC_OK)
		return funcret;

	GRAPHIC_JPG.BlockBuffer[count++] = GRAPHIC_JPG.vvalue;
	GRAPHIC_JPG.HufTabIndex = achufindex;

	while (count < 64)
	{
		funcret = DecodeElement ();

		if (funcret != FUNC_OK)
			return funcret;

		if ((GRAPHIC_JPG.rrun == 0) && (GRAPHIC_JPG.vvalue == 0))
		{
			for (i = count; i < 64; i++)
			{
				GRAPHIC_JPG.BlockBuffer[i] = 0;
			}
			count = 64;
		}
		else
		{
			for (i = 0; i < GRAPHIC_JPG.rrun; i++)
			{
				GRAPHIC_JPG.BlockBuffer[count++] = 0;
			}
			GRAPHIC_JPG.BlockBuffer[count++] = GRAPHIC_JPG.vvalue;
		}
	}

	return FUNC_OK;
}

static int DecodeElement (void)
{
	int thiscode, tempcode;
	unsigned short temp, valueex;
	short codelen;
	BYTE hufexbyte, runsize, tempsize, sign;
	BYTE newbyte, lastbyte;

	if (GRAPHIC_JPG.BitPos >= 1)
	{
		GRAPHIC_JPG.BitPos--;
		thiscode = (BYTE) GRAPHIC_JPG.CurByte >> GRAPHIC_JPG.BitPos;
		GRAPHIC_JPG.CurByte = GRAPHIC_JPG.CurByte
				& GRAPHIC_JPG_AND[GRAPHIC_JPG.BitPos];
	}
	else
	{
		lastbyte = ReadByte ();
		GRAPHIC_JPG.BitPos--;
		newbyte = GRAPHIC_JPG.CurByte & GRAPHIC_JPG_AND[GRAPHIC_JPG.BitPos];
		thiscode = lastbyte >> 7;
		GRAPHIC_JPG.CurByte = newbyte;
	}
	codelen = 1;

	while ((thiscode
			< GRAPHIC_JPG.huf_min_value[GRAPHIC_JPG.HufTabIndex][codelen - 1])
			|| (GRAPHIC_JPG.code_len_table[GRAPHIC_JPG.HufTabIndex][codelen - 1]
					== 0)
			|| (thiscode
					> GRAPHIC_JPG.huf_max_value[GRAPHIC_JPG.HufTabIndex][codelen
							- 1]))
	{
		if (GRAPHIC_JPG.BitPos >= 1)
		{
			GRAPHIC_JPG.BitPos--;
			tempcode = (BYTE) GRAPHIC_JPG.CurByte >> GRAPHIC_JPG.BitPos;
			GRAPHIC_JPG.CurByte = GRAPHIC_JPG.CurByte
					& GRAPHIC_JPG_AND[GRAPHIC_JPG.BitPos];
		}
		else
		{
			lastbyte = ReadByte ();
			GRAPHIC_JPG.BitPos--;
			newbyte = GRAPHIC_JPG.CurByte & GRAPHIC_JPG_AND[GRAPHIC_JPG.BitPos];
			tempcode = (BYTE) lastbyte >> 7;
			GRAPHIC_JPG.CurByte = newbyte;
		}
		thiscode = (thiscode << 1) + tempcode;
		codelen++;
		if (codelen > 16)
			return FUNC_FORMAT_ERROR;
	}
	temp = thiscode
			- GRAPHIC_JPG.huf_min_value[GRAPHIC_JPG.HufTabIndex][codelen - 1]
			+ GRAPHIC_JPG.code_pos_table[GRAPHIC_JPG.HufTabIndex][codelen - 1];
	hufexbyte =
			(BYTE) GRAPHIC_JPG.code_value_table[GRAPHIC_JPG.HufTabIndex][temp];
	GRAPHIC_JPG.rrun = (short) (hufexbyte >> 4);
	runsize = hufexbyte & 0x0f;
	if (runsize == 0)
	{
		GRAPHIC_JPG.vvalue = 0;
		return FUNC_OK;
	}
	tempsize = runsize;
	if (GRAPHIC_JPG.BitPos >= runsize)
	{
		GRAPHIC_JPG.BitPos -= runsize;
		valueex = (BYTE) GRAPHIC_JPG.CurByte >> GRAPHIC_JPG.BitPos;
		GRAPHIC_JPG.CurByte = GRAPHIC_JPG.CurByte
				& GRAPHIC_JPG_AND[GRAPHIC_JPG.BitPos];
	}
	else
	{
		valueex = GRAPHIC_JPG.CurByte;
		tempsize -= GRAPHIC_JPG.BitPos;
		while (tempsize > 8)
		{
			lastbyte = ReadByte ();
			valueex = (valueex << 8) + (BYTE) lastbyte;
			tempsize -= 8;
		}
		lastbyte = ReadByte ();
		GRAPHIC_JPG.BitPos -= tempsize;
		valueex = (valueex << tempsize) + (lastbyte >> GRAPHIC_JPG.BitPos);
		GRAPHIC_JPG.CurByte = lastbyte & GRAPHIC_JPG_AND[GRAPHIC_JPG.BitPos];
	}
	sign = valueex >> (runsize - 1);
	if (sign)
	{
		GRAPHIC_JPG.vvalue = valueex;
	}
	else
	{
		valueex = valueex ^ 0xffff;
		temp = 0xffff << runsize;
		GRAPHIC_JPG.vvalue = -(short) (valueex ^ temp);
	}
	return FUNC_OK;
}

static void IQtIZzMCUComponent (short flag)
{
	short H, VV;
	short i, j;
	short *pQtZzMCUBuffer;
	short *pMCUBuffer;

	switch (flag)
	{
		case 0:
			H = GRAPHIC_JPG.SampRate_Y_H;
			VV = GRAPHIC_JPG.SampRate_Y_V;
			pMCUBuffer = GRAPHIC_JPG.MCUBuffer;
			pQtZzMCUBuffer = GRAPHIC_JPG.QtZzMCUBuffer;
			break;

		case 1:
			H = GRAPHIC_JPG.SampRate_U_H;
			VV = GRAPHIC_JPG.SampRate_U_V;
			pMCUBuffer = GRAPHIC_JPG.MCUBuffer + GRAPHIC_JPG.Y_in_MCU * 64;
			pQtZzMCUBuffer = GRAPHIC_JPG.QtZzMCUBuffer + GRAPHIC_JPG.Y_in_MCU * 64;
			break;

		case 2:
			H = GRAPHIC_JPG.SampRate_V_H;
			VV = GRAPHIC_JPG.SampRate_V_V;
			pMCUBuffer = GRAPHIC_JPG.MCUBuffer + (GRAPHIC_JPG.Y_in_MCU + GRAPHIC_JPG.U_in_MCU) * 64;
			pQtZzMCUBuffer = GRAPHIC_JPG.QtZzMCUBuffer + (GRAPHIC_JPG.Y_in_MCU + GRAPHIC_JPG.U_in_MCU) * 64;
			break;

		default:
			return;
	}

	for (i = 0; i < VV; i++)
	{
		for (j = 0; j < H; j++)
		{
			IQtIZzBlock (pMCUBuffer + (i * H + j) * 64,
					pQtZzMCUBuffer + (i * H + j) * 64, flag);
		}
	}
}

static void IQtIZzBlock (short* s, short* d, short flag)
{
	short i, j;
	short tag;
	short *pQt;
	int buffer2[8][8];
	int *buffer1;
	short offset;

	switch (flag)
	{
		case 0:
			pQt = GRAPHIC_JPG.YQtTable;
			offset = 128;
			break;

		case 1:
			pQt = GRAPHIC_JPG.UQtTable;
			offset = 0;
			break;

		case 2:
			pQt = GRAPHIC_JPG.VQtTable;
			offset = 0;
			break;

		default:
			return;
	}

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			tag = GRAPHIC_JPG_ZZ[i][j];
			buffer2[i][j] = (int) s[tag] * (int) pQt[tag];
		}
	}

	buffer1 = (int *) buffer2;
	Fast_IDCT (buffer1);
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			d[i * 8 + j] = buffer2[i][j] + offset;
		}
	}
}

static void Fast_IDCT (int* block)
{
	short i;

	for (i = 0; i < 8; i++)
		idctrow (block + 8 * i);
	for (i = 0; i < 8; i++)
		idctcol (block + i);
}

static BYTE ReadByte (void)
{
	BYTE i;

	i = *(GRAPHIC_JPG.lp++);

	if (i == 0xff)
		GRAPHIC_JPG.lp++;

	GRAPHIC_JPG.BitPos = 8;
	GRAPHIC_JPG.CurByte = i;

	return i;
}

static void Initialize_Fast_IDCT (void)
{
	short i;

	GRAPHIC_JPG.iclp = GRAPHIC_JPG.iclip + 512;

	for (i = -512; i < -256; i++)
	{
		GRAPHIC_JPG.iclp[i] = -256;
	}

	for (i = -256; i < 256; i++)
	{
		GRAPHIC_JPG.iclp[i] = i;
	}

	for (i = 256; i < 512; i++)
	{
		GRAPHIC_JPG.iclp[i] = 512;
	}

}

static void idctrow (int* blk)
{
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;

	if (!((x1 = blk[4] << 11) | (x2 = blk[6]) | (x3 = blk[2]) | (x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3])))
	{
		blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
		return;
	}

	x0 = (blk[0] << 11) + 128;

	x8 = W7 * (x4 + x5);
	x4 = x8 + (W1 - W7) * x4;
	x5 = x8 - (W1 + W7) * x5;
	x8 = W3 * (x6 + x7);
	x6 = x8 - (W3 - W5) * x6;
	x7 = x8 - (W3 + W5) * x7;

	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6 * (x3 + x2);
	x2 = x1 - (W2 + W6) * x2;
	x3 = x1 + (W2 - W6) * x3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;

	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;

	blk[0] = (x7 + x1) >> 8;
	blk[1] = (x3 + x2) >> 8;
	blk[2] = (x0 + x4) >> 8;
	blk[3] = (x8 + x6) >> 8;
	blk[4] = (x8 - x6) >> 8;
	blk[5] = (x0 - x4) >> 8;
	blk[6] = (x3 - x2) >> 8;
	blk[7] = (x7 - x1) >> 8;
}

static void idctcol (int* blk)
{
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;

	if (!((x1 = (blk[8 * 4] << 8)) | (x2 = blk[8 * 6]) | (x3 = blk[8 * 2]) | (x4 = blk[8 * 1]) | (x5 = blk[8 * 7]) | (x6 = blk[8 * 5]) | (x7 = blk[8 * 3])))
	{
		blk[8 * 0] = blk[8 * 1] = blk[8 * 2] = blk[8 * 3] = blk[8 * 4] = blk[8 * 5] = blk[8 * 6] = blk[8 * 7] = GRAPHIC_JPG.iclp[(blk[8 * 0] + 32) >> 6];
		return;
	}

	x0 = (blk[8 * 0] << 8) + 8192;

	x8 = W7 * (x4 + x5) + 4;
	x4 = (x8 + (W1 - W7) * x4) >> 3;
	x5 = (x8 - (W1 + W7) * x5) >> 3;
	x8 = W3 * (x6 + x7) + 4;
	x6 = (x8 - (W3 - W5) * x6) >> 3;
	x7 = (x8 - (W3 + W5) * x7) >> 3;

	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6 * (x3 + x2) + 4;
	x2 = (x1 - (W2 + W6) * x2) >> 3;
	x3 = (x1 + (W2 - W6) * x3) >> 3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;

	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;

	blk[8 * 0] = GRAPHIC_JPG.iclp[(x7 + x1) >> 14];
	blk[8 * 1] = GRAPHIC_JPG.iclp[(x3 + x2) >> 14];
	blk[8 * 2] = GRAPHIC_JPG.iclp[(x0 + x4) >> 14];
	blk[8 * 3] = GRAPHIC_JPG.iclp[(x8 + x6) >> 14];
	blk[8 * 4] = GRAPHIC_JPG.iclp[(x8 - x6) >> 14];
	blk[8 * 5] = GRAPHIC_JPG.iclp[(x0 - x4) >> 14];
	blk[8 * 6] = GRAPHIC_JPG.iclp[(x3 - x2) >> 14];
	blk[8 * 7] = GRAPHIC_JPG.iclp[(x7 - x1) >> 14];
}

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

bool jpeg_decode (uint8_t* jpg, uint8_t* decoded, uint16_t xpos, uint16_t ypos)
{
	memset (&GRAPHIC_JPG, 0x00, sizeof(GRAPHIC_JPG));

	GRAPHIC_JPG.CurX = xpos;
	GRAPHIC_JPG.CurY = ypos;

	GRAPHIC_JPG.lpJpegBuf = (unsigned char*)jpg;

	//InitTable ();

	if (InitTag () != FUNC_OK)
	{
		printf ("InitTag() error\n");
		return false;
	}

	if ((GRAPHIC_JPG.SampRate_Y_H == 0 ) || (GRAPHIC_JPG.SampRate_Y_V == 0))
	{
		printf ("SampRate error\n");
		return false;
	}

	if (Decode (decoded) != FUNC_OK)
	{
		printf ("Decode() error\n");
		return false;
	}

	return true;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
