#include "Quick_Test.h"
#include "API_utility.h"
#include "ecg_config.h"
#include "quick_test_config.h"
#include "bpf.h"
#include "ProjectConfiguration.h"
#include "API_Buzzer.h"
#include "API_Flash_org.h"
#include "API_Display.h"
#include "push_button.h"
#include "API_timer.h"
#include <math.h>
#include "max86150.h"
#include "API_IO_Exp.h"
#include "API_ADS.h"
#include "Error_Handling.h"
#include "API_Flash_org.h"
#include "ECG_12_Lead.h"
#include "driver/periph_ctrl.h"

// MACROS REQUIRED FOR WDOG SpO2

#define SPO2_WAIT_TIME							5
#define SPO2_NUM_SEC_DATA_CAPTURE		      	5
#define SPO2_NUM_SEC_FINGER_DET_DATA_CAPTUR  	(3 * 2)  // 3 x (two time detecting the finger)
#define SPO2_REGISTER_INIT_TIME               	1
#define SPO2_MODULE_INIT_TIME              		(SPO2_REGISTER_INIT_TIME + SPO2_WAIT_TIME)
#define SPO2_DATA_CAPTURE_TIME             		(SPO2_NUM_SEC_DATA_CAPTURE + SPO2_NUM_SEC_FINGER_DET_DATA_CAPTUR)
#define SPO2_DATA_PROCESSING_TIME          		1  // processing time is less than 1sec
#define SPO2_DATA_STORAGE_TIME             		1
//#define SPO2_DISPLAY_DELAY_TIME            		(DISP_RESULT_TIME + (DISP_NOTIFICATION_TIME * 3))// 3 display screens will encounters the delay of  "DISP_NOTIFICATION_TIME"
#define SPO2_WDOG_TIME_FEED                		(SPO2_MODULE_INIT_TIME + SPO2_DATA_CAPTURE_TIME + SPO2_DATA_PROCESSING_TIME + SPO2_DISPLAY_DELAY_TIME + SPO2_DATA_STORAGE_TIME)


// MACROS REQUIRED FOR WDOG BP

#define BP_NUM_SEC_DUMMY_DATA_CAPTURE   		BP_NUM_SEC_DATA_CAPTURE
#define BP_REGISTER_INIT_TIME           		2
#define BP_WAIT_TIME		            		5
#define BP_MODULE_INIT_TIME            			(BP_REGISTER_INIT_TIME + BP_WAIT_TIME)
#define BP_DATA_CAPTURE_TIME           			(BP_NUM_SEC_DATA_CAPTURE * 2)
#define BP_DATA_PROCESSING_TIME         		1
#define BP_DATA_STORAGE_TIME            		(3 * 2) // BP + ECG
//#define BP_DISPLAY_DELAY_TIME         			(DISP_RESULT_TIME + (DISP_NOTIFICATION_TIME * 3))	// 3 display screens will encounters the delay of  "DISP_NOTIFICATION_TIME"
#define BP_WDOG_TIME_FEED              			(BP_MODULE_INIT_TIME + BP_DATA_CAPTURE_TIME + BP_DATA_PROCESSING_TIME + BP_DISPLAY_DELAY_TIME + BP_DATA_STORAGE_TIME)

#define QUICK_TEST_BUZZER_ON_TIME              	(2 + 5)

#define QUICK_TEST_TOTAL_TIME_SPO2              (SPO2_MAX_CAPTURE_ITERATIONS * SPO2_NUM_SEC_DATA_CAPTURE)
#define QUICK_TEST_TOTAL_TIME_BP                (BP_MAX_CAPTURE_ITERATIONS * BP_NUM_SEC_DATA_CAPTURE)

#define QUICK_TEST_SPO2_REG_INIT_STABILIZATION_TIME     5
#define QUICK_TEST_BP_REG_INIT_STABILIZATION_TIME       2

//#define QUICK_TEST_WDOG_TIME_FEED	(BP_WDOG_TIME_FEED + SPO2_WDOG_TIME_FEED + QUICK_TEST_BUZZER_ON_TIME + WDOG_EXTRA_TIME + 90+5+40)

/* Quick Test Flash Storage Configuration Macros */

/***************************************************/
#define BP_PPG_RED_SAMPLES 1200U
#define BP_ECG_L1_SAMPLES  1200U
#define ECG_L1_SAMPLES  600U
#define ECG_L2_SAMPLES  600U
#define SPO2_RED_SAMPLES 600U
#define SPO2_IR_SAMPLES  600U

/***************************************************/
uint32_t SPO2_PPG_IR_BUFF[TOTAL_SAMPLES];
uint32_t SPO2_PPG_RED_BUFF[TOTAL_SAMPLES];
float ECG_Lead1_buff[TOTAL_SAMPLES];
float ECG_Lead2_buff[TOTAL_SAMPLES];
float BP_ECG_Lead1_buff[TOTAL_SAMPLES];
uint32_t BP_PPG_RED_BUFF[TOTAL_SAMPLES];
uint32_t BP_PPG_IR_BUFF[TOTAL_SAMPLES];
float FilterOutputBuffer1[TOTAL_SAMPLES];
float FilterOutputBuffer2[TOTAL_SAMPLES];
float FilterOutputBuffer3[TOTAL_SAMPLES];
float FilterOutputBuffer4[TOTAL_SAMPLES];

char date_info[50];
typedef struct __attribute__((__packed__))
{
	uint32_t  		record_len;
	uint8_t  		patient_id[10];
	uint8_t  		day;
	uint8_t  		month;
	uint8_t  		year;
	uint8_t  		hour;
	uint8_t  		minute;
	uint8_t  		second;
	float 			sbp_multiplier;
	float 			dbp_multiplier;
}BP_MULTIPLIER_TO_FLASH;

typedef enum {
			TEST_SUCCESS = 0,
			SpO2_REG_INIT_FAILED ,
			BP_ECG_REG_INIT_FAILED,
			BP_PPG_REG_INIT_FAILED,
			SpO2_CAPTURE_FAILED,
			BP_CAPTURE_FAILED,
			SpO2_PEAKS_NOT_DETECTED,
			SpO2_VALLEY_NOT_DETECTED,
			SpO2_AC_DC_NOT_IN_RANGE,
			SpO2_NOT_IN_RANGE,
			SpO2_FLASH_WRITE_FAILED,
			BP_ECG_PEAKS_NOT_DETECTED,
			BP_PPG_PEAK_NOT_DETECTED,
			BP_SBP_DBP_NOT_IN_RANGE,
			BP_FLASH_WRITE_FAILED,
			ECG_FLASH_WRITE_FAILED

}QUICK_TEST_STATUS;


static float raw_data_1[1200] = {0};
static float raw_data_2[1200] = {0};
static float filtered_data[1200] = {0};

static float ir_avg_buff[ NUM_SPO2_SAMPLES] = {0};            //compuation_ir_avg_buff
static float red_avg_buff[ NUM_SPO2_SAMPLES] = {0};           //compuation_red_avg_buff

static bool bp_status 	= FALSE;
static bool spo2_status = FALSE;
static bool ecg_status 	= FALSE;

static double avg_ptt_by_hrt = 0.00;

/*----------------------------------------- FUNCTION DECLARATION ----------------------------------------------------*/

static bool spo2_dummy_capture(void);
static bool spo2_capture_raw_data(void);
static bool spo2_4point_average(float input_buff[],float *output_buff, int32_t n_samples);
static bool spo2_peak_detection(float filter_data[], uint16_t peak_location[],uint8_t *total_peak_count, uint16_t nbfSamples);
static bool spo2_find_exact_valley_loc(uint8_t *valley_locs_count,uint16_t exact_valley_locs[],
		 	 	 	 	 	 	 	 	 uint8_t total_peak_count,uint16_t peak_location[]);
static void spo2_maxim_sort_ascend( uint8_t *buffer_x,uint8_t n_size );
static uint8_t spo2_ac_dc_component_ratio(uint8_t valley_locs_count,uint16_t exact_valley_locs[],uint8_t ratio_buffer[]);
static QUICK_TEST_STATUS spo2_store_data_to_flash(const uint32_t spo2_val);

static QUICK_TEST_STATUS bp_dummy_capture(void);
static QUICK_TEST_STATUS bp_capture_raw_data(void);
static bool bp_ecg_peak_detection(uint16_t ecg_peak_pos[0], uint8_t *num_peaks);
static bool bp_ppg_peak_detection(uint16_t ecg_peak_pos[5], uint16_t *ecg_loc_1 ,uint16_t *ecg_loc_2,uint16_t *ppg_peak_pos);
static void bp_calculation(uint16_t ppgloc, uint16_t ecgloc1, uint16_t ecgloc2, uint16_t *sbp, uint16_t *dbp);
static uint16_t heart_rate_computation(uint16_t peak_loc[]);

static QUICK_TEST_STATUS bp_store_data_to_flash(const uint16_t sbp, const uint16_t dbp);
static QUICK_TEST_STATUS ecg_lead1_store_data_to_flash(const uint16_t heart_rate);

static void average_10point(float input[],float averaged_output[]);
static bool bp_ppg_peak_detection_2nd_validation(float output_ECG_PPG[], uint16_t ecg_peak_pos[],uint16_t ppg_peak_pos[]);

static void quick_test_clear_buffers(void);
static void quick_test_set_wdog(void);
static bool spo2_reg_int(void);
static bool bp_reg_int(void);
static bool quick_test_flash_memory_status_check(void);
static QUICK_TEST_STATUS bp_store_multipliers_to_flash(const float sbp_multi,const float dbp_multi);

//off-line feature related functions
void store_spo2_data_to_flash_offline(uint32_t result);
void store_bp_data_to_flash(uint32_t result);
void ecg_lead1_hr_to_flash_offline(uint32_t heart_rate);

bool Lead12_LeadOff_Detect(void);

bool skip_quick_test = FALSE;

bool QUICK_Test1(void);

bool QUICK_Test1(void)
{
	uint16_t result[4] = {0};

	printf("\nQuick Test1 Started");

	Enable_Power_Supply();

	/*if(API_ECG_Init())
	{
		if(API_ECG_Lead_OFF_Detect(LEAD1))
		{
			API_Disp_Display_Lead_Connection_Status((uint8_t)LEAD1);

			Disable_Power_Supply();

			return false;
		}
		API_ECG_Chip_Reset();
	}*/

	API_ECG_Chip_Reset();

	if(API_ECG_Init())
	{

		printf("\nECG Init Successful");
		printf("Entering into Max86150 setup");

		   if(API_MAX86150_Setup() == true)
		   {
			   printf("\nCapturing data.................");
			   API_Disp_Quick_test_screen(DISP_QT_TEST_IN_PROGRESS);
			   // TODO; retry when raw data is not good
			   if(Capture_PPG_ECG_Data(CAPTURE_ECG_L1,true) == false)
				   {
						Disable_Power_Supply();

				   	   return false;
				   }
			   //Filter_Quicktest1_Data();
			   // Peak detection
			   // Result Computation
			   Vital_result.SBP1 = 0; // Should remove this when BP estimation Algorithm implemented
			   Vital_result.DBP1 = 0; // Should remove this when BP estimation Algorithm implemented


			   API_Disp_Quick_Test_Result(result);
			   if(Selected_PID_type != GUEST_PID) Store_QuickTest1_Data_To_Flash();
		   }

		   else
		   {
			   printf("\nMAX86150 Init Fail");
		   }

	}

	Disable_Power_Supply();
	API_Buzzer_Sound(SHORT_BEEP);

	printf("\nTest completed.");
	return true;
}


 bool Run_Quick_Vital(void)
{
	uint16_t result[4] = {0};

	printf("\nQuick Test2 Started");

	Is_time_displayed = TRUE;
	API_DISP_Toggle_Date_Time();
	API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);

	 API_IO_Exp_Power_Control(EN_VLED,HIGH);
	 API_IO_Exp_Power_Control(EN_ANALOG,HIGH);
	 API_IO_Exp_Power_Control(EN_IR,HIGH);

	if((Selected_PID_type == VALID_PID) || (Selected_PID_type == GUEST_PID))
	{
			if(API_Flash_Org_Check_For_Memory_Free())
			{

				//API_Buzzer_Sound(SHORT_BEEP);

				API_Disp_Quick_Test_Icon();

				API_Disp_Quick_test_screen(DISP_QT_PLACE_FINGER);

				printf("\nEnteriing into ECG init");

				Enable_Power_Supply();
			//	Select_Vlead(LEAD6);// Enable only 6 lead
				API_IO_Exp_Power_Control(EN_IR,LOW);
				API_ECG_Chip_Reset();

				/*	if(API_ECG_Init())
				{

					if(API_ECG_Lead_OFF_Detect(LEAD2))
					{
						API_Disp_Display_Lead_Connection_Status((uint8_t)LEAD2);

						 API_Disp_Quick_test_screen(DISP_QT_TEST_IN_PROGRESS);
						 API_Disp_Quick_Test_Icon();

						bool status = QUICK_Test1();

						if(status == false)
						{

						}

						Disable_Power_Supply();

						return false;
					}
					API_ECG_Chip_Reset();
				}
*/

				 if(API_MAX86150_Setup())
				 {
					if(API_ECG_Init())
						{
							API_Disp_Quick_test_screen(DISP_QT_PPG_TEST_IN_PROGRESS);

							//while(1){
							Print_time("\nSPO2 start");
							Capture_PPG_ECG_Data(CAPTURE_PPG,true);
							Print_time("\nSPO2 end");
						//	Filter_Quicktest1_Data();
						//	}
							//Capture_BP_Data(true);

							//Check_PPG_Data_Quality();

							//API_ECG_Registers_Check_For_Corruption();

							//if(API_Temperature_Init())

								API_Disp_Quick_test_screen(DISP_QT_ECG_L1_TEST_IN_PROGRESS);
								Print_time("/ECG start");
								   if(Capture_PPG_ECG_Data(CAPTURE_ECG_L1_AND_L2,true)==false)
									   {
										   Disable_Power_Supply();
										   return false;
									   }
								   Print_time("\nECG end");
									 printf("\nCapturing BP................");

                                      Print_time("\nBP START");
									 Capture_BP_Data(true);
									 Print_time("\nBP END");

											// Filter_Quicktest1_Data();
											// Peak detection
											// Result Computation

											// Filter_Quicktest1_Data();
											// Peak detection
											// Result Computation

										   Vital_result.SBP1 = 117; // Need to change later
										   Vital_result.DBP1 = 77; // Need to change later

											result[0] = 98;
											result[1] = 85;
											result[2] = 115;
											result[3] = 85;

										   API_Disp_Quick_Test_Result(result);
										   if(Selected_PID_type != GUEST_PID) Store_QuickTest1_Data_To_Flash();
							   }

						 }

				}

			else
			{
				printf("\nDevice Memory Full... Please sync the data");
				 API_DISP_Memory_Full_Status();
			}
	}

	else
	{
        printf("\nPlease select the PID");
        API_Disp_Quick_test_screen(DISP_QT_PLEASE_REGISTER_PID);
	}

	API_Buzzer_Sound(SHORT_BEEP);

	Disable_Power_Supply();

  	printf("\nTest completed.");

	return true;
}


void Dummy_Capture(uint16_t total_samples)
{

    uint16_t raw_data_index;
    esp_err_t status;
    uint32_t max_ecg_no_use; // Not using ecg data from max86150 as hardware will not support
    float value1;
    float value2;
    uint32_t value3;
    uint32_t value4;

	for(raw_data_index=0; raw_data_index<total_samples; raw_data_index++)
	{
		status = API_ECG_Capture_Samples_2Lead(&value1,&value2);

		if(status != ESP_OK) Catch_RunTime_Error(ECG_DATA_CAPTURE_FAIL);

		status = API_MAX86150_Raw_Data_capture(&value3,&value4,&max_ecg_no_use,1,0,0);

		if(status != ESP_OK) Catch_RunTime_Error(PPG_DATA_CAPTURE_FAIL);
	}

}

 bool Capture_PPG_ECG_Data(DATA_CAPTURE_TYPE_t captureType, bool enableDummyCapture)
{
    uint16_t raw_data_index;
    esp_err_t status=ESP_FAIL;

    uint32_t max_ecg_no_use[1]; // Not using ecg data from max86150 as hardware will not support

    bool dataQuality = false;
	//API_IO_Exp2_P1_write_pin(DC_LEAD_OFF,LOW);
	//API_IO_Exp1_P1_write_pin(DC_LEAD_OFF_V,LOW);

	API_IO_Exp1_P1_write_pin(DISPLAY_CSN,HIGH);

	API_IO_Exp1_P0_write_pin(EFM_DISP_EN2,LOW);

	if(captureType == CAPTURE_ECG_L1)
	{
		API_ECG_Start_Conversion();

		if(enableDummyCapture)
		{
			for(raw_data_index=0; raw_data_index<100; raw_data_index++)// ~3sec dummy capture
			{
				status = API_ECG_Capture_Samples_2Lead(ECG_Lead1_buff + raw_data_index, ECG_Lead2_buff+raw_data_index);
			}
		}

		MemSet(ECG_Lead1_buff,0,sizeof(ECG_Lead1_buff));
		MemSet(ECG_Lead2_buff,0,sizeof(ECG_Lead2_buff));

		for(raw_data_index=0; raw_data_index<600; raw_data_index++)
			{
				status = API_ECG_Capture_Samples_2Lead(ECG_Lead1_buff + raw_data_index, ECG_Lead2_buff+raw_data_index);
			}

		for(int i=0;i<600;i++)
					{
					  printf("\n%f",ECG_Lead1_buff[i]);
					}

		MemSet(ECG_Lead2_buff,0,sizeof(ECG_Lead2_buff)); // Lead1 capture doesn't need Lead2 data.

		API_ECG_Stop_Conversion();

	}

	else if(captureType == CAPTURE_ECG_L2)
	{
		API_ECG_Start_Conversion();


		if(enableDummyCapture)
		{
			for(raw_data_index=0; raw_data_index<100; raw_data_index++)// ~3sec dummy capture
			{
				status = API_ECG_Capture_Samples_2Lead(ECG_Lead1_buff + raw_data_index, ECG_Lead2_buff+raw_data_index);
			}
		}

	MemSet(ECG_Lead1_buff,0,sizeof(ECG_Lead1_buff));
	MemSet(ECG_Lead2_buff,0,sizeof(ECG_Lead2_buff));

	for(raw_data_index=0; raw_data_index<600; raw_data_index++)
		{
			status = API_ECG_Capture_Samples_2Lead(ECG_Lead1_buff + raw_data_index, ECG_Lead2_buff+raw_data_index);
		}
	printf("\nECG L1");

	for(int i=0;i<600;i++)
			{
			  printf("%f\n",ECG_Lead1_buff[i]);
			}
	printf("\nECG L2");
	for(int i=0;i<600;i++)
			{
			  printf("%f\n",ECG_Lead2_buff[i]);
			}

	API_ECG_Stop_Conversion();

	}

	else if(captureType == CAPTURE_PPG)
	{
		Max86150_Clear_Fifo();


		if(enableDummyCapture)
		{
			for(raw_data_index=0; raw_data_index<300; raw_data_index++)// ~3sec dummy capture
			{
				status = API_MAX86150_Raw_Data_capture(SPO2_PPG_RED_BUFF+raw_data_index, SPO2_PPG_IR_BUFF+raw_data_index,max_ecg_no_use,1,0,0);
			}
		}

		MemSet(SPO2_PPG_RED_BUFF,0,sizeof(SPO2_PPG_RED_BUFF));
		MemSet(SPO2_PPG_IR_BUFF,0,sizeof(SPO2_PPG_IR_BUFF));


		for(raw_data_index=0; raw_data_index<600; raw_data_index++)
			{
				status = API_MAX86150_Raw_Data_capture(SPO2_PPG_RED_BUFF+raw_data_index, SPO2_PPG_IR_BUFF+raw_data_index,max_ecg_no_use,1,0,0);
			}

		printf("\nSPO2 PPG-RED DATA:");
		for(int i=0;i<600;i++)
			{
			  printf("\n%ld",SPO2_PPG_RED_BUFF[i]);
			}
		printf("\nSPO2 PPG-IR DATA:");
		for(int i=0;i<600;i++)
			{
			  printf("\n%ld",SPO2_PPG_IR_BUFF[i]);
			}
	}

	else if(captureType == CAPTURE_ECG_L1_AND_L2)
		{
			API_ECG_Start_Conversion();

			if(enableDummyCapture)
			{
				for(raw_data_index=0; raw_data_index<100; raw_data_index++)// ~3sec dummy capture
				{
					status = API_ECG_Capture_Samples_2Lead(ECG_Lead1_buff + raw_data_index, ECG_Lead2_buff+raw_data_index);
				}
			}

			MemSet(ECG_Lead1_buff,0,sizeof(ECG_Lead1_buff));
			MemSet(ECG_Lead2_buff,0,sizeof(ECG_Lead2_buff));

			for(raw_data_index=0; raw_data_index<600; raw_data_index++)
				{
					status = API_ECG_Capture_Samples_2Lead(ECG_Lead1_buff + raw_data_index, ECG_Lead2_buff+raw_data_index);
				}

			printf("\nECG L1 data:\n");
			for(int i=0;i<600;i++)
			{
			  printf("\n%f",ECG_Lead1_buff[i]);
			}

			printf("\nECG L2 data:\n");
			for(int i=0;i<600;i++)
			{
			  printf("\n%f",ECG_Lead2_buff[i]);
			}

			API_ECG_Stop_Conversion();

		}

	else
	{
		;
	}

	API_IO_Exp1_P0_write_pin(EFM_DISP_EN2,HIGH);
	API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);

	return (status==ESP_OK);
}

 bool Capture_BP_Data(bool enableDummyCapture)
 {
		uint32_t max_ecg_no_use[1U] ={0U}; // Not using ecg data from max86150 as hardware will not support
		uint16_t raw_data_index =0U;
		esp_err_t status = ESP_FAIL;

		API_ECG_Start_Conversion();
		Max86150_Clear_Fifo();

		if(enableDummyCapture)
		{
			for(raw_data_index=0; raw_data_index<600; raw_data_index++)// ~3sec dummy capture
			{
				status = API_ECG_Capture_Samples_2Lead(BP_ECG_Lead1_buff + raw_data_index, ECG_Lead2_buff+raw_data_index);
				status = API_MAX86150_Raw_Data_capture(BP_PPG_RED_BUFF+raw_data_index, BP_PPG_IR_BUFF+raw_data_index,max_ecg_no_use,1,0,0);
			}
		}

		MemSet(BP_ECG_Lead1_buff,0,sizeof(BP_ECG_Lead1_buff));
		MemSet(ECG_Lead2_buff,0,sizeof(ECG_Lead2_buff));
		MemSet(BP_PPG_RED_BUFF,0,sizeof(BP_PPG_RED_BUFF));
		MemSet(BP_PPG_IR_BUFF,0,sizeof(BP_PPG_IR_BUFF));

		for(raw_data_index=0; raw_data_index<600; raw_data_index++)
			{
				status = API_ECG_Capture_Samples_2Lead(BP_ECG_Lead1_buff + raw_data_index, ECG_Lead2_buff+raw_data_index);
				status = API_MAX86150_Raw_Data_capture(BP_PPG_RED_BUFF+raw_data_index, BP_PPG_IR_BUFF+raw_data_index,max_ecg_no_use,1,0,0);
			}

		printf("\nBP ECG L1 data:\n");
		for(int i=0;i<600;i++)
		{
		  printf("%f\n",BP_ECG_Lead1_buff[i]);
		}
		/*
		printf("\nECG L2 data:\n");
		for(int i=0;i<600;i++)
		{
		  printf("\n%f",ECG_Lead2_buff[i]);
		}*/

		printf("\nBP PPG Red data:\n");
		for(int i=0;i<600;i++)
		{
			printf("%ld\n",BP_PPG_RED_BUFF[i]);
		}

		/*printf("\nPPG IR data:\n");
		for(int i=0;i<600;i++)
		{
			printf("\n%d",PPG_IR_BUFF[i]);
		}
*/
		API_ECG_Stop_Conversion();

		return true;
 }

void Filter_Quicktest1_Data(void)
{
	//filter(ECG_Lead1_buff,FilterOutputBuffer1,TOTAL_SAMPLES,100);
	//filter(ECG_Lead2_buff,FilterOutputBuffer2,TOTAL_SAMPLES,100);
	//filter((float *)PPG_RED_BUFF,FilterOutputBuffer3,600,100);
	//filter((float *)PPG_IR_BUFF,FilterOutputBuffer4,TOTAL_SAMPLES,100);

	for(int i=0;i<600;i++)
	{
		//printf("\n%f",FilterOutputBuffer3[i]);
	}

//	if(ENABLE_DEBUG_MESSAGES)
//	{
//		printf("\n\nECG Lead 1 Filter data");
//
//		for(int i=0;i<TOTAL_SAMPLES;i++)
//		{
//			printf("\n%f",FilterOutputBuffer1[i]);
//		}
//
//		printf("\n\nECG Lead 2 Filter data");
//
//		for(int i=0;i<TOTAL_SAMPLES;i++)
//		{
//			printf("\n%f",FilterOutputBuffer2[i]);
//		}
//
//		printf("\n\nPPG RED Filter data");
//
//		for(int i=0;i<TOTAL_SAMPLES;i++)
//		{
//			printf("\n%f",FilterOutputBuffer3[i]);
//		}
//
//		printf("\n\nPPG IR Filter data");
//
//		for(int i=0;i<TOTAL_SAMPLES;i++)
//		{
//			printf("\n%f",FilterOutputBuffer4[i]);
//		}
//	}

}


void Filter_Quicktest2_Data(void)
{
	filter(ECG_Lead1_buff,FilterOutputBuffer1,TOTAL_SAMPLES,100);
	filter(ECG_Lead2_buff,FilterOutputBuffer2,TOTAL_SAMPLES,100);
	filter((float *)SPO2_PPG_RED_BUFF,FilterOutputBuffer3,TOTAL_SAMPLES,100);
	filter((float *)SPO2_PPG_IR_BUFF,FilterOutputBuffer4,TOTAL_SAMPLES,100);

	if(ENABLE_DEBUG_MESSAGES)
	{
		printf("\n\nECG Lead 1 Filter data");

		for(int i=0;i<TOTAL_SAMPLES;i++)
		{
			printf("\n%f",FilterOutputBuffer1[i]);
		}

		printf("\n\nECG Lead 2 Filter data");

		for(int i=0;i<TOTAL_SAMPLES;i++)
		{
			printf("\n%f",FilterOutputBuffer2[i]);
		}

		printf("\n\nPPG RED Filter data");

		for(int i=0;i<TOTAL_SAMPLES;i++)
		{
			printf("\n%f",FilterOutputBuffer3[i]);
		}

		printf("\n\nPPG IR Filter data");

		for(int i=0;i<TOTAL_SAMPLES;i++)
		{
			printf("\n%f",FilterOutputBuffer4[i]);
		}
	}
}

void Store_QuickTest1_Data_To_Flash(void)
{
	RECORD_OPS_STATUS status;

	uint32_t offfset = 0;

	API_Update_Record_Header(BP1,&record_header);

	MemCpy(BT_flash_buffer+offfset,&record_header,REC_HEADER_LEN);

	offfset = REC_HEADER_LEN;
	MemCpy(BT_flash_buffer+offfset,BP_PPG_RED_BUFF,(BP_PPG_RED_SAMPLES*4));

	offfset += BP_PPG_RED_SAMPLES*4;
	MemCpy(BT_flash_buffer+offfset,BP_ECG_Lead1_buff,(BP_ECG_L1_SAMPLES*4));

	status = API_Flash_Write_Record(BP1,(void*)BT_flash_buffer);

	if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(BP_DATA_STORE_TO_FLASH_FAIL);


		API_Update_Record_Header(ECG_1_Lead,&record_header);

		MemSet(BT_flash_buffer,0,sizeof(BT_flash_buffer));

		MemCpy(BT_flash_buffer,&record_header,REC_HEADER_LEN);
		offfset = REC_HEADER_LEN;

		MemCpy(BT_flash_buffer+offfset,ECG_Lead1_buff,(BP_ECG_L1_SAMPLES*4));
		offfset += BP_ECG_L1_SAMPLES*4;

		status = API_Flash_Write_Record(ECG_1_Lead,(void*)BT_flash_buffer);

		if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(ECG_DATA_STORE_TO_FLASH_FAIL);


		MemSet(BT_flash_buffer,0,sizeof(BT_flash_buffer));

		API_Update_Record_Header(SPO2,&record_header);

		MemCpy(BT_flash_buffer,&record_header,REC_HEADER_LEN);
		offfset = REC_HEADER_LEN;

		MemCpy(BT_flash_buffer+offfset,SPO2_PPG_RED_BUFF,(SPO2_RED_SAMPLES*4));

		offfset += SPO2_RED_SAMPLES*4;
		MemCpy(BT_flash_buffer+offfset,SPO2_PPG_IR_BUFF,(SPO2_IR_SAMPLES*4));

		status = API_Flash_Write_Record(SPO2,(void*)BT_flash_buffer);

		if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(SPO2_DATA_STORE_TO_FLASH_FAIL);

		IsValidRecordsInFlash = true;

		printf("\nTotal SPO2 Records = %ld", get_records_count(SPO2));
		printf("\nTotal BP1 Records = %ld", get_records_count(BP1));
		printf("\nTotal ECG L1 Records = %ld", get_records_count(ECG_1_Lead));

}


void Test_Store_QuickTest1_Data_To_Flash(void)
{
	RECORD_OPS_STATUS status;

	uint32_t offfset = 0;

	API_Update_Record_Header(BP1,&record_header);

	MemSet(&record_header,1,sizeof(record_header));

	MemCpy(BT_flash_buffer+offfset,&record_header,REC_HEADER_LEN);

	offfset = REC_HEADER_LEN;

	MemSet(BP_PPG_RED_BUFF,2,sizeof(BP_PPG_RED_BUFF));

	MemCpy(BT_flash_buffer+offfset,BP_PPG_RED_BUFF,(BP_PPG_RED_SAMPLES*4));

	MemSet(BP_ECG_Lead1_buff,3,sizeof(BP_ECG_Lead1_buff));

	offfset += BP_PPG_RED_SAMPLES*4;
	MemCpy(BT_flash_buffer+offfset,BP_ECG_Lead1_buff,(BP_ECG_L1_SAMPLES*4));

	status = API_Flash_Write_Record(BP1,(void*)BT_flash_buffer);

	if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(BP_DATA_STORE_TO_FLASH_FAIL);

		API_Update_Record_Header(ECG_1_Lead,&record_header);

		MemSet(&record_header,5,sizeof(record_header));

		MemSet(BT_flash_buffer,0,sizeof(BT_flash_buffer));

		MemCpy(BT_flash_buffer,&record_header,REC_HEADER_LEN);
		offfset = REC_HEADER_LEN;

		MemSet(ECG_Lead1_buff,6,sizeof(ECG_Lead1_buff));

		MemCpy(BT_flash_buffer+offfset,ECG_Lead1_buff,(BP_ECG_L1_SAMPLES*4));
		offfset += BP_ECG_L1_SAMPLES*4;

		status = API_Flash_Write_Record(ECG_1_Lead,(void*)BT_flash_buffer);

		if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(ECG_DATA_STORE_TO_FLASH_FAIL);


		MemSet(BT_flash_buffer,0,sizeof(BT_flash_buffer));

		API_Update_Record_Header(SPO2,&record_header);

		MemSet(&record_header,7,sizeof(record_header));

		MemCpy(BT_flash_buffer,&record_header,REC_HEADER_LEN);
		offfset = REC_HEADER_LEN;

		MemSet(SPO2_PPG_RED_BUFF,8,sizeof(SPO2_PPG_RED_BUFF));

		MemCpy(BT_flash_buffer+offfset,SPO2_PPG_RED_BUFF,(SPO2_RED_SAMPLES*4));

		offfset += SPO2_RED_SAMPLES*4;

		MemSet(SPO2_PPG_IR_BUFF,9,sizeof(SPO2_PPG_IR_BUFF));

		MemCpy(BT_flash_buffer+offfset,SPO2_PPG_IR_BUFF,(SPO2_IR_SAMPLES*4));

		status = API_Flash_Write_Record(SPO2,(void*)BT_flash_buffer);

		if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(SPO2_DATA_STORE_TO_FLASH_FAIL);

		printf("\nTotal SPO2 Records = %ld", get_records_count(SPO2));
		printf("\nTotal BP Records = %ld", get_records_count(BP1));
		printf("\nTotal ECG L1 Records = %ld", get_records_count(ECG_1_Lead));

}

void Store_QuickTest2_Data_To_Flash(void)
{
	RECORD_OPS_STATUS status;

	uint32_t offfset = 0;

	API_Update_Record_Header(BP1,&record_header);

	MemCpy(BT_flash_buffer+offfset,&record_header,REC_HEADER_LEN);

	offfset = REC_HEADER_LEN;
	MemCpy(BT_flash_buffer+offfset,BP_PPG_RED_BUFF,(BP_PPG_RED_SAMPLES*4));

	offfset += BP_PPG_RED_SAMPLES*4;
	MemCpy(BT_flash_buffer+offfset,BP_ECG_Lead1_buff,(BP_ECG_L1_SAMPLES*4));

	status = API_Flash_Write_Record(BP1,(void*)BT_flash_buffer);

	if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(BP_DATA_STORE_TO_FLASH_FAIL);


		API_Update_Record_Header(ECG_6_Lead,&record_header);

		MemSet(BT_flash_buffer,0,sizeof(BT_flash_buffer));

		MemCpy(BT_flash_buffer,&record_header,REC_HEADER_LEN);
		offfset = REC_HEADER_LEN;

		MemCpy(BT_flash_buffer+offfset,ECG_Lead1_buff,(ECG_L1_SAMPLES*4));
		offfset += ECG_L1_SAMPLES*4;

		MemCpy(BT_flash_buffer+offfset,ECG_Lead2_buff,(ECG_L2_SAMPLES*4));

		status = API_Flash_Write_Record(ECG_6_Lead,(void*)BT_flash_buffer);

		if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(ECG_DATA_STORE_TO_FLASH_FAIL);


		MemSet(BT_flash_buffer,0,sizeof(BT_flash_buffer));

		API_Update_Record_Header(SPO2,&record_header);

		MemCpy(BT_flash_buffer,&record_header,REC_HEADER_LEN);
		offfset = REC_HEADER_LEN;

		MemCpy(BT_flash_buffer+offfset,SPO2_PPG_RED_BUFF,(SPO2_RED_SAMPLES*4));

		offfset += SPO2_RED_SAMPLES*4;
		MemCpy(BT_flash_buffer+offfset,SPO2_PPG_IR_BUFF,(SPO2_IR_SAMPLES*4));

		status = API_Flash_Write_Record(SPO2,(void*)BT_flash_buffer);

		if(status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(SPO2_DATA_STORE_TO_FLASH_FAIL);

		IsValidRecordsInFlash = true;
		printf("\nTotal SPO2 Records = %ld", get_records_count(SPO2));
		printf("\nTotal BP Records = %ld", get_records_count(BP1));
		printf("\nTotal ECG L1 Records = %ld", get_records_count(ECG_6_Lead));

}


/*bool spo2_find_exact_valley_loc(uint32_t *valley_locs_count,uint32_t *exact_valley_locs,
*							 		uint32_t total_peak_count,uint16_t peak_location[])
* \brief        To find_signal_ratio
* \param[in]    valley_locs_count[output]-it tells number of valley location
*               exact_valley_locs[output]-locations of valley
*               ratio_buffer[output]-it store the ratio of SPO2 calculated
*               peaks[input] - number of peaks
*               filp_data_peak_locs[][input]-the raw signal is filpped and its peak locs
* \param[out]   bool. true vally locations>2 false <2 locations
* \retval       TRUE - capture signal has good signal ratio
*        		FALSE -signal ratio is not good
*/

bool spo2_find_exact_valley_loc(uint8_t *valley_locs_count,uint16_t *exact_valley_locs,
							 uint8_t total_peak_count,uint16_t peak_location[])
{
	uint32_t ir_valley_locs[15] = {0};
	uint32_t is_only_once = 0 ;
	uint32_t minimum_value = 0;
	uint32_t exact_valley_loc_count = 0;
	uint32_t samples_count = 0;
	uint16_t spo2_min_index = 0;
	bool signal_ratio_status = TRUE ;
	bool vally_location_range_status = TRUE;

	 for ( samples_count=0 ;samples_count < total_peak_count;samples_count++)
	 {
		  ir_valley_locs[samples_count] = peak_location[samples_count] + 2; //here 3 is (filter coefficient size/2)
	 }
	 // we need to assess DC and AC value of ir and red PPG

	// 4 pt avg of Red and IR signal
	spo2_4point_average(raw_data_2,ir_avg_buff,NUM_SPO2_SAMPLES);
	spo2_4point_average(raw_data_1,red_avg_buff,NUM_SPO2_SAMPLES);

	exact_valley_loc_count = 0;														//find precise MIN near ir_valley_loc

	for(samples_count = 0 ; samples_count < total_peak_count ;samples_count++)
    {
		is_only_once = 1;
		minimum_value = 16777216; 											//2^24 byte read(8*3=24).

		for(spo2_min_index = (ir_valley_locs[samples_count] - 6);
				spo2_min_index < (ir_valley_locs[samples_count] + 6);spo2_min_index++)
		{
			if(ir_avg_buff[spo2_min_index] < minimum_value)
			{
				if(is_only_once > 0)
				{
					is_only_once = 0;
				}
				minimum_value = ir_avg_buff[spo2_min_index];
				exact_valley_locs[samples_count] = spo2_min_index;
			}
		}
		if(is_only_once == 0)
		{
			exact_valley_loc_count ++;
		}
   }
   if (exact_valley_loc_count < 2 )
   {
	   signal_ratio_status= FALSE ;
   }

   *valley_locs_count = exact_valley_loc_count;

   return (vally_location_range_status & signal_ratio_status);
}

/*
* uint8_t spo2_ac_dc_component_ratio(uint32_t valley_locs_count,
		                           uint16_t exact_valley_locs[],uint32_t ratio_buffer[])
* \brief        ac/dc component ratio
* \par          valley_locs_count[input]-number of vally_location
* 		        exact_valley_locs[][input]-vally_location,
* 				ratio_buffer[][input] -ratio of SPO2_calculated
*
* return value  int32_t value
*               which return the ratio_count value
*/
uint8_t spo2_ac_dc_component_ratio(uint8_t valley_locs_count,uint16_t exact_valley_locs[],uint8_t ratio_buffer[])
{
	int32_t red_AC_component = 0;   		//AC component of red_led_data
	int32_t ir_AC_component = 0;    		//AC component of ir_led_data
	int32_t red_DC_max = 0;       			//DC maximum value of red_led
	int32_t ir_DC_max = 0;        			//DC maximum value of ir_led
	int32_t red_DC_max_index = 0;
	int32_t ir_DC_max_index = 0;
	int32_t red_ac_component = 0;
	int32_t ir_ac_component = 0;
	int32_t middle_index = 0;
	uint16_t temp_ratio = 0;				//temp_ratio is used to store ratio result temporary
	uint16_t ratio_average = 0;
	uint16_t loc_count = 0;
	uint16_t ir_loc_index = 0;
	uint8_t ratio_count = 0;

    for (loc_count = 0; loc_count < (valley_locs_count - 1); loc_count++)
    {
    	ir_DC_max = -16777216;
    	red_DC_max = -16777216;

        if (exact_valley_locs[loc_count+1]-exact_valley_locs[loc_count] > 10)
        {
            for (ir_loc_index = exact_valley_locs[loc_count];
            	 ir_loc_index <  exact_valley_locs[loc_count+1]; ir_loc_index++)
            {
                if (ir_avg_buff[ir_loc_index] > ir_DC_max)
                {
                	ir_DC_max = ir_avg_buff[ir_loc_index];
                	ir_DC_max_index =ir_loc_index;
                }
                if (red_avg_buff[ir_loc_index] > red_DC_max)
                {
                	red_DC_max = red_avg_buff[ir_loc_index];
                	red_DC_max_index = ir_loc_index;
                }
            }

            //----------------------------------------------------------------------------------//
            red_AC_component = (red_avg_buff[exact_valley_locs[loc_count+1]]- red_avg_buff[exact_valley_locs[loc_count]])
										*(red_DC_max_index - exact_valley_locs[loc_count]); //red

            red_AC_component= red_avg_buff[exact_valley_locs[loc_count]]
							+ red_AC_component / (exact_valley_locs[loc_count+1]- exact_valley_locs[loc_count])  ;


            // Subract linear DC components from Red raw
            red_AC_component =  red_avg_buff[red_DC_max_index] - red_AC_component;

            //---------------------------------------------------------------------------------//
            ir_AC_component = (ir_avg_buff[exact_valley_locs[loc_count+1]] - ir_avg_buff[exact_valley_locs[loc_count]] )
										*(ir_DC_max_index -exact_valley_locs[loc_count]); // ir

            ir_AC_component =  ir_avg_buff[exact_valley_locs[loc_count]]
							+ ir_AC_component/ (exact_valley_locs[loc_count+1] - exact_valley_locs[loc_count]);

            // Subract linear DC components from IR raw
            ir_AC_component =  ir_avg_buff[ir_DC_max_index] - ir_AC_component;

            //---------------------------------------------------------------------------------//

            //calculation of SPO2 Using the formula

            red_ac_component = ( red_AC_component * ir_DC_max) >> 7 ; //prepare X100 to preserve floating value
            ir_ac_component = ( ir_AC_component * red_DC_max) >> 7;


			if (ir_ac_component > 0  && ratio_count < 5 &&  red_ac_component != 0)
			{
				//Formula: ( red_AC_component * ir_DC_max) / ( ir_AC_component * red_DC_max) ;
				temp_ratio = (red_ac_component * 100) / ir_ac_component ;
				if(temp_ratio < 116)
				{
					ratio_buffer[ratio_count] = temp_ratio ;
					ratio_count ++;
				}
			}
        }
    }

    spo2_maxim_sort_ascend(ratio_buffer, ratio_count);

   middle_index = ratio_count/2;
   if (middle_index > 1)
   {
	   ratio_average = ( ratio_buffer[middle_index-1] +ratio_buffer[middle_index])/2; // use median
   }
   else
   {
	   ratio_average = ratio_buffer[middle_index ];
   }

   return ratio_average;

}


/**************************************************************************************************
* bool average_4(uint32_t input_buff[],uint32_t *output_buff, uint32_t n_samples)
* \brief        function to average the sample data (4 & 8 averaging)
* \param[in]    input_buff[]  	- raw data buffer to be averaged
* \param[in]    sample no		- number of samples to be averaged
*
*
* \param[out]    output_buff   - averaged data buffer of the input buff
*
* \retval      bool, FALSE on successful averaging
***************************************************************************************************/
bool spo2_4point_average(float input_buff[],float *output_buff, int32_t n_samples)
{
	bool averaging_status = FALSE;
	uint16_t sample_count;

	for(sample_count = 0;sample_count < (n_samples);sample_count++)
	{
		output_buff[sample_count] = (input_buff[sample_count] + input_buff[sample_count+1] + input_buff[sample_count+2] + input_buff[sample_count+3])/4;
	}
    averaging_status = TRUE;

    return averaging_status;
}

/*
* void spo2_maxim_sort_ascend(int32_t *buffer_x,int32_t n_size)
* \brief        Sort array in ascending order (insertion sort algorithm)
* \param[in]    peak location buffer,
* \param[in]	size of buffer
*               Sort array in ascending order (insertion sort algorithm)
* \retval       None
*/
static void spo2_maxim_sort_ascend(uint8_t *buffer_x,uint8_t n_size)
{
    uint16_t sample_count, j, n_temp;
    for (sample_count = 1; sample_count < n_size; sample_count++)
    {
        n_temp = buffer_x[sample_count];
        for (j = sample_count; j > 0 && n_temp < buffer_x[j-1]; j--)
        {
            buffer_x[j] = buffer_x[j-1];
        }
        buffer_x[j] = n_temp;
    }
}

/*static void spo2_peak_detection(float filter_data[], uint16_t peak_location[],uint16_t *total_peak_count)
 * \brief		peak detection logic to detect the peaks in the filtered data
 * 				we are using the raising threshold 0.1 and falling threshold 0.2 peak threshold is 0.2
 *				rising  and falling side range is 10 samples.
 * \param[in]	filtered data
 * \param[in]	buffer to store peak locations
 * \param[in]	variable to store the count of total_peak_count
 *
 * \retval		None
 */
static bool spo2_peak_detection(float filter_data[], uint16_t peak_location[],uint8_t *total_peak_count, uint16_t nbfSamples)
{
	float temp_filt_max = 0.0;
	float temp_ppg_falling_edge_valley_value = -1;
	float temp_ppg_falling_edge_peak_value = -1;

	uint16_t samples_count = 0;
	uint16_t peaks_count = 0;
	uint16_t ir_index = 0;
	uint8_t isMaxValue = 0;
	uint8_t num_of_peaks_count = 0;
	uint16_t ppg_peak_index = 0;
	bool peak_detection_status = FALSE;

	assert(nbfSamples>SPO2_TH_RANGE_RISE_SIDE);

	for(samples_count = SPO2_TH_RANGE_RISE_SIDE; samples_count < (nbfSamples - SPO2_TH_RANGE_RISE_SIDE) ; samples_count++)
	{
		if((filter_data[samples_count] >= SPO2_PEAK_DETECT_TH) &&
				(filter_data[samples_count] > filter_data[samples_count - SPO2_TH_RANGE_RISE_SIDE]) &&
				(filter_data[samples_count] > filter_data[samples_count + SPO2_TH_RANGE_FALL_SIDE]) &&
				(filter_data[samples_count] -filter_data[samples_count - SPO2_TH_RANGE_RISE_SIDE] >= 0.2) &&
				(filter_data[samples_count] -filter_data[samples_count + SPO2_TH_RANGE_RISE_SIDE] >= 0.2))
		{
			isMaxValue = 1;

			for(ir_index = (samples_count - SPO2_TH_RANGE_RISE_SIDE);ir_index <= (samples_count + SPO2_TH_RANGE_FALL_SIDE);ir_index++)
			{
				temp_filt_max = filter_data[samples_count];
				if(filter_data[ir_index] > temp_filt_max)
				{
					isMaxValue = 0;
					break;
				}
			}
			if(isMaxValue)
			{
				peak_location[peaks_count++] = samples_count;
			}
		}
	}
	if(peaks_count > 3)
	{
		peak_detection_status = TRUE;
	}

	if(peak_detection_status == TRUE)
		{
			/* Falling PPG peak validation */
			for(num_of_peaks_count = 0; num_of_peaks_count < peaks_count-1 ; num_of_peaks_count++)
			{
				for(ppg_peak_index = peak_location[num_of_peaks_count] ; ppg_peak_index  < (peak_location[num_of_peaks_count] + 7) ; ppg_peak_index++)
				{
					if((filter_data[ppg_peak_index +1]) > (filter_data[ppg_peak_index]))
					{
						temp_ppg_falling_edge_valley_value = filtered_data[ppg_peak_index];

						for( ;ppg_peak_index  < (peak_location[num_of_peaks_count] + 7) ; ppg_peak_index++)
						{
							if((filter_data[ppg_peak_index +2]) < (filter_data[ppg_peak_index+1]))
							{
								temp_ppg_falling_edge_peak_value = filter_data[ppg_peak_index +1];

								if((fabsf(temp_ppg_falling_edge_peak_value - temp_ppg_falling_edge_valley_value)) > 0.05)
								{
									peak_detection_status = FALSE;
								}
								ppg_peak_index += 2;
								break;
							}
							if(peak_detection_status == FALSE)
							{
								break;
							}
						}
					}
					if(peak_detection_status == FALSE)
					{
						break;
					}
				}
				if(peak_detection_status == FALSE)
				{
					break;
				}
			}
		}

		if(peak_detection_status == TRUE)
		{
			*total_peak_count = peaks_count;
		}

	return peak_detection_status;
}

static void quick_test_clear_buffers(void)
{
	MemSet(raw_data_1,(float)0x00u,sizeof(raw_data_1));
	MemSet(raw_data_2,(float)0x00u,sizeof(raw_data_2));
	MemSet(ir_avg_buff,(int32_t)0x00u,sizeof(ir_avg_buff));
	MemSet(red_avg_buff,(int32_t)0x00u,sizeof(red_avg_buff));
}

/*
* static bool bp_ecg_peak_detection(uint16_t ecg_peak_pos[], uint8_t *num_peaks)
* \brief		Function to detect ecg peaks
*
* \retval      TRUE on successful peak detection
*/
static bool bp_ecg_peak_detection(uint16_t ecg_peak_pos[], uint8_t *num_peaks)
{
	float ecg_peak_value[BP_MAX_NUM_ECG_PEAKS] = {0};
	float temp_filt_max = 0.0;
	uint16_t bp_ecg_index = 0;
	uint16_t bp_ecg_max_index = 0;
	uint8_t ecg_peak_index = 0;
	uint8_t num_ecg_peaks_grtr_pt5 = 0;
	uint8_t num_ecg_peaks_grtr_pt4 = 0;
	bool isMaxValue = 0;
	bool status_peak_detection = FALSE;

	*num_peaks = 0;
	MemSet(ecg_peak_pos,(uint16_t)0x00u,(sizeof(BP_MAX_NUM_ECG_PEAKS)* sizeof(uint16_t)));
	MemSet(ecg_peak_value,(float)0x00u,sizeof(ecg_peak_value));

	for (bp_ecg_index = BP_ECG_TH_RANGE_RISE_SIDE;
			((bp_ecg_index < (BP_NUM_ECG_PPG_SAMPLES - BP_ECG_TH_RANGE_FALL_SIDE)) && (ecg_peak_index < BP_MAX_NUM_ECG_PEAKS));
			bp_ecg_index++)
	{
		if ((filtered_data[bp_ecg_index] >= BP_ECG_PPG_PK_DETECT_TH_0pt4) &&
			(filtered_data[bp_ecg_index] > filtered_data[bp_ecg_index - BP_ECG_TH_RANGE_RISE_SIDE]) &&
			(filtered_data[bp_ecg_index] > filtered_data[bp_ecg_index + BP_ECG_TH_RANGE_FALL_SIDE]) &&
			((filtered_data[bp_ecg_index] - filtered_data[bp_ecg_index - BP_ECG_TH_RANGE_RISE_SIDE]) >= BP_ECG_PPG_PK_DETECT_TH_0pt4) &&
			((filtered_data[bp_ecg_index] - filtered_data[bp_ecg_index + BP_ECG_TH_RANGE_FALL_SIDE]) >= BP_ECG_PPG_PK_DETECT_TH_0pt4))
		{
			isMaxValue = TRUE;

			for (bp_ecg_max_index = (bp_ecg_index - BP_ECG_TH_RANGE_RISE_SIDE); bp_ecg_max_index <= (bp_ecg_index + BP_ECG_TH_RANGE_FALL_SIDE); bp_ecg_max_index++)
			{
				temp_filt_max = filtered_data[bp_ecg_index];

				if (filtered_data[bp_ecg_max_index] > temp_filt_max)
				{
					isMaxValue = FALSE;
					break;
				}
			}

			if (isMaxValue)
			{
				ecg_peak_pos[ecg_peak_index] = bp_ecg_index;
				ecg_peak_value[ecg_peak_index] = temp_filt_max;
				ecg_peak_index++;
				*num_peaks = *num_peaks + 1;
				status_peak_detection = TRUE;
			}
		}
	}


	for(ecg_peak_index = 0; (ecg_peak_index < *num_peaks)&& (status_peak_detection == TRUE) ; ecg_peak_index++)
	{
		if(ecg_peak_value[ecg_peak_index] >= BP_ECG_PPG_PK_DETECT_TH_0pt5)
		{
			num_ecg_peaks_grtr_pt5++;
		}
		else if(ecg_peak_value[ecg_peak_index] >= BP_ECG_PPG_PK_DETECT_TH_0pt4)
		{
			num_ecg_peaks_grtr_pt4++;
		}
		else
		{
			status_peak_detection = FALSE;
			break;
		}
	}

	if(status_peak_detection == TRUE)
	{
		if((*num_peaks >= BP_NUM_MINIMUM_ECG_PEAKS) && (num_ecg_peaks_grtr_pt5 >= NUM_MIN_ECG_PEAKS_0pt5) && (num_ecg_peaks_grtr_pt4 <= (*num_peaks - NUM_MIN_ECG_PEAKS_0pt5)))
		{
			status_peak_detection = TRUE;
		}
		else
		{
			status_peak_detection = FALSE;
		}
	}
	return (status_peak_detection);
}

/*
* static bool bp_ppg_peak_detection(uint16_t ecg_peak_pos[5], uint16_t *ecg_loc_1 ,uint16_t *ecg_loc_2,
* 									uint16_t *ppg_peak_pos)
* \brief		Function to detect ppg peaks
*
* \retval      TRUE on successful peak detection
*/
static bool bp_ppg_peak_detection(uint16_t ecg_peak_pos[5], uint16_t *ecg_loc_1 ,uint16_t *ecg_loc_2,uint16_t *ppg_peak_pos)
{
	float avg_output_ecg_ppg[BP_NUM_ECG_PPG_SAMPLES] = {0};

	float ppg_pk_value[BP_MAX_NUM_ECG_PEAKS -1] = {0};
	float temp_filt_max = 0.0;
	uint16_t ppg_pk_pos[BP_MAX_NUM_ECG_PEAKS -1] = {0};
	uint16_t bp_ppg_index = 0;
	uint16_t bp_ppg_max_index = 0;
	uint16_t bp_ppg_start_pos = 0;
	uint16_t bp_ppg_peak_index = 0;
	uint8_t bp_ecg_peak_index = 0;
	uint8_t num_ecg_peaks_grtr_100 = 0;
	uint8_t num_ppg_peaks_grtr_pt5 = 0;
	uint8_t num_ppg_peaks_grtr_pt4 = 0;
	bool isMaxValue = 0;
	bool status_peak_detection = FALSE;

	avg_ptt_by_hrt = 0.0;

	float temp_ppg_falling_edge_valley_value = -1;
	float temp_ppg_falling_edge_peak_value = -1;

	float heart_rate_time[BP_MAX_NUM_ECG_PEAKS - 2] = { 0.0 };
	float pulse_train_time[BP_MAX_NUM_ECG_PEAKS - 2] = { 0.0 };
	float ptt_by_hrt[BP_MAX_NUM_ECG_PEAKS - 2] = { 0.0 };
	float per_avg_ptt_by_hrt = 0.0;

	*ppg_peak_pos = 0;
	*ecg_loc_1 = 0;
	*ecg_loc_2 = 0;

	MemSet(ppg_pk_value,(float)0x00u,sizeof(ppg_pk_value));
	MemSet(heart_rate_time,(float)0x00u,sizeof(heart_rate_time));
	MemSet(pulse_train_time,(float)0x00u,sizeof(pulse_train_time));
	MemSet(ptt_by_hrt,(float)0x00u,sizeof(ptt_by_hrt));

	MemSet(ppg_pk_pos,(uint16_t)0x00u,sizeof(ppg_pk_pos));


	for (bp_ecg_peak_index = 1; bp_ecg_peak_index < BP_MAX_NUM_ECG_PEAKS; bp_ecg_peak_index++)
	{
		if (ecg_peak_pos[bp_ecg_peak_index] >= BP_PPG_TH_RANGE_RISE_SIDE)
		{
			bp_ppg_start_pos = ecg_peak_pos[bp_ecg_peak_index];
			num_ecg_peaks_grtr_100 = BP_MAX_NUM_ECG_PEAKS - bp_ecg_peak_index;
			break;
		}
	}

	/*--------------peak detection with max threshold 0.2---------------------------------------------*/
	for (bp_ppg_index = bp_ppg_start_pos,bp_ppg_peak_index = 0;
			((bp_ppg_index < (BP_NUM_ECG_PPG_SAMPLES - BP_PPG_TH_RANGE_FALL_SIDE)) && (bp_ppg_index < ecg_peak_pos[bp_ecg_peak_index+1]) &&
			(bp_ecg_peak_index < BP_MAX_NUM_ECG_PEAKS) && (ppg_pk_pos[bp_ppg_peak_index] == 0)  ); bp_ppg_index++)
	{
		if ((filtered_data[bp_ppg_index] >= BP_PPG_PEAK_DETECT_TH) &&
			(filtered_data[bp_ppg_index] > filtered_data[bp_ppg_index - BP_PPG_TH_RANGE_RISE_SIDE]) &&
			(filtered_data[bp_ppg_index] > filtered_data[bp_ppg_index + BP_PPG_TH_RANGE_FALL_SIDE]) &&
			((filtered_data[bp_ppg_index] - filtered_data[bp_ppg_index - BP_PPG_TH_RANGE_RISE_SIDE]) >= BP_PPG_PEAK_DETECT_TH) &&
			((filtered_data[bp_ppg_index] - filtered_data[bp_ppg_index + BP_PPG_TH_RANGE_FALL_SIDE]) >= BP_PPG_PEAK_DETECT_TH))
		{
			isMaxValue = TRUE;

			for (bp_ppg_max_index = (bp_ppg_index - BP_PPG_TH_RANGE_RISE_SIDE);
					bp_ppg_max_index <= (bp_ppg_index + BP_PPG_TH_RANGE_FALL_SIDE); bp_ppg_max_index++)
			{
				temp_filt_max = filtered_data[bp_ppg_index];

				if (filtered_data[bp_ppg_max_index] > temp_filt_max)
				{
					isMaxValue = FALSE;
					break;
				}
			}
			if (isMaxValue)
			{
				ppg_pk_pos[bp_ppg_peak_index] = bp_ppg_index;
				ppg_pk_value[bp_ppg_peak_index] = filtered_data[bp_ppg_index];
				bp_ppg_peak_index++;
				bp_ecg_peak_index++;
				bp_ppg_index = bp_ppg_start_pos = ecg_peak_pos[bp_ecg_peak_index];
				status_peak_detection = TRUE;
			}
		}
	}

	/*-------------- Verify if peak values are within 0.4 ---------------------------------------------*/
	for(bp_ppg_peak_index = 0; (bp_ppg_peak_index < (num_ecg_peaks_grtr_100 - 1))&& (status_peak_detection == TRUE) ; bp_ppg_peak_index++)
	{
		if(ppg_pk_value[bp_ppg_peak_index] >= BP_ECG_PPG_PK_DETECT_TH_0pt5)
		{
			num_ppg_peaks_grtr_pt5++;
		}
		else if(ppg_pk_value[bp_ppg_peak_index] >= BP_ECG_PPG_PK_DETECT_TH_0pt4)
		{
			num_ppg_peaks_grtr_pt4++;
		}
		else
		{
			status_peak_detection = FALSE;
			break;
		}
	}

	if(status_peak_detection == TRUE)
	{
		if((num_ppg_peaks_grtr_pt5 >= NUM_MIN_PPG_PEAKS_0pt5) &&
				(num_ppg_peaks_grtr_pt4 <= ((num_ecg_peaks_grtr_100 - 1) - NUM_MIN_PPG_PEAKS_0pt5)))
		{
			status_peak_detection = TRUE;
		}
		else
		{
			status_peak_detection = FALSE;
		}
	}

	/*-------------- Check if Falling peak to valley is > 0.05 ---------------------------------------------*/
	if(status_peak_detection == TRUE)
	{
	  /* Falling PPG peak validation */
	   for(bp_ppg_index=0; ((bp_ppg_index < (BP_MAX_NUM_ECG_PEAKS - 1)) && (ppg_pk_pos[bp_ppg_index] != 0)); bp_ppg_index++)
	   {
	   		for(bp_ppg_peak_index = ppg_pk_pos[bp_ppg_index]; bp_ppg_peak_index < (ppg_pk_pos[bp_ppg_index] + NBR_PPG_SAMPLES_TO_DETECT); bp_ppg_peak_index++)
			{
				if (filtered_data[bp_ppg_peak_index + 1] > filtered_data[bp_ppg_peak_index])
				{
					temp_ppg_falling_edge_valley_value = filtered_data[bp_ppg_peak_index];

					for ( ; bp_ppg_peak_index < (ppg_pk_pos[bp_ppg_index] + NBR_PPG_SAMPLES_TO_DETECT); bp_ppg_peak_index++)
					{
						if (filtered_data[bp_ppg_peak_index + 2] < filtered_data[bp_ppg_peak_index + 1])
						{
							temp_ppg_falling_edge_peak_value = filtered_data[bp_ppg_peak_index + 1];

							if ((fabsf(temp_ppg_falling_edge_peak_value - temp_ppg_falling_edge_valley_value)) > PPG_FALLING_EDGE_VALLY_TO_PEAK_DIFF)
							{
								status_peak_detection = FALSE;
						    }
							bp_ppg_peak_index +=2;
							break;
						}
						if (status_peak_detection == FALSE)
						{
							break;
						}

					}
				}
				if (status_peak_detection == FALSE)
				{
				    break;
				}
			}
			if (status_peak_detection == FALSE)
			{
				break;
			}
	   }
	}
	//if Falling peak detection fails then 10 point averaging on PPG Signal and and detecting peaks again
	if(status_peak_detection == FALSE)
	{
		MemSet(ppg_pk_pos, 0x00, sizeof(ppg_pk_pos));
		average_10point(filtered_data,avg_output_ecg_ppg);
		status_peak_detection = bp_ppg_peak_detection_2nd_validation(avg_output_ecg_ppg,ecg_peak_pos,ppg_pk_pos);
	}


	/*-------------- Check if Individual PTT/HRT diff is > 25% of Average PTT/HRT ---------------------------------------------*/
	if (status_peak_detection == TRUE)
	{
		for ( bp_ppg_index = 2; bp_ppg_index < BP_MAX_NUM_ECG_PEAKS; bp_ppg_index++)
		{

			heart_rate_time[bp_ppg_index - 2] = ecg_peak_pos[bp_ppg_index] - ecg_peak_pos[bp_ppg_index - 1];
			pulse_train_time[bp_ppg_index - 2] = ppg_pk_pos[bp_ppg_index - 2] - ecg_peak_pos[bp_ppg_index - 1];
			ptt_by_hrt[bp_ppg_index - 2] = pulse_train_time[bp_ppg_index - 2] / heart_rate_time[bp_ppg_index - 2];
		}

		for (bp_ppg_index = 0; bp_ppg_index < BP_MAX_NUM_ECG_PEAKS - 2; bp_ppg_index++)
		{
			avg_ptt_by_hrt += ptt_by_hrt[bp_ppg_index];
		}

		avg_ptt_by_hrt = avg_ptt_by_hrt / 3;

		per_avg_ptt_by_hrt = PERCENTAGE_OF_PTT_BY_HR * avg_ptt_by_hrt;

		if ((fabsf(ptt_by_hrt[1] - ptt_by_hrt[0]) > per_avg_ptt_by_hrt) || (fabsf(ptt_by_hrt[2] - ptt_by_hrt[0]) > per_avg_ptt_by_hrt))
		{
			status_peak_detection = FALSE;

		}
	}

	//PPG peak validation - must be within two ecg peaks

	if (status_peak_detection == TRUE)
	{
		for (bp_ppg_index = 1; bp_ppg_index < (BP_MAX_NUM_ECG_PEAKS-1); bp_ppg_index++)
		{
			if ((ppg_pk_pos[bp_ppg_index - 1] <= ecg_peak_pos[bp_ppg_index]) || (ppg_pk_pos[bp_ppg_index - 1] >= ecg_peak_pos[bp_ppg_index + 1]))
			{
				status_peak_detection = FALSE;

			}
		}
	}


	for (bp_ecg_peak_index = 0; (bp_ecg_peak_index < BP_MAX_NUM_ECG_PEAKS) && (status_peak_detection == TRUE) ; bp_ecg_peak_index++)
	{
		for (bp_ppg_peak_index = 0; bp_ppg_peak_index < (BP_MAX_NUM_ECG_PEAKS - 1); bp_ppg_peak_index++)
		{
			if ((ppg_pk_pos[bp_ppg_peak_index] >= ecg_peak_pos[bp_ecg_peak_index] ) && (ppg_pk_pos[bp_ppg_peak_index] < ecg_peak_pos[bp_ecg_peak_index+1] ))
			{
				*ecg_loc_1 = ecg_peak_pos[bp_ecg_peak_index];
				*ecg_loc_2 = ecg_peak_pos[bp_ecg_peak_index + 1];
				*ppg_peak_pos = ppg_pk_pos[bp_ppg_peak_index];
				break;

			}
			if(ppg_peak_pos)
			{
				status_peak_detection = TRUE;
				break;
			}
		}
	}

	return (status_peak_detection);
}

/*
* void bp_calculation(uint16_t ppgloc,uint16_t ecgloc1,uint16_t ecgloc2,uint16_t *sbp, uint16_t *dbp)
* \brief        Calculate SBP and BP value based on Heart Rate
* \param[in]    ppgloc    	- ppg peak
* \param[in]    ecgloc1    	- ecg location 1
* \param[in]    ecgloc2    	- ecg location 2
*
* \param[out]   sbp    		- systolic bp
* \param[out]   dbp    		- diastolic bp
* \retval       none
*/
static void bp_calculation(uint16_t ppgloc, uint16_t ecgloc1, uint16_t ecgloc2, uint16_t *sbp, uint16_t *dbp)
{
	float Max_SBP_Multi = 0.0;
	float Max_DBP_Multi = 0.0;
	float ptt_by_hrt = 0.0;
	float PTT = 0.0;
	float heart_rate_time = 0.0;
	float sbp_multiply_factor = 0.0;
	float dbp_multiply_factor = 0.0;
    float heart_rate = 0.0;
    uint16_t sbp_dbp_index = 0;

	*sbp = 0;
	*dbp = 0;

	heart_rate_time = ((float)(ecgloc2 - ecgloc1) / 200);
	heart_rate = (SECONDS_60 / heart_rate_time);
	PTT = ((float)(ppgloc - ecgloc1) / 200);


	if(bp_calibration.sub_command == BP_STD_VALUES)
	{
		sbp_multiply_factor = (bp_calibration.sbp_std_val) / ((1 - (PTT / heart_rate)) * (BP_MAX_SBP_THRESHOLD - BP_MIN_SBP_THRESHOLD) * heart_rate);
		dbp_multiply_factor = (bp_calibration.dbp_std_val * 100) / ((1 - (PTT/heart_rate)) * (BP_MAX_DBP_THRESHOLD - BP_MIN_DBP_THRESHOLD) * heart_rate);

		Max_SBP_Multi = sbp_multiply_factor + ((heart_rate - MIN_HEART_RATE_FOR_MULTIPLIER) * 0.00002);
		Max_DBP_Multi = dbp_multiply_factor + ((heart_rate - MIN_HEART_RATE_FOR_MULTIPLIER) * 0.002);

		if((Max_SBP_Multi <= BP_MAX_SBP_MULTIPIER) && (Max_DBP_Multi <= BP_MAX_DBP_MULTIPIER)
						&& (Max_SBP_Multi >= BP_MIN_SBP_MULTIPIER) && (Max_DBP_Multi >= BP_MIN_DBP_MULTIPIER))
		{
			*sbp = bp_calibration.sbp_std_val;
			*dbp = bp_calibration.dbp_std_val;

			// Store mutipliers to flash
			//bp_store_multipliers_to_flash(Max_SBP_Multi,Max_DBP_Multi);

			//Set the calib factors, if the user retests
			bp_calibration.sub_command = BP_CALIBRATION_FACTORS;
			bp_calibration.sbp_multiplier_val = Max_SBP_Multi;
			bp_calibration.dbp_multiplier_val = Max_DBP_Multi;
		}
		else
		{
			*sbp = 0;
			*dbp = 0;
		}
	}
	else if(bp_calibration.sub_command == BP_CALIBRATION_FACTORS)
	{
		Max_SBP_Multi = bp_calibration.sbp_multiplier_val;
		Max_DBP_Multi = bp_calibration.dbp_multiplier_val;

		*sbp = (1 - (PTT/heart_rate)) * (BP_MAX_SBP_THRESHOLD - BP_MIN_SBP_THRESHOLD)*
				(Max_SBP_Multi - ((heart_rate - MIN_HEART_RATE_FOR_MULTIPLIER) * 0.00002)) * heart_rate;


		*dbp = ((1-(PTT/heart_rate))* (BP_MAX_DBP_THRESHOLD - BP_MIN_DBP_THRESHOLD)*
				(Max_DBP_Multi - ((heart_rate - MIN_HEART_RATE_FOR_MULTIPLIER) * 0.002)) * heart_rate)/100;

	}
	else if((bp_calibration.sub_command != BP_STD_VALUES) && (bp_calibration.sub_command != BP_CALIBRATION_FACTORS))
	{

		heart_rate_time = ((ecgloc2 - ecgloc1) * BP_CAL_ODR_FACTOR);
		heart_rate = (uint16_t)((SECONDS_60 * 1000)/(heart_rate_time));

		sbp_dbp_index = (heart_rate) - MIN_HEART_RATE_FOR_MULTIPLIER;
		sbp_multiply_factor = SBP_MUL_FACTOR[sbp_dbp_index];
		dbp_multiply_factor = DBP_MUL_FACTOR[sbp_dbp_index];

		ptt_by_hrt = avg_ptt_by_hrt; 											//(pulse_tran_time/heart_rate_time);

		*sbp = (uint16_t) ( (1 - ptt_by_hrt)* (BP_MAX_SBP_THRESHOLD - BP_MIN_SBP_THRESHOLD)*(sbp_multiply_factor * heart_rate) );
		*dbp = (uint16_t) ( (1 - ptt_by_hrt)* (BP_MAX_DBP_THRESHOLD - BP_MIN_DBP_THRESHOLD) * ((dbp_multiply_factor* heart_rate)/100) );
	}
}

static uint16_t heart_rate_computation(uint16_t peak_loc[])
{
    uint16_t pos_dif = 0.0;
    float t_pos_dif = 0.0;
    float HR_sum = 0.0;
    float HR_avg = 0.0;
    float HR[10] = {0.0};

	for(uint16_t i = 0;i < BP_MAX_NUM_ECG_PEAKS-1;i++)
	{
		pos_dif = peak_loc[i+1] - peak_loc[i];     // difference between two 									//consecutive peak
		t_pos_dif = ((float)pos_dif)/BP_ODR;                                               // ECG_output data rate = 100
		HR[i] = SECONDS_60/t_pos_dif;          //HR calculation, SECONDS_60 = 60
	}

	for(uint16_t i = 0;i < BP_MAX_NUM_ECG_PEAKS-1;i++)
	{
		HR_sum = HR[i] + HR_sum;
	}
	HR_avg = (HR_sum / (BP_MAX_NUM_ECG_PEAKS-1));									// To find average of all the individual HRs calculated

	return (uint16_t)HR_avg;

}


static void average_10point(float input[],float averaged_output[])
{
	float average_data = 0;
	uint16_t samples_count = 0;
	uint8_t average_samples = 0;

	for(samples_count = 0 ; samples_count < (BP_NUM_ECG_PPG_SAMPLES - 10) ; samples_count++)
	{
		average_data = input[samples_count];
		for(average_samples = 0 ; average_samples < 10 ; average_samples++)
		{
			average_data += input[samples_count + average_samples];
		}
		average_data = (average_data/10);

		averaged_output[samples_count] = average_data;
	}
}

static bool bp_ppg_peak_detection_2nd_validation(float output_ECG_PPG[], uint16_t ecg_peak_pos[],uint16_t ppg_peak_pos[])
{

	float temp_filt_max = 0.0;
	uint16_t i = 0;
	uint16_t j = 0;
	uint16_t k = 0;
	uint16_t p = 0;
	uint8_t isMaxValue = 0;
	uint16_t bp_ppg_start_pos = 0;
	uint8_t status_peak_detection = FALSE;
	uint8_t num_ecg_peaks_grtr_100 = 0;

	uint8_t count1 = 0;
	uint8_t count2 = 0;
	uint8_t ppg_th_range_rise_side = 0;
	uint8_t ppg_th_range_fall_side = 0;

	//MemSet((void *)ppg_peak_pos, 0, sizeof(ppg_peak_pos));
	//MemSet((void *)bp_ppg_peak_value, 0, sizeof(bp_ppg_peak_value));

	for (k = 0; k < 5; k++)
	{
		if ((ecg_peak_pos[k + 1] - ecg_peak_pos[k]) <= 50)
		{
			count2++;
			if (count2 >= 3)
			{
				ppg_th_range_rise_side = 25;   //HR >200
				ppg_th_range_fall_side = 20;

				break;
			}
		}
		else if ((ecg_peak_pos[k + 1] - ecg_peak_pos[k]) <= 104)
		{
			count1++;
			if (count1 >= 3)                 // To check HR must be continuous
			{
				ppg_th_range_rise_side = 50;                                ///HR 115
				ppg_th_range_fall_side = BP_PPG_TH_RANGE_FALL_SIDE;

				break;
			}
		}
		else
		{
			ppg_th_range_rise_side = 100;                                       // Normal HR
			ppg_th_range_fall_side = BP_PPG_TH_RANGE_FALL_SIDE;
		}

	}

	for (k = 1; k < 5; k++)
	{
		if (ecg_peak_pos[k] >= BP_PPG_TH_RANGE_RISE_SIDE)
		{
			num_ecg_peaks_grtr_100++;
		}
	}

	for (k = 1; k < 5; k++)
	{
		if (ecg_peak_pos[k] >= BP_PPG_TH_RANGE_RISE_SIDE)
		{
			bp_ppg_start_pos = ecg_peak_pos[k];
			break;
		}
	}

	for (i = bp_ppg_start_pos, j = 0; ((i < 1159) && (i < ecg_peak_pos[k + 1]) && (ppg_peak_pos[j] == 0) && (k < BP_MAX_NUM_ECG_PEAKS)); i++)
	{
		if ((output_ECG_PPG[i] > output_ECG_PPG[i - ppg_th_range_rise_side]) &&
			(output_ECG_PPG[i] > output_ECG_PPG[i + ppg_th_range_fall_side]) )//&&
			//((output_ECG_PPG[i] - output_ECG_PPG[i - ppg_th_range_rise_side]) >= BP_PPG_RISING_TH_0pt2) &&
			//((output_ECG_PPG[i] - output_ECG_PPG[i + ppg_th_range_fall_side]) >= BP_PPG_FALLING_TH_0pt2)) //(output_ECG_PPG[i] >= BP_PPG_PEAK_DETECT_TH_0pt4) &&
		{
			isMaxValue = 1;
			for (p = (i - ppg_th_range_rise_side); p <= (i + ppg_th_range_fall_side); p++)      //ppg window move
			{
				temp_filt_max = output_ECG_PPG[i];

				if (output_ECG_PPG[p] > temp_filt_max)
				{
					isMaxValue = 0;
					break;
				}
			}
			if (isMaxValue)
			{
				ppg_peak_pos[j] = i;
				//bp_ppg_peak_value[j] = output_ECG_PPG[i];
				status_peak_detection = TRUE;
				j++;
				k++;
				i = bp_ppg_start_pos = ecg_peak_pos[k];
				//*ppg_err_code = BP_NO_ERROR;
			}
		}
	}
	return (status_peak_detection);
}


bool Run_Multi_Vital(void)
{
	API_DISP_Display_Screen(DISP_ECG_12_LEAD_SCREEN);

	Enable_Power_Supply();

	if(1)
	//if(Lead12_LeadOff_Detect() == FALSE)
	{
		if(Run_Quick_Vital()==TRUE)
		{
			//if(Selected_PID_type != GUEST_PID)
				Store_QuickTest2_Data_To_Flash();

			Lead12_Test();
		}
	}

	else
	{
		API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);
		API_Disp_Quick_test_screen(DISP_12LEAD_CABLE_NOT_CONNECTED_PROPERLY);
	}

	Disable_Power_Supply();
	return true;
}

bool Lead12_LeadOff_Detect(void)
{
	uint8_t count=0;

	bool leadOffStatus = true;

	API_ECG_Chip_Reset();

	if(API_ECG_Init())
	{
		for(uint8_t vlead=1;vlead<=6;vlead++)
		{
			Select_Vlead(vlead);
			if(API_ECG_Lead_OFF_Detect(LEAD12))
			{
				count++;
				printf("\nError Lead Off : vlead%d",vlead);
			}
		}
	}

	if(count == 0) leadOffStatus = false;

	return leadOffStatus;
}


void Enable_Power_Supply(void)
{
	API_IO_Exp_Power_Control(EN_VLED,HIGH);
	API_IO_Exp_Power_Control(EN_ANALOG,HIGH);
	API_IO_Exp_Power_Control(EN_IR,HIGH);
	Delay_ms(500);
}


void Disable_Power_Supply(void)
{
	API_IO_Exp_Power_Control(EN_VLED,LOW);
	API_IO_Exp_Power_Control(EN_ANALOG,LOW);
	API_IO_Exp_Power_Control(EN_IR,LOW);
	Delay_ms(500);

}


bool Check_PPG_Data_Quality(void)
{
	uint16_t peakLocations[15]={0};
	uint8_t total_peak_count;

   bool status = false;

   status = spo2_peak_detection(( float *)SPO2_PPG_IR_BUFF, peakLocations,&total_peak_count,600);

   if(status)
   {
	   printf("\n PPG Peak detection success\n");
   }
   return status;

}
