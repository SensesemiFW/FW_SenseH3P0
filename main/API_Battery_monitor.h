
#ifndef SRC_FUEL_GUAGE_SRC_H_
#define SRC_FUEL_GUAGE_SRC_H_

#include <stdbool.h>

typedef enum
{
	DEVICE_ACTIVE = 0,
	DEVICE_SLEEP

}DEVICE_MODE_t;

#define FUEL_GUAGE_MIN_VOLTAGE              (float) 3.1
#define FUEL_GUAGE_MAX_VOLTAGE              (float) 4.2
#define FUEL_GUAGE_VOLTAGE_DIFF             (float) (FUEL_GUAGE_MAX_VOLTAGE - FUEL_GUAGE_MIN_VOLTAGE)

/*  Percentage voltage calculations */

#define FUEL_GUAGE_10_PER_VOLTAGE         	(float) (((FUEL_GUAGE_VOLTAGE_DIFF * 10) / 100) + FUEL_GUAGE_MIN_VOLTAGE)
#define FUEL_GUAGE_15_PER_VOLTAGE         	(float) (((FUEL_GUAGE_VOLTAGE_DIFF * 15) / 100) + FUEL_GUAGE_MIN_VOLTAGE)
#define FUEL_GUAGE_33_PER_VOLTAGE         	(float) (((FUEL_GUAGE_VOLTAGE_DIFF * 33) / 100) + FUEL_GUAGE_MIN_VOLTAGE)
#define FUEL_GUAGE_66_PER_VOLTAGE         	(float) (((FUEL_GUAGE_VOLTAGE_DIFF * 66) / 100) + FUEL_GUAGE_MIN_VOLTAGE)
#define FUEL_GUAGE_100_PER_VOLTAGE        	FUEL_GUAGE_MAX_VOLTAGE


#define HUNDRED            					100
#define ONE_PERCENT         				(float)0.016

//bool voltage_critically_low ;


bool  API_Battery_monitor_Init(void);
void Fuel_Guage_update_battery_status(float batVoltage);
void Fuel_Guage_turn_off_notification_led(void);


float API_ADC_Read_Battery_Voltage(void);

#endif /* SRC_FUEL_GUAGE_SRC_H_ */
