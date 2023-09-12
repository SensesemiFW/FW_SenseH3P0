#ifndef _QUICK_TEST_H_
#define _QUICK_TEST_H_

#include <stdbool.h>
#include <stdint.h>
#include <esp_attr.h>
#include "rtc.h"

typedef enum{
	CAPTURE_ECG_L1=0,
	CAPTURE_ECG_L2,
	CAPTURE_ECG_L1_AND_L2,
	CAPTURE_PPG
}DATA_CAPTURE_TYPE_t;

void Enable_Power_Supply(void);
void Disable_Power_Supply(void);

 bool Run_Quick_Vital(void);
bool Run_Multi_Vital(void);
void Filter_Quicktest1_Data(void);
void Filter_Quicktest2_Data(void);

void Dummy_Capture(uint16_t total_samples);

bool Capture_PPG_ECG_Data(DATA_CAPTURE_TYPE_t captureType, bool enableDummyCapture);
bool Capture_BP_Data(bool enableDummyCapture);

void Store_QuickTest1_Data_To_Flash(void);
void Store_QuickTest2_Data_To_Flash(void);
bool Check_PPG_Data_Quality(void);

void Test_Store_QuickTest1_Data_To_Flash(void); // only for testing

#endif
