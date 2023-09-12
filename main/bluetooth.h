#ifndef SOURCE_BLUETOOTH_H_
#define SOURCE_BLUETOOTH_H_

#include "stdint.h"
#include "stdbool.h"

#define ACK								0x00

#define SOR								0x01	// Start Of Record
#define COR								0x02 	// Continuation Of Record
#define EOR								0x03	// End Of Record
#define SOS							    0xC0	// Start Of String
#define EOS							    0xC0    // End Of String

/*
 * BT Request/Response commands
 */
#define BP1_data_req					0x10
#define BP2_data_req					0x90
#define BG_data_req						0x11
#define ECG_1_Lead_data_req				0x12
#define ECG_6_Lead_data_req				0x13
#define SPO2_data_req					0x14
#define SENSETEMP_data_req				0x15
#define ECG_12_Lead_data_req			0x16
#define OFFLINE_DATA_REQ				0x17
#define BP_multiplier_req				0x18

#define FW_VER_REQ						0x61

#define OTA_FW_UPGRADE_REQ				0x70
#define Patient_Reg_req					0x71
#define TIME_SYNC_REQ					0x72
#define REC_SNAPSHOT_REQ				0x73
#define REC_SNAPSHOT_REQ_OFFLINE_CONF	0x74

#define Authenticate_Mobile_App			0x75
#define CRYPTO_key_req					0x76
#define Device_Configuration			0x77

#define NACK_INVALID_COMMAND		 	0xFB
#define NACK_INVALID_PACKET			 	0xFC
#define NACK_REQ_DATA_NOT_SUPPORTED 	0xFD
#define NACK_DATA_NOT_AVAILABLE 		0xFE
#define NACK_DEVICE_BUSY		 		0xFF

#define BT_MOBILE_NUM_SYNC_REQ      	0x78

#define BT_FULL_DIAGNOSTICS_REQ         0x79
#define BT_SELF_DIAGNOSTICS_REQ         0x80

#define NO_CMD_REQ						0xCD

#define DATA_SYNC_NOTIFICATION_TIMEOUT   TIMER_6SEC		// TIMER_15SEC
#define BT_DATA_SYNC_TIMEOUT             TIMER_7SEC


/*
 * Request/Response structures for BT communication 
 */
typedef struct __attribute__((__packed__))
{
    uint8_t     sos;
    uint8_t     cmd;
    uint16_t    length;
    uint8_t     data[54];
    uint8_t     eos;
    uint16_t    chksum;
}SYNC_HEADER_STRUCT;


typedef struct __attribute__((__packed__))
{
    uint8_t     sos;
    uint8_t     cmd;
    uint16_t    length;
    uint8_t     data[8];
    uint8_t     eos;
    uint16_t    chksum;
}DEVICE_DIAGNOSIS_STRUCT_t;


typedef struct __attribute__((__packed__))
{
    uint8_t     sos;
    uint8_t     cmd;
    uint16_t    length;
    uint16_t	bp1_rec;
    uint16_t	bp2_rec;
	uint16_t	bg_rec;
	uint16_t	spo2_rec;
	uint16_t	sensetemp_rec;
	uint16_t	ecg1_rec;
	uint16_t	ecg6_rec;
	uint16_t	ecg_12lead_rec;
	uint16_t	bp_calib_rec;
    uint8_t     eos;
    uint16_t    chksum;
}REC_SNAP_RES;


typedef struct __attribute__((__packed__))
{
	uint8_t     sos;
	uint8_t     cmd;
	uint16_t    length;
	uint8_t		data;
	uint8_t     eos;
	uint16_t    chksum;

}REC_SNAP_RES_OFFLINE_CONFIG;


typedef struct __attribute__((__packed__))
{
    uint8_t     sos;
    uint8_t     cmd;
    uint16_t    length;
    uint8_t     eos;
    uint16_t    chksum;
}ACK_AND_NACK_RESPONSE;


typedef struct __attribute__((__packed__))
{
    uint8_t     sos;
    uint8_t     cmd;
    uint16_t    length;
    uint8_t     data[81];
    uint8_t     eos;
    uint16_t    chksum;
}SYNC_HEADER_STRUCT_OFFLINE_CONFIG;


typedef struct __attribute__((__packed__))
{
	uint8_t     sos;
	uint8_t     cmd;
	uint16_t    length;
	uint8_t 	data[10];
	uint8_t     eos;
	uint16_t    chksum;
}FW_VER_RESPONSE;


/*
 * Vitals records sync states  
 */
typedef enum __attribute__((__packed__))
{
	INIT_STATE,
	WAIT_ACK_SOR_STATE,
	WAIT_ACK_COR_STATE,
	WAIT_ACK_EOR_STATE,
}VITALS_RESPONSE_STATE;


typedef enum __attribute__((__packed__))
{
	BG_REQ_INIT_STATE,
	BG_WAIT_ACK_EOR_STATE,
}BT_BG_RESPONSE_STATE;

typedef enum __attribute__((__packed__))
{
	TEMP_REQ_INIT_STATE,
	TEMP_WAIT_ACK_EOR_STATE,
}BT_TEMP_RESPONSE_STATE;


typedef enum __attribute__((__packed__))
{
	OFFLINE_REC_REQ_INIT_STATE,
	OFFLINE_REC__WAIT_ACK_SOR_STATE,
	OFFLINE_REC__WAIT_ACK_COR_STATE,
	OFFLINE_REC__WAIT_ACK_EOR_STATE,
}BT_OFFLINE_CONFIG_REC_RESPONCE;


typedef enum __attribute__((__packed__))
{
	BP_MULTIPLIER_REQ_INIT_STATE,
	BP_MULTIPLIER_WAIT_ACK_EOR_STATE,
}BP_MULTIPLIER_RESPONSE_STATE;


typedef enum __attribute__((__packed__))
{
	BT_DISCONNECTED=1,
	BT_PAIRED,
	DEFAULT,
} BT_STATUS;

extern bool Data_sync_in_progress;
extern bool Prepare_device_to_upgrade;
extern bool BT_ongoing_session;
extern BT_STATUS Is_Device_Paired;

uint8_t BT_Initialize(void);
void BT_process_requests(void);
char* BT_Get_mobile_number(void);
bool BT_Get_timesynq_req_flag(void);
int BTL_validate_and_copy2buf(uint8_t * fw_buff_ptr);
void BT_Sync_Timeout_Init_State(void);


#endif /* SOURCE_BLUETOOTH_H_ */
