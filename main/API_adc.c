/* ADC1 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "API_adc.h"

#include <stdio.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "API_timer.h"

#if CONFIG_IDF_TARGET_ESP32
#include "esp_adc_cal.h"
#endif

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   10          //Multisampling

#if CONFIG_IDF_TARGET_ESP32
static esp_adc_cal_characteristics_t *adc_chars;
#elif CONFIG_IDF_TARGET_ESP32S2BETA
static const adc_channel_t channel = ADC_CHANNEL_6;     // GPIO7 if ADC1, GPIO17 if ADC2
#endif
static const adc_atten_t atten = ADC_ATTEN_DB_11;//;ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

#if CONFIG_IDF_TARGET_ESP32
static void check_efuse(void)
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}
#endif

void  API_IR_ADC_Init(void)
{
#if CONFIG_IDF_TARGET_ESP32
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();
#endif

    //Configure ADC
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(ADC_CHANNEL_1, atten);
        adc1_config_channel_atten(ADC_CHANNEL_2, atten);

      //  adc_power_on();
      //  adc2_config_width(ADC_WIDTH_BIT_12);
        //adc2_config_channel_atten((adc2_channel_t)channel, atten);

#if CONFIG_IDF_TARGET_ESP32
    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
#endif

}

void API_IR_Data_Read(uint16_t data_object[], uint16_t data_ambient[],bool enable_adc2_read,uint16_t length_to_read)
{
        //Multisampling
        for (int i = 0; i < length_to_read; i++)
        {
        	    data_object[i] = adc1_get_raw(ADC_CHANNEL_1);

            	if(enable_adc2_read){
                //adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, data_ambient+i);
            		data_ambient[i] = adc1_get_raw(ADC_CHANNEL_2);

            }
        }

}

void API_ADC_Conv_Avg(float ref_vol, uint16_t data_buff[],uint8_t avg_len,char *tag)
{
	uint32_t avg = 0;
	float voltage = 0.0;

	 for(int i=0;i<avg_len;i++)
	 {
		 avg +=  data_buff[i];
	 }

	 avg = avg/avg_len;
       // float voltage = adc_reading*1.1/4095;

	    voltage = avg*ref_vol/4095;
        printf("%s, Raw: %ld\tVoltage: %f\n", tag, avg, voltage);
}

void API_RUN_TEMPERATURE_TEST(void)
{
	uint16_t obj_data;
	uint16_t amb_data;
	printf("\n\nIR Data Capture 10 SPS");

	while(1)
	{
	  API_IR_Data_Read(&obj_data, &amb_data,true,1);

	  printf("\n%d    %d",obj_data,amb_data);
	  Delay_ms(100);
	}
}
