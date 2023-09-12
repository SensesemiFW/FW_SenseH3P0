#include "rtc.h"
#include "esp_task_wdt.h"
#include <stdlib.h>
/**
 * @brief Get actual internal RTC date in UNIX format.
 * 
 * @return UNIX date.
 */
int8_t Time_zone;
extern char date_info[50];
int32_t get_unix()
{
	time_t now = 0;
	time(&now);
	return now+(3600*Time_zone);
}

/**
 * @brief Get actual internal RTC date with specific string format.
 * 
 * This special function can be used to output formatted date (string)
 * with any format (specified by strftime(), to more details, Google it!)
 * 
 * Default format is "DAY/MONTH/YEAR HOUR:MINUTES:SECONDS"
 * 
 * @attention Destiny char array need to be >= 18B for default format.
 * 
 * @param [*dst]: Destiny char_array which will contain the current date.
 * @param [*format]: Specific formatted string output date.
 */
//void get_string(char *dst, const char *format="%d/%m/%y %H:%M:%S")
void API_RTC_Get_Date_Time(char *dst, const char *format)
{
	struct tm *stm;

	time_t now = get_unix();
	stm = gmtime(&now);

	strftime(dst, 18, format, stm);
	//printf("Data inside dst buffer: %s\n", dst);
}


/**
 * @brief Set actual internal RTC date with UNIX number.
 * 
 * @param [time]: UNIX to be set.
 */
void set_unix(int32_t time)
{
	struct timeval stv;
	stv.tv_sec = time;
	stv.tv_usec = 0;
	settimeofday(&stv, NULL);
}

/**
 * @brief Set actual internal RTC date with one-by-one parameter.
 * 
 * @param [day]: Day. (1-31)
 * @param [month]: Month. (1-12)
 * @param [year]: Year. Eg: 2020.
 * @param [hour]: Hour. (0-23)
 * @param [min]: Minutes. (0-59)
 * @param [sec]: Seconds. (0-59)
 */
void API_RTC_Update_Date_Time(uint8_t day, uint8_t month, uint16_t year, uint8_t hour, uint8_t min, uint8_t sec)
{
	struct tm stm;
	struct timeval stv;
	time_t date;

	stm.tm_mday = day;
	stm.tm_mon  = month-1;
	stm.tm_year = year-1900;
	stm.tm_hour = hour;
	stm.tm_min  = min;
	stm.tm_sec  = sec;
	date = mktime(&stm);

	stv.tv_sec = date;
	stv.tv_usec = 0;
	settimeofday(&stv, NULL);
}

/**
 * @brief Set actual internal RTC date automatic by NTP.
 * 
 * @attention This function will block code (task) for max 10secs.
 * @attention WiFi with internet connection is needed.
 * 
 * 
 * @param [*server]: NTP server to get date. Eg: "a.ntp.br"
 * @param [timezone]: Timezone to apply with NTP successful. (because NTP return in UTC)
 * 
 * @return 0: Sync fail.
 * @return 1: Sync sucess.
 */
//uint8_t set_ntp(const char *server, int8_t timezone=0)
uint8_t set_ntp(const char *server, int8_t timezone)
{
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, (char*)server);
	sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
	sntp_init();

	int8_t ok = 0;
	for (int16_t i = 0; i < 100; i++)
	{
		esp_task_wdt_reset();
		vTaskDelay(pdMS_TO_TICKS(100));

		if (sntp_get_sync_status() != SNTP_SYNC_STATUS_RESET)
		{
			ok = 1; break;
		}
	}
	

	sntp_stop();
	if (ok)
	{
		Time_zone = timezone;
		return 1;
	}
	else
	{
		return 0;
	}
}


void Print_time(char *name){
	memset(date_info,'\0',sizeof(date_info));
	API_RTC_Get_Date_Time(date_info,"%d/%m/%y %H:%M:%S");
	printf(name);
	printf(" Time: %s\n",date_info);
}
