#include "MainFlow.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdbool.h"
#include "esp_ota_ops.h"
#include "max86150.h"
#include "API_Display.h"
#include "API_adc.h"
#include "API_IO_Exp.h"
#include "API_timer.h"
#include "rtc.h"
#include "API_Battery_monitor.h"
#include "API_Flash.h"
#include "push_button.h"
#include "API_ADS.h"
#include "Quick_Test.h"
#include "bluetooth.h"
#include "OTA_Upgrade.h"
#include "API_Bluetooth.h"
#include "Battery_management.h"
#include "ProjectConfiguration.h"
#include "Hardware.h"
#include "esp_sleep.h"
#include "API_utility.h"

static void Interfaces_init(void);
void POR_Init(void);
void Manage_Device_Sleep(void);

void TestNbfsTests_PerBatteryFull(void);
void ExecuteComplianceSequence(void);
void HandleDataSync(void);

void TestFlashStorage(void);

 void Application_Run(void)
{
	static SELECTED_TEST_t state;

	printf("\nSystem turning on..");

	/*API_IO_Exp_PowerOnReset_Configuration();

	API_Battery_monitor_Init();

	if(IsUSB_Charger_Connected())
	{
        if(API_ADC_Read_Battery_Voltage() < (FUEL_GUAGE_MIN_VOLTAGE+0.4))
        {
          printf("\nLow Battery Voltage.. Device Shutting Down.");
          EnterSleepMode(SYSTEM_DEEP_SLEEP);
        }
	}*/

	API_TIMER_Run_1MS_Timer();

	API_IO_Exp_init();

    Interfaces_init();


	if(API_Flash_Initialize_Data_pointers() == RECORDS_UPDATE_FAILED)
	   {
		   printf("\n Fail: API_Flash_Initialize_Data_pointers");
	   }

		/*** Testing code, need to be removed during the Release */

		/**************************************************************/
	//	TestFlashStorage();

	//	while(1){}
		/**************************************************************/


	  API_DISP_SenseSemi_Logo(STATIC_IMAGE);
	  API_DISP_Firmware_Version();

	/* Delay Provided for the User to read the Firmware version */
	Delay_ms(3000);
	POR_Init();

	/** Testing */
	/***************************************************/

	Selected_PID_type = VALID_PID;

	/***************************************************/
		char datetime[18]; // Assuming you want to store the result in a char array
	    const char *customFormat = "%Y-%m-%d %H:%M:%S"; // Example date-time format


	while(1)
	{
		if(flash_data.sys_mode == DEVICE_ACTIVE_MODE)
		{
			    if(Detect_low_battery_display_notification()==false)
				{
			    	  state = API_Disp_Select_PID_Screen();
			    	//state = VIEW_SCREEN;

			    	    if(state == VIEW_SCREEN)
			    	    {
			    	    	state = API_Display_View_Screen();
			    	    	//API_RTC_Update_Date_Time(8, 8, 2023, 16, 24, 17);
							API_RTC_Get_Date_Time(datetime, customFormat);
							printf("Formatted Date and Time: %s\n", datetime);
			    	    }

						if(state == DATA_SYNC)
						{
							HandleDataSync();
							state = NO_TEST;
						}

						 if((state == QUICK_VITALS) || (state == MULTI_VITALS))
						 {
							   if(API_Check_USB_Charger_Connection_Display_Notification())
								{
								   state = 0;//clear the selected test option if the charger is connected.
								}
						 }


						if(state)
						{
							API_TIMER_Kill_Timer(USER_INACTIVE_TIMEOUT);
						}

						switch(state)
						{
							case QUICK_VITALS:{
								Is_Test_In_Progress = true;
								Run_Quick_Vital();
								Is_Test_In_Progress = false;
								break;
							}

							case MULTI_VITALS:{
								Is_Test_In_Progress = true;
								Run_Multi_Vital();
								Is_Test_In_Progress = false;

								break;
							}

							case TEST_EXIT:{
								flash_data.sys_mode = DEVICE_HIBERNET_MODE;
								Is_Test_In_Progress = false;

								break;
							}

							default : break;
						}

						if(state)
							{
								API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
							}

						if (is_firmware_data_available())
						{
							API_TIMER_Kill_Timer(USER_INACTIVE_TIMEOUT);
							Firmware_upgrade();
						}

					}

			    else
			    {
			    	flash_data.sys_mode = DEVICE_HIBERNET_MODE;
			    }
		}

		if(flash_data.sys_mode == DEVICE_SLEEP_MODE)
		{
			Manage_Device_Sleep();
		}

		if(flash_data.sys_mode == DEVICE_HIBERNET_MODE)
				{
					EnterSleepMode(SYSTEM_DEEP_SLEEP);

				}
	}

}

static void Interfaces_init(void)
{
	/**************** Flash init *********************************************/
    API_FLASH_init();

	/*********** Display ********************************************************/
	API_IO_Exp_Reset();

	Power_Up_All_Modules();
	API_Display_interface_init();
	API_DISP_Clear_Full_Screen_3_Wire(WHITE);

	/*********** MAX86150 ********************************************************/
	API_MAX86150_I2C_Init();

	/*********** Battery Monitor  ***********************************************/
	API_Battery_monitor_Init();

	/****************** RTC init  ***********************************************/
	API_RTC_Update_Date_Time(0,0,0,0,0,0);

	/*********** Push Button ****************************************************/
	API_Push_Btn_init();

	/**************** IR ADC init ***********************************************/
	API_IR_ADC_Init();


	/**************** BLE init ***********************************************/
	API_BLE_Init();

}


void POR_Init(void)
{
	API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);

	API_Select_PID();

	memset(PatientID.pid,'\0',sizeof(PatientID.pid));

	API_Disp_Reset_Screen();
	API_DISP_Toggle_Date_Time();
	API_Disp_BT_Icon(WHITE);
	flash_data.sys_mode = DEVICE_ACTIVE_MODE;
	Did_Push_Button_Pressed = false;
	Selected_PID_type =PID_NOT_SELECTED;
	flash_data.device_config.registration_config = HOSPITAL;

	API_Reset_Push_Button_State();
	Selected_PID_type = PID_NOT_SELECTED;
	Is_time_displayed = TRUE;
	 BT_ongoing_session = FALSE;
	 Is_Test_In_Progress = FALSE;
	API_TIMER_Register_Timer(DATE_TIME_FLIP_TIME);
	API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
	IsValidRecordsInFlash = true;

	Disable_Power_Supply();

}

void Manage_Device_Sleep(void)
{
	Delay_ms(3000);
	API_DISP_Clear_Screen_Fast(WHITE);

	//API_DISP_Clear_Full_Screen_3_Wire(WHITE);

	EnterSleepMode(SYSTEM_SLEEP);


	API_TIMER_Register_Timer(DEEP_SLEEP_TIMEOUT);

	Did_Push_Button_Pressed = false;


	while(1)
	{
		if(Did_Push_Button_Pressed)
		{
			if(API_Push_Btn_Get_Buttton_Press())
			 {
				POR_Init();

				break;
			 }
		}

		if(API_TIMER_Get_Timeout_Flag(DEEP_SLEEP_TIMEOUT))
		{
			EnterSleepMode(SYSTEM_DEEP_SLEEP);
		}
	}

}



void TestNbfsTests_PerBatteryFull(void)
{/*
	static uint32_t nbf_tests_conducted = 0;

    while(1)
    {
    	QUICK_Test1();
    	QUICK_Test2();
    	Lead12_Test();
    	API_Flash_Initialize_Data_pointers();
    	nbf_tests_conducted++;

    	printf("\nnbf_tests_conducted = %d",nbf_tests_conducted);

    	if(Detect_low_battery_display_notification())
    	{
    		printf("\nTotal Tests Completed = %d",nbf_tests_conducted);
    		printf("\nEnterring Into Sleep Mode");
    		Manage_Device_Sleep();
    	}

    }
*/}

void ExecuteComplianceSequence(void)
{
	uint8_t btn_press=0;
	API_ECG_Init();
	API_MAX86150_Setup();
	API_ECG_Reginit_12Lead();
	API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);

	   while(1)
	   {
		   btn_press = API_Push_Btn_Get_Buttton_Press();

		   if((API_TIMER_Get_Timeout_Flag(USER_INACTIVE_TIMEOUT) == true) || btn_press)
		   		{
		   			EnterSleepMode(SYSTEM_DEEP_SLEEP);
		   		}
	   }
}

void HandleDataSync(void)
{
	uint8_t btn_press;

	API_DISP_Display_Screen(DISP_DATA_SYNC_IN_PROGRESS);
	API_Disp_Display_Exit_Bottom_Section();

	if(Detect_low_battery_display_notification()==false)
	{
		while(1)
		{
			btn_press = API_Push_Btn_Get_Buttton_Press();

			if(API_TIMER_Get_Timeout_Flag(DATA_SYNC_TIMEOUT)) // call get_time_out function
			{
				BT_Sync_Timeout_Init_State();
				API_DISP_Display_Screen(DISP_DATA_SYNC_FAIL);
				Delay_ms(2000);
				 EnterSleepMode(SYSTEM_DEEP_SLEEP);
			}

			if(IsValidRecordsInFlash == false)// No records in flash
			{
				API_DISP_Display_Screen(DISP_DATA_SYNC_COMPLETED);
				Delay_ms(2000);
				break;
			}

			if((btn_press == 1) || ((btn_press == 2)))
			{
				API_DISP_Display_Screen(DISP_DATA_SYNC_FAIL);
				Delay_ms(2000);
				break;
			}
		}
	}
	else
	{
		API_DISP_Display_Screen(DISP_DATA_SYNC_FAIL);
		Delay_ms(2000);
		EnterSleepMode(SYSTEM_DEEP_SLEEP);
	}

}

#ifdef TEST_FLASH
uint8_t DstBuffer[DATA_BUFFER3_LENGTH];

void TestFlashStorage(void)
{
	uint8_t totalRecordsToValidate = 10U;

	RECORD_POINTER_t records_pointer;

	MemSet(BT_flash_buffer,0,sizeof(BT_flash_buffer));
	MemSet(DstBuffer,0,sizeof(DstBuffer));

	for(int i=0;i<DATA_BUFFER3_LENGTH;i++)
	{
		BT_flash_buffer[i] = i%0xFF;
	}

	printf("\n Flash storage validation, Please wait........\n");

   for(int i=0;i<totalRecordsToValidate;i++)
   {
	 API_Flash_Write_Record(SPO2,(uint8_t*) BT_flash_buffer);
	 API_Flash_Read_Record(SPO2, (uint8_t*)DstBuffer);

	 get_record_pointer_details (SPO2, &records_pointer);

	 if(memcmp(BT_flash_buffer,DstBuffer,records_pointer.one_record_len) !=0)
	 {
		 printf("\nSPO2 storage fail");
	 }
	 else
	 {
		 printf("\nSPO2 storage pass");
	 }
   }

   for(int i=0;i<totalRecordsToValidate;i++)
     {
		 API_Flash_Write_Record(BP1,(uint8_t*) BT_flash_buffer);
		 API_Flash_Read_Record(BP1, (uint8_t*)DstBuffer);

		 get_record_pointer_details (BP1, &records_pointer);

		 if(memcmp(BT_flash_buffer,DstBuffer,records_pointer.one_record_len) !=0)
		 {
			 printf("\nBP1 storage fail");
		 }
		 else
			 {
				 printf("\nBP1 storage pass");
			 }
     }

   for(int i=0;i<totalRecordsToValidate;i++)
     {
   		 API_Flash_Write_Record(BP2,(uint8_t*) BT_flash_buffer);
   		 API_Flash_Read_Record(BP2, (uint8_t*)DstBuffer);

   		 get_record_pointer_details (BP2, &records_pointer);

   		 if(memcmp(BT_flash_buffer,DstBuffer,records_pointer.one_record_len) !=0)
   		 {
   			 printf("\nBP2 storage fail");
   		 }
   		 else
   			 {
   				 printf("\nBP2 storage pass");
   			 }
     }

   for(int i=0;i<totalRecordsToValidate;i++)
       {
     		 API_Flash_Write_Record(ECG_1_Lead,(uint8_t*) BT_flash_buffer);
     		 API_Flash_Read_Record(ECG_1_Lead, (uint8_t*)DstBuffer);

     		 get_record_pointer_details (ECG_1_Lead, &records_pointer);

     		 if(memcmp(BT_flash_buffer,DstBuffer,records_pointer.one_record_len) !=0)
     		 {
     			 printf("\ECG_1_Lead storage fail");
     		 }
     		 else
     			 {
     				 printf("\nECG_1_LEAD storage pass");
     			 }
       }

   for(int i=0;i<totalRecordsToValidate;i++)
        {
      		 API_Flash_Write_Record(ECG_6_Lead,(uint8_t*) BT_flash_buffer);
      		 API_Flash_Read_Record(ECG_6_Lead, (uint8_t*)DstBuffer);

      		 get_record_pointer_details (ECG_6_Lead, &records_pointer);

      		 if(memcmp(BT_flash_buffer,DstBuffer,records_pointer.one_record_len) !=0)
      		 {
      			 printf("\nECG_6_Lead storage fail");
      		 }
      		 else
      			 {
      				 printf("\nECG_6_Lead storage pass");
      			 }
        }

   for(int i=0;i<totalRecordsToValidate;i++)
         {
       		 API_Flash_Write_Record(ECG_12_LEAD,(uint8_t*) BT_flash_buffer);
       		 API_Flash_Read_Record(ECG_12_LEAD, (uint8_t*)DstBuffer);

       		 get_record_pointer_details (ECG_12_LEAD, &records_pointer);

       		 if(memcmp(BT_flash_buffer,DstBuffer,records_pointer.one_record_len) !=0)
       		 {
       			 printf("\nECG_12_LEAD storage fail");
       		 }
       		 else
			 {
				 printf("\nECG_12_Lead storage pass");
			 }
         }

   for(int i=0;i<totalRecordsToValidate;i++)
           {
         		 API_Flash_Write_Record(SENSE_TEMP,(uint8_t*) BT_flash_buffer);
         		 API_Flash_Read_Record(SENSE_TEMP, (uint8_t*)DstBuffer);

         		 get_record_pointer_details (SENSE_TEMP, &records_pointer);

         		 if(memcmp(BT_flash_buffer,DstBuffer,records_pointer.one_record_len) !=0)
         		 {
         			 printf("\nSENSE_TEMP storage fail");
         		 }
         		 else
				 {
					 printf("\nSENSE_TEMP storage pass");
				 }
           }

}

#endif
