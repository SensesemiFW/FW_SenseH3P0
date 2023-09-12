/*
 * Battery_management.h
 *
 *  Created on: 16-Dec-2022
 *      Author: sensesemi
 */

#ifndef _BATTERY_MANAGEMENT_H_
#define _BATTERY_MANAGEMENT_H_


typedef enum
{
	SYSTEM_POWER_DOWN,
	SYSTEM_POWER_UP,
	SYSTEM_SEMI_SLEEP,
	SYSTEM_SLEEP,
	SYSTEM_DEEP_SLEEP,
} SYSTEM_POWER_STATE_t;



extern SYSTEM_POWER_STATE_t Power_up_flag;

bool Detect_low_battery_display_notification();// Will checks the battery voltage, if the voltage is < 10 % then display low battery notification and holds the
void EnterSleepMode(SYSTEM_POWER_STATE_t powerMode);

void Detect_Battery_Dead_Push_Device_Hibernate(void);


#endif /* MAIN_BATTERY_MANAGEMENT_H_ */
