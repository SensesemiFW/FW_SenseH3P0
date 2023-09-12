#include "API_Buzzer.h"
#include "API_IO_Exp.h"
#include "API_timer.h"
#include <stdbool.h>


static void short_beep(void);
static void medium_beep(void);
static void long_beep(void);

static void short_beep(void)
{
	uint32_t ms_count=0;
	uint8_t beep_count = 0;

	while(1)
	{
		API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);
		API_IO_Exp1_P0_write_pin(BUZZER_EN,HIGH);
		ms_count++;

		if(ms_count>=70)
		{
			API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);
			Delay_ms(50);
			ms_count =0;
			beep_count++;
		}


		if(beep_count >= 3) break;
	}

}


static void medium_beep(void)
{

	uint32_t ms_count=0;
	uint8_t beep_count = 0;

	while(1)
	{
		API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);
		Delay_ms(1);
		API_IO_Exp1_P0_write_pin(BUZZER_EN,HIGH);
		Delay_ms(1);
		ms_count++;

		if(ms_count>=(4*100))
		{
			API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);
			Delay_ms(4*200);
			ms_count =0;
			beep_count++;
		}

	if(beep_count >= 3) break;
	}

}

static void long_beep(void)
{

	uint32_t ms_count=0;
	uint8_t beep_count = 0;

	while(1)
	{
		API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);
		Delay_ms(1);
		API_IO_Exp1_P0_write_pin(BUZZER_EN,HIGH);
		Delay_ms(1);
		ms_count++;

		if(ms_count>=(4*400))
		{
			API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);
			Delay_ms(4*800);
			ms_count =0;
			beep_count++;
		}

		if(beep_count >= 3) break;
	}


}


void API_Buzzer_Sound(BUZZER_SOUND_t beep_type)
{
	switch (beep_type)
	{
	case SHORT_BEEP: short_beep();break;
	case MEDIUM_BEEP: medium_beep();break;
	case LONG_BEEP: long_beep(); break;
	default : break;

	}

	API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);

}
