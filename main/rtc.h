#ifndef rtc_H
#define rtc_H

#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <lwip/apps/sntp.h>
#include "esp_sntp.h"

int32_t get_unix();
void API_RTC_Get_Date_Time(char *dst, const char *format);

void set_unix(int32_t time);
void API_RTC_Update_Date_Time(uint8_t day, uint8_t month, uint16_t year, uint8_t hour, uint8_t min, uint8_t sec);

uint8_t set_ntp(const char *server, int8_t timezone);
void Print_time(char *name);

#endif
