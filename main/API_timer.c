#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "API_timer.h"

/*----------All time-out flags defined here -------------*/
/*********************************************************/

static bool time_out_1sec  = false;
static bool time_out_2sec  = false;
static bool time_out_3sec  = false;
static bool time_out_5sec  = false;
static bool time_out_6sec  = false;
static bool time_out_7sec  = false;
static bool time_out_10sec = false;
static bool time_out_15sec = false;
static bool time_out_30sec = false;
static bool time_out_1min  = false;
static bool time_out_3min  = false;
static bool time_out_5min  = false;
static bool time_out_10min = false;
static bool time_out_30min = false;

/*********************************************************/


static uint32_t top_value_1sec   = 0x00;
static uint32_t top_value_2sec   = 0x00;
static uint32_t top_value_3sec   = 0x00;
static uint32_t top_value_5sec  = 0x00;
static uint32_t top_value_6sec  = 0x00;
static uint32_t top_value_7sec  = 0x00;
static uint32_t top_value_10sec = 0x00;
static uint32_t top_value_15sec = 0x00;
static uint32_t top_value_30sec = 0x00;
static uint32_t top_value_1min  = 0x00;
static uint32_t top_value_3min  = 0x00;
static uint32_t top_value_5min  = 0x00;
static uint32_t top_value_10min = 0x00;
static uint32_t top_value_30min = 0x00;

uint32_t Delay_time_out;
esp_timer_handle_t periodic_timer;

static void periodic_timer_callback(void* arg)
{
	if(BTN_timing >= 1)
	{
		BTN_timing++;
	}

	if(Delay_time_out) Delay_time_out--;


    if(top_value_1sec)
	{
    	top_value_1sec--;

	    if(top_value_1sec == 0x00) time_out_1sec = true;
	}
    if(top_value_2sec)
	{
		top_value_2sec--;

	  if(top_value_2sec == 0x00) time_out_2sec = true;
	}
    if(top_value_3sec)
    	{
    		top_value_3sec--;

    	  if(top_value_3sec == 0x00) time_out_3sec = true;
    	}

    if(top_value_5sec)
	{
    	top_value_5sec--;

	  if(top_value_5sec == 0x00) time_out_5sec = true;
	}

    if(top_value_6sec)
    	{
        	top_value_6sec--;

    	  if(top_value_6sec == 0x00) time_out_6sec = true;
    	}
    if(top_value_7sec)
        	{
            	top_value_7sec--;

        	  if(top_value_7sec == 0x00)
        		  {
        		  time_out_7sec = true;
        		  }
        	}

    if(top_value_10sec)
   	{
       	top_value_10sec--;

   	  if(top_value_10sec == 0x00) time_out_10sec = true;
   	}

    if(top_value_15sec)
	{
		top_value_15sec--;

	  if(top_value_15sec == 0x00) time_out_15sec = true;
	}

    if(top_value_30sec)
	{
		top_value_30sec--;

	  if(top_value_30sec == 0x00) time_out_30sec = true;
	}

    if(top_value_1min)
	{
    	top_value_1min--;

	  if(top_value_1min == 0x00) time_out_1min = true;
	}

    if(top_value_3min)
   	{
       	top_value_3min--;

   	  if(top_value_3min == 0x00) time_out_3min = true;
   	}

    if(top_value_5min)
	{
    	top_value_5min--;

	  if(top_value_5min == 0x00) time_out_5min = true;
	}

    if(top_value_10min)
	{
    	top_value_10min--;

	  if(top_value_10min == 0x00) time_out_10min = true;
	}

    if(top_value_30min)
   	{
       	top_value_30min--;

   	  if(top_value_30min == 0x00) time_out_30min = true;
   	}
}


void API_TIMER_Register_Timer(TIMER_t timer_type)
{
  switch(timer_type)
  {
	case TIMER_1SEC  :
		{
			top_value_1sec  = 1000;
			time_out_1sec  = false;

			break;
		}
	case TIMER_2SEC  :
		{
			top_value_2sec  = 2000;
			time_out_2sec  = false;

			break;
		}
	case TIMER_3SEC  :
			{
				top_value_3sec  = 3000;
				time_out_3sec  = false;

				break;
			}
	case TIMER_5SEC  :
		{
			top_value_5sec  = 5000;
			time_out_5sec = false;
			break;
		}
	case TIMER_6SEC  :
		{
			top_value_6sec  = 6000;
			time_out_6sec = false;
			break;
		}
	case TIMER_7SEC  :
			{
				top_value_7sec= 7000;
				time_out_7sec = false;
				break;
			}
	case TIMER_10SEC :
		{
			top_value_10sec = 10000;
			time_out_10sec = false;
			break;
		}
	case TIMER_15SEC :
		{
			top_value_15sec = 15000;
			time_out_15sec = false;
			break;
		}
	case TIMER_30SEC :
		{
			top_value_30sec = 30000;
			time_out_30sec = false;
			break;
		}
	case TIMER_1MIN  :
		{
			top_value_1min  = 60000;
			time_out_1min = false;
			break;
		}
	case TIMER_3MIN  :
		{
			top_value_3min  = 180000;
			time_out_3min = false;
			break;
		}
	case TIMER_5MIN  :
		{
			top_value_5min  = 300000;
			time_out_5min = false;
			break;
		}
	case TIMER_10MIN :
		{
			top_value_10min = 600000;
			time_out_10min = false;
			break;
		}

	case TIMER_30MIN :
		{
			top_value_30min = 1800000;
			time_out_30min  = false;
			break;
		}

  }
}

void API_TIMER_Kill_Timer(TIMER_t timer_type)
{
	switch(timer_type)
	  {
		case TIMER_1SEC  :{
			top_value_1sec  = 0;
			time_out_1sec  = false;
			break;
		}
		case TIMER_2SEC  :{
			top_value_2sec  = 0;
			time_out_2sec  = false;
			break;
		}
		case TIMER_3SEC  :{
					top_value_3sec  = 0;
					time_out_3sec  = false;
					break;
				}

		case TIMER_5SEC  :{
			top_value_5sec  = 0;
			time_out_5sec  = false;
			break;
		}
		case TIMER_6SEC  :{
			top_value_6sec  = 0;
			time_out_6sec  = false;
			break;
		}
		case TIMER_7SEC  :{
			top_value_7sec  = 0;
			time_out_7sec  = false;
			break;
				}
		case TIMER_10SEC :{
			top_value_10sec = 0;
			time_out_10sec  = false;
			break;
		}
		case TIMER_15SEC :{
			top_value_15sec = 0;
			time_out_15sec  = false;
			break;
		}
		case TIMER_30SEC :{
			top_value_30sec = 0;
			time_out_30sec  = false;
			break;
		}
		case TIMER_1MIN  :{
			top_value_1min  = 0;
			time_out_1min = false;
			break;
		}
		case TIMER_3MIN  :{
			top_value_3min  = 0;
			time_out_3min = false;
			break;
		}
		case TIMER_5MIN  :{
			top_value_5min  = 0;
			time_out_5min = false;
			break;
		}
		case TIMER_10MIN :{
			top_value_10min = 0;
			time_out_10min = false;
			break;
		}

		case TIMER_30MIN :{
					top_value_30min = 0;
					time_out_30min = false;
					break;
				}

	  }
}

bool API_TIMER_Get_Timeout_Flag(TIMER_t timer_type)
{
	bool time_out_flag = false;

	switch(timer_type)
	  {

		case TIMER_1SEC  :
		{
			if(time_out_1sec == true)
			{
				time_out_1sec  = false;

				time_out_flag =true;
			}
		 break;
		}
		case TIMER_2SEC  :
		{
			if(time_out_2sec == true)
			{
				time_out_2sec  = false;

				time_out_flag =true;
			}
		 break;
		}

		case TIMER_3SEC  :
				{
					if(time_out_3sec == true)
					{
						time_out_3sec  = false;

						time_out_flag =true;
					}
				 break;
				}
		case TIMER_5SEC  :
		{
			if(time_out_5sec == true)
			{
				time_out_5sec  = false;

				time_out_flag =true;
			}
		 break;
		}

		case TIMER_6SEC  :
		{
			if(time_out_6sec == true)
			{
				time_out_6sec  = false;

				time_out_flag =true;
			}
		 break;
		}

		case TIMER_7SEC  :
				{
					if(time_out_7sec == true)
					{
						time_out_7sec  = false;

						time_out_flag =true;
					}
				 break;
				}

		case TIMER_10SEC  :
		{
			if(time_out_10sec == true)
			{
				time_out_10sec  = false;
				time_out_flag =true;
			}
		 break;
		}
		case TIMER_15SEC :
		{
			if(time_out_15sec == true)
			{
				time_out_15sec = false;
				time_out_flag =true;
			}
		 break;
		}
		case TIMER_30SEC :
		{
			if(time_out_30sec == true)
			{
				time_out_30sec = false;
				time_out_flag =true;
			}
		 break;
		}
		case TIMER_1MIN  :
		{
			if(time_out_1min == true)
			{
				time_out_1min = false;
				time_out_flag =true;
			}
		 break;
		}

		case TIMER_3MIN  :
		{
			if(time_out_3min == true)
			{
				time_out_3min = false;
				time_out_flag =true;
			}
		 break;
		}

		case TIMER_5MIN  :
		{
			if(time_out_5min == true)
			{
				time_out_5min = false;
				time_out_flag =true;
			}
		 break;
		}

		case TIMER_10MIN  :
		{
			if(time_out_10min == true)
			{
				time_out_10min = false;
				time_out_flag =true;
			}


		 break;
		}

		case TIMER_30MIN  :
				{
					if(time_out_30min == true)
					{
						time_out_30min = false;
						time_out_flag =true;
					}


				 break;
				}


	} // switch end

	return time_out_flag;
}



void API_TIMER_Run_1MS_Timer(void)
{

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* The timer has been created but is not running yet */

    /* Start the timers */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));
    ESP_LOGI("ESP 1 ms timer", "Started timers, time since boot: %lld us", esp_timer_get_time());

}



void Delay_ms(uint32_t t_ms)
{
	Delay_time_out = t_ms;

	usleep(t_ms*1000);
}
