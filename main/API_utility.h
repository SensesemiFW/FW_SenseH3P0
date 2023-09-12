

#ifndef API_API_UTILITY_H_
#define API_API_UTILITY_H_

#include "stdbool.h"
#include "stdlib.h"
#include <stdint.h>
#define DATA_BUFFER3_LENGTH			20000U
#define TOTAL_SAMPLES 1200 //1

/*-----------------------------------MACROS ------------------------------------------------------------------*/
#define TRUE				1
#define FALSE				0
//#define NULL				0

#define PIN_HIGH			1
#define PIN_LOW				0
/*----------------------------------- ENUM ------------------------------------------------------------------*/

typedef enum{
	ENTER =	0,
	NEXT,
	BACK,
	TOTAL_NUM_BTN
}NAVIGATE_BUTTON_TYPE;

typedef enum
{
	PATIENT_REGISTRATION = 0,
	PATIENT_DEREGISTRATION = 1,
	BP_CALIBRATION_FACTORS = 2,
	BP_STD_VALUES = 4,
}PID_REG_SUB_CMD ;
/*----------------------------------- STRUCTURE --------------------------------------------------------*/

typedef struct __attribute__((__packed__))
{
	uint8_t 		sub_command;
	uint8_t 		new_pid[10];
	uint32_t    	sbp_std_val;
	uint32_t    	dbp_std_val;
	float    		sbp_multiplier_val;
	float    		dbp_multiplier_val;
	bool 			valid_bp_test;
}PATIENT_ID_INFO;

extern PATIENT_ID_INFO bp_calibration;
/*-----------------------------------GLOBAL FUNCTIONS --------------------------------------------------------*/

extern uint8_t lead_1_status ;
extern bool Is_Test_In_Progress;

extern uint8_t BT_flash_buffer[DATA_BUFFER3_LENGTH];

extern uint32_t SPO2_PPG_IR_BUFF[TOTAL_SAMPLES];
extern uint32_t SPO2_PPG_RED_BUFF[TOTAL_SAMPLES];
extern float ECG_Lead1_buff[TOTAL_SAMPLES];
extern float ECG_Lead2_buff[TOTAL_SAMPLES];
extern float BP_ECG_Lead1_buff[TOTAL_SAMPLES];
extern uint32_t BP_PPG_RED_BUFF[TOTAL_SAMPLES];
extern uint32_t BP_PPG_IR_BUFF[TOTAL_SAMPLES];
extern float FilterOutputBuffer1[TOTAL_SAMPLES];
extern float FilterOutputBuffer2[TOTAL_SAMPLES];
extern float FilterOutputBuffer3[TOTAL_SAMPLES];
extern float FilterOutputBuffer4[TOTAL_SAMPLES];

char* StrCat(char* destination, const char* source);
void IntergerToString(char str[], uint32_t num);
void FloatToString(float value,char* dst_str);
void MemSet(void *buffer, uint8_t value, uint32_t size);
void MemCpy(void *dest, void *src, uint32_t size);
uint16_t Get_strlen(const char* source);
uint32_t Length_padding_multiple_of_four(uint32_t len);
void Hex_to_Float(uint8_t hex_val[], float* float_val);

#endif /* API_API_UTILITY_H_ */
