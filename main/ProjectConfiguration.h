#ifndef _PROJECT_CONFIGURATION_H_
#define _PROJECT_CONFIGURATION_H_

#include <stdbool.h>
#include "API_timer.h"

#define ENABLE_DEBUG_MESSAGES        true
#define DEVICE_ID                    (uint32_t)0x1234abcd // TODO, Read from muc or flash

#define DISPLAY_TEST_SCEEN_TIMEOUT_MS   3000U // 3 sec
#define SAMPLING_FREQUENCY_PPG_ECG      100U
#define DEEP_SLEEP_TIMEOUT           (TIMER_t)TIMER_1MIN
#define TEST_ENTERY_TIMEOUT          (TIMER_t)TIMER_3SEC
#define USER_INACTIVE_TIMEOUT        (TIMER_t)TIMER_1MIN
#define DATE_TIME_FLIP_TIME          (TIMER_t)TIMER_5SEC
#define DATA_SYNC_TIMEOUT			 (TIMER_t)TIMER_30SEC
#define LEAD_OFF_DETECTION           true


#endif
