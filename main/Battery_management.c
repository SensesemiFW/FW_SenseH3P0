/*
 * Battery_management.c
 *
 *  Created on: 16-Dec-2022
 *      Author: sensesemi
 */

#include "API_Battery_monitor.h"
#include "API_Display.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "API_timer.h"
#include "API_utility.h"
#include "API_Flash_org.h"
#include "Battery_management.h"
#include "API_IO_Exp.h"
#include "esp_sleep.h"
#include "push_button.h"
#include "esp_bt.h"
#include "max86150.h"
#include "esp_bt_main.h"
#include "driver/periph_ctrl.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"
#include "Hardware.h"
#include "driver/adc.h"


#define BATTERY_LOW_VOLTAGE_THRESHOLD   (float)3.7
static void Enter_Deep_Sleep(void);

static void power_down_all_modules(void);

bool Detect_low_battery_display_notification()
{
	bool is_lowBattery = false;

	static float battery_voltage = 0x00;

	static char buff[10]; // testing

	//MemSet(buff,'\0',Get_strlen(buff));

	  battery_voltage = API_ADC_Read_Battery_Voltage();

	//  printf("\nbattery_voltage=%f",battery_voltage);

	  Fuel_Guage_update_battery_status(battery_voltage);

		/*FloatToString(battery_voltage,buff);
		API_Disp_Dsplay_Char_With_Offset(21,4,buff,GREEN);*/

		if(battery_voltage < FUEL_GUAGE_10_PER_VOLTAGE)
		{
			   API_DISP_Display_Screen(DISP_VERY_LOW_VOLTAGE_PLEASE_CHARGE);// critically low voltage
			   is_lowBattery = true;
		}

		else if(battery_voltage < FUEL_GUAGE_15_PER_VOLTAGE)
		{
			  API_DISP_Display_Screen(DISP_LOW_VOLTAGE_PLEASE_CHARGE); // low voltage
			  is_lowBattery = true;
		}

		return  is_lowBattery;
}

void EnterSleepMode(SYSTEM_POWER_STATE_t powerMode)
{
	if(powerMode == SYSTEM_SEMI_SLEEP)
	{
       //
	}
	else if(powerMode == SYSTEM_SLEEP)
	{
		 //esp_bt_sleep_enable();
	    // Power_Down_All_Modules();
	}

	else if(powerMode == SYSTEM_DEEP_SLEEP)
	{
		Enter_Deep_Sleep();
	}

}

static void Enter_Deep_Sleep(void)
{
    uint8_t allhigh = ALL_PINS_HIGH;
    uint8_t alllow  = ALL_PINS_LOW;

    API_IO_Exp_Reset();

	API_IO_Exp_PowerOnReset_Configuration();

	API_Display_spi_init();
	api_disp_write_com(DISPLAY_SLEEP_MODE_CMD);

//	adc_power_off();

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_PULLUP_PULLDOWN_EN_0,&allhigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_ADDR_CONFIGURATION_PORT_0,&allhigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_PULLUP_PULLDOWN_SEL_0,&alllow,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_ADDR_OUTPUT_PORT_0,&alllow,1);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_PULLUP_PULLDOWN_EN_1,&allhigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_ADDR_CONFIGURATION_PORT_1,&allhigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_PULLUP_PULLDOWN_SEL_1,&alllow,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_ADDR_OUTPUT_PORT_1,&alllow,1);


	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_PULLUP_PULLDOWN_EN_0,&allhigh,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_ADDR_CONFIGURATION_PORT_0,&allhigh,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_PULLUP_PULLDOWN_SEL_0,&alllow,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_ADDR_OUTPUT_PORT_0,&alllow,1);

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_PULLUP_PULLDOWN_EN_1,&allhigh,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_ADDR_CONFIGURATION_PORT_1,&allhigh,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_PULLUP_PULLDOWN_SEL_1,&alllow,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_ADDR_OUTPUT_PORT_1,&alllow,1);


	periph_module_disable(PERIPH_I2C0_MODULE);
	periph_module_disable(PERIPH_I2C1_MODULE);
	periph_module_disable(PERIPH_HSPI_MODULE);
	periph_module_disable(PERIPH_VSPI_MODULE);
	periph_module_disable(PERIPH_WIFI_MODULE);
	periph_module_disable(PERIPH_BT_MODULE);
	periph_module_disable(PERIPH_RMT_MODULE);
	periph_module_disable(PERIPH_BT_BASEBAND_MODULE);
	periph_module_disable(PERIPH_BT_LC_MODULE);
	periph_module_disable(PERIPH_SDIO_SLAVE_MODULE);
	periph_module_disable(PERIPH_TIMG1_MODULE);
	periph_module_disable(PERIPH_PCNT_MODULE);
	periph_module_disable(PERIPH_PWM0_MODULE);
	periph_module_disable(PERIPH_PWM1_MODULE);

	wifi_bt_common_module_disable();

	esp_sleep_enable_ext0_wakeup(GPIO_NUM_36,LOW);
	esp_deep_sleep_disable_rom_logging();
    esp_deep_sleep_start();

}

static void power_down_all_modules(void)
{
	printf("\nPower down block!\n");
			gpio_num_t next_pin = 0;
			periph_module_disable(PERIPH_I2C0_MODULE);
			periph_module_disable(PERIPH_I2C1_MODULE);
			//periph_module_disable(PERIPH_SPI_MODULE);
			periph_module_disable(PERIPH_HSPI_MODULE);
			periph_module_disable(PERIPH_VSPI_MODULE);
			periph_module_disable(PERIPH_WIFI_MODULE);
			periph_module_disable(PERIPH_BT_MODULE);
			periph_module_disable(PERIPH_RMT_MODULE);
			periph_module_disable(PERIPH_BT_BASEBAND_MODULE);
			periph_module_disable(PERIPH_BT_LC_MODULE);
			//periph_module_disable(PERIPH_SARADC_MODULE);
			periph_module_disable(PERIPH_SDIO_SLAVE_MODULE);
			//periph_module_disable(PERIPH_TIMG0_MODULE);
			periph_module_disable(PERIPH_TIMG1_MODULE);
			periph_module_disable(PERIPH_PCNT_MODULE);
			periph_module_disable(PERIPH_PWM0_MODULE);
			periph_module_disable(PERIPH_PWM1_MODULE);
			//periph_module_disable(PERIPH_MODULE_MAX);
			wifi_bt_common_module_disable();

			for (next_pin = 13; next_pin < 16; next_pin++)
			{
				if((gpio_reset_pin(next_pin)) == ESP_ERR_INVALID_ARG)	{
					printf("\nReset error @GPIO: %d !!\n", next_pin);
				}
			}
			for (next_pin = 25; next_pin < 28; next_pin++)
			{
				if((gpio_reset_pin(next_pin)) == ESP_ERR_INVALID_ARG)	{
					printf("\nReset error @GPIO: %d !!\n", next_pin);
				}
			}
			for (next_pin = 32; next_pin < 40; next_pin++)
			{
				if(next_pin != 36)
				if((gpio_reset_pin(next_pin)) == ESP_ERR_INVALID_ARG)	{
					printf("\nReset error @GPIO: %d !!\n", next_pin);
				}
			}
}


void Detect_Battery_Dead_Push_Device_Hibernate(void)
{
	float battery_voltage = 0.0;

	API_TIMER_Run_1MS_Timer();
	API_Battery_monitor_Init();

	battery_voltage = API_ADC_Read_Battery_Voltage();

	printf("\nbattery_voltage=%f",battery_voltage);

	if(1)
	//if(battery_voltage < BATTERY_LOW_VOLTAGE_THRESHOLD)
	{
		//API_IO_Exp_init();
		//power_down_setup();

		//API_enable_display_sleep();
		//detect_bat_0_restart();
		power_down_all_modules();
		esp_sleep_enable_ext0_wakeup(GPIO_NUM_36,LOW);
		esp_sleep_enable_ext1_wakeup(GPIO_NUM_36,LOW);
		esp_deep_sleep_disable_rom_logging();
		esp_deep_sleep_start();
	}

}
