#ifndef _BTL_H_
#define _BTL_H_

#include <stdbool.h>
#include <stdint.h>

// BTL (Boot loader)

#define APPLICATION_SIZE                  (uint32_t)(900*1024) // Each application size = 400kb .
#define FW_BUFFER_THRESHOLD_INDEX         (uint32_t)50000  // FW( firmware buffer )
#define EXT_FLASH_SECTOR_SIZE 	  		  (uint32_t)4096
//#define BUFFER_SIZE   (uint32_t)51200 // Firmware buffer size of 50kb
#define BUFFER_SIZE   (uint32_t)1024 

#define OFFSET_TO_RECORD_TYPE   3

typedef struct __attribute__((__packed__))
{
	uint8_t  year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  hour;
	uint8_t  minute;
	uint8_t  second;

}DATE_TIME_t;

typedef enum
{
	FW_PACKET_GOOD = 0,
	FW_PACKET_CURRUPTED,
	END_OF_FILE_RECORD,
}FIRMWARE_DATA_STATE_t;

typedef enum
{
	FIRMWARE_RECORD_TYPE0 = 0,
	FIRMWARE_RECORD_TYPE1,
	FIRMWARE_RECORD_TYPE2,
	FIRMWARE_RECORD_TYPE3

} FIRMWARE_RECORD_TYPE_t;

typedef enum
{
	DEVICE_UPGRADE_START = 0,
	DEVICE_UPGRADING,
    DEVICE_UPGRADE_SUCCESS,
    DEVICE_UPGRADE_FAIL,
} FIRMWARE_UPGRADE_STATE_t;

extern bool     App_Receiving_success;
extern bool     BT_ongoing_session;
extern bool     Is_arrived_valid_data;
extern bool FW_complete_data_received;

int read_firmware_data (uint8_t * ota_write_data, int BUFFSIZE);
bool is_firmware_data_available (void);
bool start_firmware_Upgrade(void);
bool Firmware_upgrade (void);


#endif
