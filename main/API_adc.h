#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>
#include <stdbool.h>

void API_IR_ADC_Init(void);
void API_IR_Data_Read(uint16_t data_object[], uint16_t data_ambient[],bool enable_adc2_read,uint16_t length_to_read);
void API_ADC_Conv_Avg(float ref_vol, uint16_t data_buff[],uint8_t avg_len,char *tag);


void API_RUN_TEMPERATURE_TEST(void);





#endif
