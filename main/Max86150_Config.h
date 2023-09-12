#ifndef _MAX86150_CONFIG_H_
#define _MAX86150_CONFIG_H_

#include <stdint.h>

static const uint8_t MAX86150_INTSTAT1 =		0x00;
static const uint8_t MAX86150_INTSTAT2 =		0x01;
static const uint8_t MAX86150_INTENABLE1 =		0x02;
//static const uint8_t MAX86150_INTENABLE2 =		0x03;

static const uint8_t MAX86150_FIFOWRITEPTR = 	0x04;
static const uint8_t MAX86150_FIFOOVERFLOW = 	0x05;
static const uint8_t MAX86150_FIFOREADPTR = 	0x06;
//static const uint8_t MAX86150_FIFODATA 		=		0x07;

static const uint8_t MAX86150_FIFOCONFIG = 		0x08;
static const uint8_t MAX86150_FIFOCONTROL1= 	0x09;
static const uint8_t MAX86150_FIFOCONTROL2 = 	0x0A;

static const uint8_t MAX86150_SYSCONTROL = 	0x0D;
static const uint8_t MAX86150_PPGCONFIG1 = 		0x0E;
static const uint8_t MAX86150_PPGCONFIG2 = 		0x0F;
static const uint8_t MAX86150_LED_PROX_AMP = 	0x10;

static const uint8_t MAX86150_LED1_PULSEAMP = 	0x11;
static const uint8_t MAX86150_LED2_PULSEAMP = 	0x12;
static const uint8_t MAX86150_LED_RANGE 		= 	0x14;
//static const uint8_t MAX86150_LED_PILOT_PA 	= 	0x15;

static const uint8_t MAX86150_ECG_CONFIG1 	= 	0x3C;
//static const uint8_t MAX86150_ECG_CONFIG3 	= 	0x3E;
static const uint8_t MAX86150_PROXINTTHRESH = 	0x10;

static const uint8_t MAX86150_PARTID = 			0xFF;

// MAX86150 Commands
static const uint8_t MAX86150_INT_A_FULL_MASK =		(byte)~0b10000000;
static const uint8_t MAX86150_INT_A_FULL_ENABLE = 	0x80;
static const uint8_t MAX86150_INT_A_FULL_DISABLE = 	0x00;

static const uint8_t MAX86150_INT_DATA_RDY_MASK = (byte)~0b01000000;
static const uint8_t MAX86150_INT_DATA_RDY_ENABLE =	0x40;
static const uint8_t MAX86150_INT_DATA_RDY_DISABLE = 0x00;

static const uint8_t MAX86150_INT_ALC_OVF_MASK = (byte)~0b00100000;
static const uint8_t MAX86150_INT_ALC_OVF_ENABLE = 	0x20;
static const uint8_t MAX86150_INT_ALC_OVF_DISABLE = 0x00;

static const uint8_t MAX86150_INT_PROX_INT_MASK = (byte)~0b00010000;
static const uint8_t MAX86150_INT_PROX_INT_ENABLE = 0x10;
static const uint8_t MAX86150_INT_PROX_INT_DISABLE = 0x00;

static const uint8_t MAX86150_SAMPLEAVG_MASK =	(byte)~0b11100000;
static const uint8_t MAX86150_SAMPLEAVG_1 = 	0x00;
static const uint8_t MAX86150_SAMPLEAVG_2 = 	0x20;
static const uint8_t MAX86150_SAMPLEAVG_4 = 	0x40;
static const uint8_t MAX86150_SAMPLEAVG_8 = 	0x60;
static const uint8_t MAX86150_SAMPLEAVG_16 = 	0x80;
static const uint8_t MAX86150_SAMPLEAVG_32 = 	0xA0;

static const uint8_t MAX86150_ROLLOVER_MASK = 	0xEF;
static const uint8_t MAX86150_ROLLOVER_ENABLE = 0x10;
static const uint8_t MAX86150_ROLLOVER_DISABLE = 0x00;

static const uint8_t MAX86150_A_FULL_MASK = 	0xF0;

static const uint8_t MAX86150_SHUTDOWN_MASK = 	0x7F;
static const uint8_t MAX86150_SHUTDOWN = 		0x80;
static const uint8_t MAX86150_WAKEUP = 			0x00;

static const uint8_t MAX86150_RESET_MASK = 		0xFE;
static const uint8_t MAX86150_RESET = 			0x01;

static const uint8_t MAX86150_MODE_MASK = 		0xF8;
//static const uint8_t MAX86150_MODE_REDONLY = 	0x02;
//static const uint8_t MAX86150_MODE_REDIRONLY = 	0x03;
//static const uint8_t MAX86150_MODE_MULTILED = 	0x07;

//static const uint8_t MAX86150_ADCRANGE_MASK = 	0x9F;
//static const uint8_t MAX86150_ADCRANGE_2048 = 	0x00;
//static const uint8_t MAX86150_ADCRANGE_4096 = 	0x20;
//static const uint8_t MAX86150_ADCRANGE_8192 = 	0x40;
//static const uint8_t MAX86150_ADCRANGE_16384 = 	0x60;

//static const uint8_t MAX86150_SAMPLERATE_MASK = 0xE3;
//static const uint8_t MAX86150_SAMPLERATE_50 = 	0x00;
//static const uint8_t MAX86150_SAMPLERATE_100 = 	0x04;
//static const uint8_t MAX86150_SAMPLERATE_200 = 	0x08;
//static const uint8_t MAX86150_SAMPLERATE_400 = 	0x0C;
//static const uint8_t MAX86150_SAMPLERATE_800 = 	0x10;
//static const uint8_t MAX86150_SAMPLERATE_1000 = 0x14;
//static const uint8_t MAX86150_SAMPLERATE_1600 = 0x18;
//static const uint8_t MAX86150_SAMPLERATE_3200 = 0x1C;

static const uint8_t MAX86150_PULSEWIDTH_MASK = 0xFC;
//static const uint8_t MAX86150_PULSEWIDTH_69 = 	0x00;
//static const uint8_t MAX86150_PULSEWIDTH_118 = 	0x01;
//static const uint8_t MAX86150_PULSEWIDTH_215 = 	0x02;
//static const uint8_t MAX86150_PULSEWIDTH_411 = 	0x03;

static const uint8_t MAX86150_SLOT1_MASK = 		0xF0;
static const uint8_t MAX86150_SLOT2_MASK = 		0x0F;
static const uint8_t MAX86150_SLOT3_MASK = 		0xF0;
static const uint8_t MAX86150_SLOT4_MASK = 		0x0F;

//static const uint8_t SLOT_NONE 				= 			0x00;
static const uint8_t SLOT_RED_LED 		= 			0x01;
static const uint8_t SLOT_IR_LED 			= 			0x02;
//static const uint8_t SLOT_RED_PILOT 	=				0x09;
//static const uint8_t SLOT_IR_PILOT 		= 			0x0A;
static const uint8_t SLOT_ECG					= 			0x0D;

#endif /* _MAX86150_CONFIG_H_ */
