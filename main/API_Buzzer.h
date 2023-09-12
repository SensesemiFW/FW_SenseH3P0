#ifndef _API_BUZZER_H_
#define _API_BUZZER_H_

typedef enum{
	SHORT_BEEP,
	MEDIUM_BEEP,
	LONG_BEEP,
}BUZZER_SOUND_t;

void API_Buzzer_Sound(BUZZER_SOUND_t beep_type);







#endif
