#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdbool.h>
#include <stdint.h>

#include "esp_timer.h"

typedef enum
{
	TIMER_1SEC = 0x00,
	TIMER_2SEC,
	TIMER_3SEC,
	TIMER_5SEC,
	TIMER_6SEC,
	TIMER_7SEC,
	TIMER_10SEC,
	TIMER_15SEC,
	TIMER_30SEC,
	TIMER_1MIN,
	TIMER_3MIN,
	TIMER_5MIN,
	TIMER_10MIN,
	TIMER_30MIN

} TIMER_t;

extern esp_timer_handle_t periodic_timer;

extern uint32_t Delay_time_out;
void Delay_ms(uint32_t t_ms);
extern uint32_t BTN_timing;

extern TIMER_t  Fw_upgrading_time_out;

void API_TIMER_Run_1MS_Timer(void);
void API_TIMER_Register_Timer(TIMER_t timer_type);
void API_TIMER_Kill_Timer(TIMER_t timer_type);
bool API_TIMER_Get_Timeout_Flag(TIMER_t timer_type);


#endif
