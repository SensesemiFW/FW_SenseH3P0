/*	crc_16.c
 *  Created on: 23-Apr-2019
 *
 * The module contains routines which are used to calculate the
 * CCITT CRC values of a string of bytes.
 */

#include <stdbool.h>
#include "API_utility.h"
#include "crc_16.h"

#define	CRC_CCITT_16_POLYNOMIAL		0x1021				//Polynomial :x^16+x^12+x^5+1
#define	CRC_START_VALUE_16			0x0000


static void init_crc_ccitt_16_table( void );

static bool crc_table_ccitt_init = FALSE;
static uint16_t crc_ccitt_table[256] = {0};

/*
 * uint16_t compute_crc_16( const unsigned char *input_str, size_t num_bytes );
 *
 * The function compute_crc_16() implementation of the CCITT
 * algorithm for a one-pass calculation of the CRC for a byte string.
 */

uint16_t compute_crc_16( const unsigned char *input_str, uint16_t num_bytes )
{

	uint16_t crc = 0;
	uint16_t temp = 0;
	uint16_t short_c = 0;
	uint16_t loop_index = 0;
	const unsigned char *ptr;


	if (!crc_table_ccitt_init)
	{
		init_crc_ccitt_16_table();
	}

	crc = CRC_START_VALUE_16;
	ptr = input_str;

	if ( ptr != NULL )
	{
		for (loop_index = 0; loop_index < num_bytes; loop_index++)
		{
			short_c = 0x00FFu & (uint16_t) *ptr;
			temp    = (crc >> 8) ^ short_c;
			crc     = (crc << 8) ^ crc_ccitt_table[temp];

			ptr++;
		}
	}

	return crc;

}  /* crc_16 */

/*
 * static void init_crc_ccitt_16_table( void );
 *
 * For optimal performance, the routine to calculate the CRC-CCITT uses a
 * lookup table with pre-compiled values that can be directly applied in the
 * XOR action. This table is created at the first call of the function by the
 * init_crc_ccitt_16_table() routine.
 */

static void init_crc_ccitt_16_table( void )
{
	uint16_t table_index = 0;
	uint16_t bit_pos = 0;
	uint16_t crc = 0;
	uint16_t c = 0;

	for (table_index = 0; table_index < 256; table_index++)
	{
		crc = 0;
		c   = table_index << 8;

		for (bit_pos = 0; bit_pos < 8; bit_pos++)
		{
			if ((crc ^ c) & 0x8000)
			{
				crc = ( crc << 1 ) ^ CRC_CCITT_16_POLYNOMIAL;
			}
			else
			{
				crc =   crc << 1;
			}

			c = c << 1;
		}

		crc_ccitt_table[table_index] = crc;
	}

	crc_table_ccitt_init = TRUE;

}  /* init_crc16_tab */

