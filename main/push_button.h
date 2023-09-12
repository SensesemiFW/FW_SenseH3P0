/*
 * push_button.h
 *
 *  Created on: 22-Apr-2022
 *      Author: pooja
 */

#ifndef PUSH_BUTTON_H_
#define PUSH_BUTTON_H_
#include <stdbool.h>
#include <stdint.h>

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define ESP_INTR_FLAG_DEFAULT 1

extern uint8_t Push_btn_Press;

extern bool intr_flag;

extern bool Did_Push_Button_Pressed;

void API_Push_Btn_init(void);
uint32_t API_Push_Btn_Get_hold_time();
void API_Reset_Push_Button_State(void);
bool API_Check_Test_Exit(void);
uint32_t API_Push_Btn_Debounce_check(uint32_t time_elapsed);

uint8_t API_Push_Btn_Get_Buttton_Press(void);
void PowerDownAllGpios(void);

#endif /* PUSH_BUTTON_H_ */
