
#ifndef API_API_ECG_H_
#define API_API_ECG_H_

#include <stdint.h>
#include <stdbool.h>
#include "esp_attr.h"



/** Configuration taken from Modular based code*/


#define CONFIG          0x00
#define FLEX_CH1_CN     0x01
#define FLEX_CH2_CN     0x02
#define FLEX_CH3_CN     0x03
#define FLEX_PACE_CN    0x04
#define FLEX_VBAT_CN    0x05
#define LOD_CN          0x06
#define LOD_EN          0x07
#define LOD_CURRENT     0x08
#define LOD_AC_CN       0x09
#define CMDET_EN        0x0A
#define CMDET_CN        0x0B
#define RLD_CN          0x0C
#define WILSON_EN1      0x0D
#define WILSON_EN2      0x0E
#define WILSON_EN3      0x0F
#define WILSON_CN       0x10
#define REF_CN          0x11
#define OSC_CN          0x12
#define AFE_RES         0x13
#define AFE_SHDN_CN     0x14
#define AFE_FAULT_CN    0x15
#define AFE_PACE_CN     0x17
#define ERROR_LOD       0x18
#define ERROR_STATUS    0x19
#define ERROR_RANGE1    0x1A
#define ERROR_RANGE2    0x1B
#define ERROR_RANGE3    0x1C
#define ERROR_SYNC      0x1D
#define ERROR_MISC      0x1E
#define DIGO_STRENGTH   0x1F
#define R2_RATE         0x21
#define R3_RATE_CH1     0x22
#define R3_RATE_CH2     0x23
#define R3_RATE_CH3     0x24
#define R1_RATE         0x25
#define DIS_EFILTER     0x26
#define DRDYB_SRC       0x27
#define SYNCB_CN        0x28
#define MASK_DRDYB      0x29
#define MASK_ERR        0x2A
#define ALARM_FILTER    0x2E
#define CN_CNFG         0x2F
#define DATA_STATUS     0x30
#define DATA_CH1_PACE_MSB8  0x31
#define DATA_CH1_PACE_LSB8  0x32
#define DATA_CH2_PACE_MSB8  0x33
#define DATA_CH2_PACE_LSB8  0x34
#define DATA_CH3_PACE_MSB8  0x35
#define DATA_CH3_PACE_LSB8  0x36
#define DATA_CH1_ECG_MSB8  0x37
#define DATA_CH1_ECG_MID8  0x38
#define DATA_CH1_ECG_LSB8  0x39
#define DATA_CH2_ECG_MSB8  0x3A
#define DATA_CH2_ECG_MID8  0x3B
#define DATA_CH2_ECG_LSB8  0x3C
#define DATA_CH3_ECG_MSB8  0x3D
#define DATA_CH3_ECG_MID8  0x3E
#define DATA_CH3_ECG_LSB8  0x3F
#define REV_ID          0x40
#define PE_LPRD         0x50

typedef enum
{
	ECG_NO_ERROR = 0,
	ECG_INIT_ERROR,
	ECG_CHIP_ERROR,
	BAT_LOW,
	LEAD_OFF,
	ECG_DEINIT_ERROR,
	EXIT_REQUESTED,
}ECG_STATUS;

typedef enum
{
	LEAD1=1,
	LEAD2,
	LEAD12=12,
}ECG_LEADS_t;

extern volatile bool ECG_Drdy_Flag;

void API_ADS_Test(void);


void API_ECG_Power_up(void);																			// Power up ecg chip
void API_ECG_Power_down(void);
bool API_ECG_Init(void);
void API_ECG_Chip_Reset(void);
ECG_STATUS API_ECG_Deinit(void);																		// De initialize the spi
ECG_STATUS API_ECG_Reginit_1Lead(void);																	// register settings for single lead
ECG_STATUS API_ECG_Reginit_2Lead(void);																	// register settings for 3 lead
ECG_STATUS API_ECG_Reginit_12Lead();
ECG_STATUS API_ECG_Reginit_Testsetup();																	// setup for test signal
ECG_STATUS API_ECG_Check_Error_Status(void);															// checking for error status
ECG_STATUS IRAM_ATTR API_ECG_Capture_Samples_2Lead(float *buff_lead_1, float *buff_lead_2);								// capture samples for 3 lead
bool API_ECG_Capture_Samples_1Lead(float *buff_lead_1);													// capture sample for single lead
bool API_ECG_Capture_Samples_VLead(float *vlead,uint16_t nbf_samples);
bool API_ECG_Diagnostic_Test(void);

void API_ECG_ADS_Capture_Data(uint32_t nbd_samples); // 600 samples L1, 600 samples L2

uint8_t API_Test_ADS(void);

void API_RUN_ECG_LEAD2_TEST(void);
void API_RUN_ECG_LEAD6_TEST(void);

void API_ECG_Enable_LeadOff_Detection(void);
void API_ECG_Disable_LeadOff_Detection(void);

bool API_ECG_Lead_OFF_Detect(ECG_LEADS_t lead);
void API_ECG_Registers_Check_For_Corruption(void);

void API_ECG_Start_Conversion(void);
void API_ECG_Stop_Conversion(void);
ECG_STATUS API_ECG_Chip_Init(void);

#endif /* API_API_ECG_H_ */
