/*
 * ECG_12_Lead.c
 *
 *  Created on: 06-Jan-2023
 *      Author: madhusudhan
 */

#include "ECG_12_Lead.h"
#include <stdbool.h>
#include <stdint.h>
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
#include "Quick_Test.h"

bool Lead12_Data_Capture(void);


bool Lead12_Test(void)
{
	uint16_t result[4] = {0};

	printf("\n12 Lead ECG Test Started");

	Is_time_displayed = TRUE;
	API_DISP_Toggle_Date_Time();

	if((Selected_PID_type == VALID_PID) || (Selected_PID_type == GUEST_PID))
	{

			if(API_Flash_Org_Check_For_Memory_Free())
			{
                // Disp quick test 1 screen
				API_IO_Exp_Power_Control(EN_VLED,LOW);
				API_IO_Exp_Power_Control(EN_ANALOG,LOW);
				API_IO_Exp_Power_Control(EN_IR,LOW);
				API_Buzzer_Sound(SHORT_BEEP);

				API_DISP_Display_Screen(DISP_ECG_12_LEAD_SCREEN);
				//Delay_ms(2000);

				printf("\nEnteriing into 12 Lead ECG Init");


				if(API_ECG_Init())
				{
					printf("\nECG L2 Init Done!");

					API_DISP_Display_Screen(DISP_TEST_IN_PROGRESS);

					Lead12_Data_Capture();

					API_Disp_Quick_Test_Result(result);
					Delay_ms(2000);
					// TODO; retry when raw data is not good
					//Filter;
				}
				else
				{
					printf("\n12 Lead ECG Init Fail");
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

	 API_IO_Exp_Power_Control(EN_VLED,LOW);
	 API_IO_Exp_Power_Control(EN_ANALOG,LOW);
	 API_IO_Exp_Power_Control(EN_IR,LOW);

	 Delay_ms(2000);
  	 API_Buzzer_Sound(SHORT_BEEP);

  	 printf("\nTest completed.");
	return true;
}


bool Lead12_Data_Capture(void)
{
	assert(DATA_BUFFER3_LENGTH > (600*8*4)); // TotalSamplesPerLead * NumberOFLeadData * SizeOfEachSample

	RECORD_OPS_STATUS flash_write_status;
	uint32_t offfset = 0;

	API_Update_Record_Header(ECG_12_LEAD,&record_header);

	MemCpy(BT_flash_buffer,&record_header,REC_HEADER_LEN);
	offfset = REC_HEADER_LEN;

	Select_Vlead(LEAD6);

	API_Disp_Lead_Count(6);

	Capture_PPG_ECG_Data(CAPTURE_ECG_L1_AND_L2,true);

	if( API_ECG_Reginit_12Lead() != ECG_NO_ERROR)
		{
		   printf("\n12 Lead Reg init fail");
		   Catch_RunTime_Error(ECG_LEAD12_INIT_FAIL);

           return 1;
		}
	printf("\n12Lead ECG Register Init done");

	MemCpy((void*)BT_flash_buffer+offfset,(void*)ECG_Lead1_buff,(600*4));
	offfset +=(600*4);

	MemCpy((void*)BT_flash_buffer+offfset,(void*)ECG_Lead2_buff,(600*4));
	offfset += (600*4);

	printf("\n12 Lead ECG Data capturing");

	for(int vlead=0;vlead<6;vlead++)
	{
		MemSet(ECG_Lead1_buff,0,sizeof(ECG_Lead1_buff)); // In this block making use of ECG_Lead1_buff to capture V lead data.
		API_Disp_Lead_Count(7+vlead);
		Select_Vlead(vlead);

		for(int sample=0;sample<600;sample++)
		{
			API_ECG_Capture_Samples_VLead(ECG_Lead1_buff+sample,1); // Here ECG_Lead1_buff is used to store V1 to V6 Lead data, Single lead data at a time.
		}

		MemCpy((void*)BT_flash_buffer+offfset,(void*)ECG_Lead1_buff,(600*4));
		offfset += (600*4);

		printf("\nECG L%d Data",vlead+7);

		for(int i=0;i<600;i++)
		{
		   printf("\n%f",ECG_Lead1_buff[i]);
		}
	}

	printf("\n12 Lead Total Samples Captured = %ld",offfset);

	flash_write_status = API_Flash_Write_Record(ECG_12_LEAD,(void *)BT_flash_buffer);

	if(flash_write_status != WRITE_RECORDS_SUCCESS) Catch_RunTime_Error(LEAD12_ECG_DATA_STORE_TO_FLASH_FAIL);

	IsValidRecordsInFlash = true;

	printf("\nTotal 12 Lead Records = %ld", get_records_count(ECG_12_LEAD));

	return true;
}

void Select_Vlead(VLEAD_TYPE_t vlead_type)
{

   switch(vlead_type)
	 {

	   case LEAD6:
	   {
			API_IO_Exp2_P0_write_pin(ECG12_A0, LOW);
			API_IO_Exp2_P0_write_pin(ECG12_A1, LOW);
			API_IO_Exp2_P0_write_pin(ECG12_A2, LOW);

			break;
	   }
		   case VLEAD1:
		   {
			   API_IO_Exp2_P0_write_pin(ECG12_A0, HIGH);
			   API_IO_Exp2_P0_write_pin(ECG12_A1, LOW);
			   API_IO_Exp2_P0_write_pin(ECG12_A2, LOW);
			   break;
		   }
		   case VLEAD2:
		   {
			   API_IO_Exp2_P0_write_pin(ECG12_A0, LOW);
			   API_IO_Exp2_P0_write_pin(ECG12_A1, HIGH);
			   API_IO_Exp2_P0_write_pin(ECG12_A2, LOW);
			   break;
		   }
		   case VLEAD3:
		   {
				API_IO_Exp2_P0_write_pin(ECG12_A0, HIGH);
				API_IO_Exp2_P0_write_pin(ECG12_A1, HIGH);
				API_IO_Exp2_P0_write_pin(ECG12_A2, LOW);
			   break;
		   }
		   case VLEAD4:
		   {
				API_IO_Exp2_P0_write_pin(ECG12_A0, LOW);
				API_IO_Exp2_P0_write_pin(ECG12_A1, LOW);
				API_IO_Exp2_P0_write_pin(ECG12_A2, HIGH);
			   break;
		   }
		   case VLEAD5:
		   {
				API_IO_Exp2_P0_write_pin(ECG12_A0, HIGH);
				API_IO_Exp2_P0_write_pin(ECG12_A1, LOW);
				API_IO_Exp2_P0_write_pin(ECG12_A2, HIGH);
			   break;
		   }
		   case VLEAD6:
		   {
				API_IO_Exp2_P0_write_pin(ECG12_A0, LOW);
				API_IO_Exp2_P0_write_pin(ECG12_A1, HIGH);
				API_IO_Exp2_P0_write_pin(ECG12_A2, HIGH);
			   break;
		   }

		   default :break;
	  }
}
