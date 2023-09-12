/*
 * push_button.c
 *
 *  Created on: 22-Apr-2022
 *      Author: pooja
 */
#include "push_button.h"
#include "driver/gpio.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "API_timer.h"
#include "Hardware.h"
#include "Error_Handling.h"
#include "driver/rtc_io.h"
#include "esp_attr.h"

#define WAKEUP_SHORT_PRESS_TIME_START   100 // in millisecond
#define WAKEUP_SHORT_PRESS_TIME_END     1000

#define WAKEUP_MEDIUM_PRESS_TIME_START  3000
#define WAKEUP_MEDIUM_PRESS_TIME_END    7000

#define WAKEUP_LONG_PRESS_TIME_START    10000
#define WAKEUP_LONG_PRESS_TIME_END      17000

#define WAKEUP_SOFT_DELAY_COUNT_10MS  (1100*10)

#define PUSH_BTN_INPUT_PIN_SEL  ( 1ULL<<PUSH_BTN_PIN )

uint32_t gpio_arg_val = 0x00;

bool Did_Push_Button_Pressed;

uint32_t BTN_timing;

/****** Only testing ********************************/
/****************************************************/

bool Data_sync_in_progress = false;
bool Is_device_upgrading = false;

uint32_t btn_press_duration;
bool is_btn_press;

static uint8_t btn_duration=0;

 static uint8_t edge_cnt = 0;

static  volatile bool  is_wakeup_button_pressd;

static void btn_measure_time_duration();

static uint32_t Scan_Btn_Press_Duration(void);

static void IRAM_ATTR PushBtnHandler(void* arg)
{
	static uint32_t time_elapsed = 0;
	static bool firstTime = false;

    uint32_t gpio_num = (uint32_t) arg;
    gpio_arg_val= gpio_num;
    if(gpio_arg_val == PUSH_BTN_PIN)
    {
    	is_wakeup_button_pressd = true;
    	Did_Push_Button_Pressed = true;
	/*
		if(firstTime)
		{
			btn_measure_time_duration();
		}

		elsegpio_get_level
		{
			firstTime = true;
		}*/

    }
}

static void btn_measure_time_duration()
{

	edge_cnt++;

	if(edge_cnt == 1)
	{
	     BTN_timing =1;
	}

	if(edge_cnt == 2)
		{
	    	btn_press_duration = BTN_timing;
		     BTN_timing = 0;
		     edge_cnt =0;
		     Did_Push_Button_Pressed = true;
		}


}

uint32_t API_Push_Btn_Debounce_check(uint32_t time_elapsed)
{
		if(time_elapsed < 200)
		{
			btn_duration = 0;
		}

		else if((time_elapsed >= 200) && (time_elapsed<=2000) )
		{
			btn_duration = 1;
		}

		else if((time_elapsed >= 2000) && (time_elapsed<=10000))
		{
			btn_duration = 2;
		}

		else if(time_elapsed >= 10000 )//10 sec
		{
			btn_duration = 3;
		}

	//	printf("\ntime_elapsed=%d",time_elapsed);

	return  btn_duration;
}

uint8_t API_Push_Btn_Get_Buttton_Press(void)
{
	uint8_t btn_press = 0;
	if(is_wakeup_button_pressd)
	{
		btn_press = API_Push_Btn_Debounce_check(Scan_Btn_Press_Duration());
		is_wakeup_button_pressd = false;
	}

	return btn_press;
}

uint32_t API_Push_Btn_Get_hold_time()
{
	uint8_t btn_duration=0;

	if(Did_Push_Button_Pressed)
	{
		printf("\nis_wakeup_button_pressd=%d  btn_press_duration=%ld",is_wakeup_button_pressd,btn_press_duration);

		if((btn_press_duration >= 100) && (btn_press_duration<=1000) )
		{
			btn_duration = 1;
			btn_press_duration =0;
		}

		else if((btn_press_duration >= 1001) && (btn_press_duration<=3000) )
			{
				btn_duration = 2;
				btn_press_duration =0;
			}

		else if(btn_press_duration >= 10000 )//10 sec
			{
				btn_duration = 3;
				btn_press_duration =0;
			}

		Did_Push_Button_Pressed = false;
	}
	return  btn_duration;
}

bool API_Check_Test_Exit(void)
{
	bool status = false;

   if(API_Push_Btn_Get_hold_time() ==1)
   {
	   status = true;
   }

   return status;
}

gpio_config_t io_conf;


void API_Push_Btn_init(void)
{
	esp_err_t error;


    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = PUSH_BTN_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    error = gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    error |= gpio_set_intr_type(PUSH_BTN_PIN, GPIO_INTR_NEGEDGE);

    //install gpio isr service
    error |= gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    error |= gpio_isr_handler_add(PUSH_BTN_PIN, PushBtnHandler, (void*) PUSH_BTN_PIN);

   // error |= gpio_intr_enable(PUSH_BTN_PIN);


    if(error != ESP_OK) Catch_RunTime_Error(PUSH_BTN_INIT_FAIL);

}

void API_Reset_Push_Button_State(void)
{
	edge_cnt = 0;
	BTN_timing = 0;
	Did_Push_Button_Pressed = false;
}

static uint32_t Scan_Btn_Press_Duration(void)
{
	 int vol=0;
	 uint32_t time_elapsed = 0;

	while(1)
	{

		vol = gpio_get_level(PUSH_BTN_PIN);

		Delay_ms(10);

		time_elapsed++;

		if(vol)
		{
			return (time_elapsed*10);
		}
	}

}

void PowerDownAllGpios(void)
{
	for(int i=0;i<40;i++)
	{
		if(i!=24)
		{
		 rtc_gpio_init(i);
		 rtc_gpio_pullup_dis(i);
		 rtc_gpio_set_drive_capability(i, GPIO_DRIVE_CAP_0);
		 rtc_gpio_set_level(i,0);
		}
	}
	rtc_gpio_force_hold_dis_all();

}
