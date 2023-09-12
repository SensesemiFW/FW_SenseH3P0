#ifndef API_API_FLASH_ORG_H_
#define API_API_FLASH_ORG_H_

#include <stdint.h>
#include <stdbool.h>

/*-------------------------------------- MACROS -------------------------------------------------------------*/
/* Flash sector size is fixed for 4096 bytes  */
#define FLASH_SECTOR_SIZE					4096

/* Total 100 number of each vital records will be maintained in internal flash */
#define MAX_RECORDS 				  		100
#define MAX_PID_RECORDS						10

/*------------------------------------One RECORD length -------------------------------------------*/
#define PID_ONE_REC_LEN 	   				18U
#define PPG_RED_ONE_REC_LEN					4854U				//Total red_data length = ((1200*4)+54)
#define PPG_IR_ONE_REC_LEN					4854U				//Total IR_data length = ((1200*4)+54)
#define PPG_ECG_ONE_REC_LEN					2454U				// (600*4)=2400+54(Header Len)
#define ECG_1_LEAD_REC_LEN 					2454U			// (600*4)=2400+54(Header Len)
#define ECG_2_LEAD_REC_LEN					2454U			// (600*4)=2400+54(Header Len)
#define REC_HEADER_LEN						54U
#define BG_ONE_REC_LEN   					56U			// Total bg_data length = ((1)*2)+ 54
#define TEMP_ONE_REC_LEN       				56U			// Total temp_data length = ((1)*2)+ 54
#define SPO2_ONE_REC_LEN         			4854U		// Total spo2_data length = ((600+600)*4)+ 54
#define BP1_ONE_REC_LEN 			    	9654U		// 	Total bp_data length = ((1200+1200)*4)+ 54
#define BP2_ONE_REC_LEN						9654U
#define ECG_3_LEAD_REC_LEN 					4854U		// 	Total ecg_data length = ((600+600)*4)+ 54
#define ECG_12_LEAD_ONE_REC_LEN				19254U		// Total Record len=(47(header )+8 lead data(2400 bytes each))

/*----------------------------Off-Line RECORD field offsets-------------------------------------*/
#define ONE_OFFLINE_CONFIG_RECORD_LENGTH	19297	
#define RECORD_SUMMARY_LENGTH				48

#define OFFLINE_BP1_LEN						8
#define OFFLINE_BP2_LEN						8
#define OFFLINE_RED_PPG_LEN					8
#define OFFLINE_IR_PPG_LEN 					8
#define OFFLINE_PPG_ECG_LEN					8
#define	OFFLINE_ECG_2LEAD_LEN				8  
#define OFFLINE_BG_LEN						8	 
#define OFFLINE_SPO2_LEN					8
#define OFFLINE_SENSETEMP_LEN				8
#define OFFLINE_ECG_1LEAD_LEN				8
#define OFFLINE_ECG_6LEAD_LEN				8

#define OFFLINE_LEAD_LEN					2400
#define OFFLINE_VLEAD_LEN					2400


/* Data partition reserved for storing all the vital records. */
#define DATA_PARTITION_START_SECTOR			0

/* Number of sectors required to store 100 number of records for each vital  */
/* No. of sectors = [(one record lenght) * (100 records)] / one sector length (4096) */  
#define DATA_POINTERS_TOTAL_SECTOR			1  
#define PID_TOTAL_SECTOR					1
#define RED_TOTAL_SECTORS					120
#define IR_TOTAL_SECTORS					120
#define ECG_2_LEAD_TOTAL SECTORS 			62
#define BG_TOTAL_SECTORS					2	
#define TEMP_TOTAL_SECTORS					2
#define PPG_ECG_TOTAL_SECTORS				62
#define ECG_1_LEAD_TOTAL_SECTORS            62     //(2454*100)/4096 ~= 62
#define ECG_2_LEAD_TOTAL_SECTORS			62
#define SPO2_TOTAL_SECTORS					120    //(4854 * 100) / 4096 ~= 120
#define ECG_3_LEAD_TOTAL_SECTORS			120    //(4854 * 100) / 4096 ~= 120
#define BP1_TOTAL_SECTORS					245    //(9654 * 100) / 4096 ~= 245
#define BP2_TOTAL_SECTORS					245
#define ECG_12_LEAD_TOTAL_SECTORS			235    //(19254*100/4096)

#define OFFLINE_CONFIG_RECORD_TOTAL_SECTORS 480

#define OFFLINE_RECORD_SUMMARY_SECTORS		2
#define OFFLINE_VITAL_TOTAL_SECTORS			1		//
#define OFFLINE_LEAD_TOTAL_SECTORS			60		//(2400 * 100) / 4096 ~= 59	
#define OFFLINE_VLEAD_TOTAL_SECTORS			60		//(2400 * 100) / 4096 ~= 59		

/* Each Vital records start Sector offset */ 	
//Sector start address = previous sector's ofset + previous vital reacord's total sectors 

#define DATA_POINTERS_START_SECTOR	  		0 		//DATA_PARTITION_START_SECTOR
#define PID_START_SECTOR					1		//DATA_POINTERS_START_SECTOR + DATA_POINTERS_TOTAL_SECTOR
#define BG_START_SECTOR  	 				2	//PID_START_SECTOR + PID_TOTAL_SECTOR
#define TEMP_START_SECTOR    	 			(BG_START_SECTOR+BG_TOTAL_SECTORS)//BG_START_SECTOR + BG_TOTAL_SECTORS
#define SPO2_START_SECTOR    	 			(TEMP_START_SECTOR+TEMP_TOTAL_SECTORS)		//TEMP_START_SECTOR + TEMP_TOTAL_SECTORS
#define ECG_1_LEAD_START_SECTOR    			(SPO2_START_SECTOR+SPO2_TOTAL_SECTORS)
#define ECG_3_LEAD_START_SECTOR    			(ECG_1_LEAD_START_SECTOR+ECG_1_LEAD_TOTAL_SECTORS)
#define BP1_START_SECTOR    	 			(ECG_3_LEAD_START_SECTOR+ECG_3_LEAD_TOTAL_SECTORS)
#define ECG_12_LEAD_START_SECTOR   			(BP1_START_SECTOR+BP1_TOTAL_SECTORS)
#define BP2_START_SECTOR					(ECG_12_LEAD_START_SECTOR + ECG_12_LEAD_TOTAL_SECTORS)


#define OFFLINE_CONFIG_RECORD_START_SECTOR  488 	//968

#define ONE_OFFLINE_RECORD_LENGTH    		19297  // 20480  //Actual record length = 19297, Rounding off to 5 sectors
#define RECORD_SUMMARY_LENGTH               48

#define OFFLINE_RECORD_SUMMARY_OFFSET 		0
#define OFFLINE_BP_OFFSET					48 
#define OFFLINE_BG_OFFSET					56	 
#define OFFLINE_SPO2_OFFSET					64
#define OFFLINE_SENSETEMP_OFFSET			72
#define OFFLINE_ECG_1LEAD_OFFSET			80	
#define OFFLINE_ECG_6LEAD_OFFSET			88

#define OFFLINE_LEAD1_OFFSET				96
#define OFFLINE_LEAD2_OFFSET				2496
#define OFFLINE_V1_LEAD_OFFSET				4896
#define OFFLINE_V2_LEAD_OFFSET				7296
#define OFFLINE_V3_LEAD_OFFSET				9696
#define OFFLINE_V4_LEAD_OFFSET				12096
#define OFFLINE_V5_LEAD_OFFSET				14496
#define OFFLINE_V6_LEAD_OFFSET				16896

/* Start and End sector address for each vital records */

#define DATA_POINTERS_START_ADDR			(DATA_POINTERS_START_SECTOR * FLASH_SECTOR_SIZE)
#define DATA_POINTERS_END_ADDR				((DATA_POINTERS_START_SECTOR + DATA_POINTERS_TOTAL_SECTOR) * FLASH_SECTOR_SIZE)

#define PID_START_ADDR						(PID_START_SECTOR * FLASH_SECTOR_SIZE)
#define PID_END_ADDR						((PID_START_SECTOR + PID_TOTAL_SECTOR) * FLASH_SECTOR_SIZE)

#define BG_START_ADDR  	 					(BG_START_SECTOR * FLASH_SECTOR_SIZE)
#define BG_END_ADDR    						((BG_START_SECTOR + BG_TOTAL_SECTORS) * FLASH_SECTOR_SIZE)

#define TEMP_START_ADDR  	 				(TEMP_START_SECTOR * FLASH_SECTOR_SIZE)
#define TEMP_END_ADDR    					((TEMP_START_SECTOR + TEMP_TOTAL_SECTORS) * FLASH_SECTOR_SIZE)

#define SPO2_START_ADDR  	 				(SPO2_START_SECTOR * FLASH_SECTOR_SIZE)
#define SPO2_END_ADDR    					((SPO2_START_SECTOR + SPO2_TOTAL_SECTORS) * FLASH_SECTOR_SIZE)

#define BP1_START_ADDR  	 				(BP1_START_SECTOR * FLASH_SECTOR_SIZE)
#define BP1_END_ADDR    					((BP1_START_SECTOR + BP1_TOTAL_SECTORS) * FLASH_SECTOR_SIZE)

#define BP2_START_ADDR  	 				(BP2_START_SECTOR * FLASH_SECTOR_SIZE)
#define BP2_END_ADDR    					((BP2_START_SECTOR + BP2_TOTAL_SECTORS) * FLASH_SECTOR_SIZE)

#define ECG_1_LEAD_START_ADDR  				(ECG_1_LEAD_START_SECTOR * FLASH_SECTOR_SIZE)
#define ECG_1_LEAD_END_ADDR    				((ECG_1_LEAD_START_SECTOR + ECG_1_LEAD_TOTAL_SECTORS) * FLASH_SECTOR_SIZE)

#define ECG_3_LEAD_START_ADDR  				(ECG_3_LEAD_START_SECTOR * FLASH_SECTOR_SIZE)
#define ECG_3_LEAD_END_ADDR    				((ECG_3_LEAD_START_SECTOR + ECG_3_LEAD_TOTAL_SECTORS) * FLASH_SECTOR_SIZE)

#define ECG_12_LEAD_START_ADDR   			(ECG_12_LEAD_START_SECTOR * FLASH_SECTOR_SIZE)
#define ECG_12_LEAD_END_ADDR    			((ECG_12_LEAD_START_SECTOR + ECG_12_LEAD_TOTAL_SECTORS) * FLASH_SECTOR_SIZE)

/*************************** OFFline records ****************************/

#define OFFLINE_CONFIG_RECORD_START_ADDR	(OFFLINE_CONFIG_RECORD_START_SECTOR * FLASH_SECTOR_SIZE)
#define OFFLINE_CONFIG_RECORD_END_ADDR		(OFFLINE_CONFIG_RECORD_START_SECTOR + OFFLINE_CONFIG_RECORD_TOTAL_SECTORS) * FLASH_SECTOR_SIZE

/*------------------------------------- ENUM -------------------------------------------------------------*/

typedef enum __attribute__((__packed__))
{
	BG,
	SENSE_TEMP,
	SPO2,
	BP1,
	BP2,
	ECG_1_Lead,
	ECG_6_Lead,
	ECG_12_LEAD,
	PID,
	ECG_OFFLINE,
	QUICK_TEST,
	VIEW_TEST,
	EXIT
} VITAL_TYPE_t;

typedef enum __attribute__((__packed__))
{
	NO_RECORDS_IN_FLASH,
	NO_SPACE_IN_FLASH,
	PID_REGISTRATION_LIMIT_REACHED,
	READ_RECORDS_SUCCESS,
	WRITE_RECORDS_SUCCESS,
	ERASE_RECORD_SUCCESS,
	RECORDS_UPDATE_SUCCESS,
	RECORDS_UPDATE_FAILED,
}RECORD_OPS_STATUS;

typedef enum __attribute__((__packed__))
{
	POC_1P0_WITH_DISP,
	POC_2P0_WITHOUT_DISP,
	PROTO_1P0_HANDHELD,
	SENSE_H3P0,
} DEVICE_TYPE_t;

typedef enum __attribute__((__packed__))
{
	DEVICE_ACTIVE_MODE,
	DEVICE_SLEEP_MODE,
	DEVICE_SEMI_SLEEP_MODE,
	DEVICE_HIBERNET_MODE
} SYSTEM_MODE_t;

typedef enum __attribute__((__packed__))
{
	RECORD_SUMMARY = 0,
	BG_DATA,
	BP1_DATA,
	BP2_DATA,
	SENSETEMP_DATA,
	SPO2_DATA,
	ECG_1LEAD,
	ECG_6LEAD,
	ECG_LEAD1,
	ECG_LEAD2,
	ECG_LEAD_CNT,
	ECG_V1_LEAD,
	ECG_V2_LEAD,
	ECG_V3_LEAD,
	ECG_V4_LEAD,
	ECG_V5_LEAD,
	ECG_V6_LEAD,
} TEST_TYPE_OFFLINE_CONFIG_t;

typedef enum __attribute__((__packed__))
{
   INDIVIDUAL,
   HOSPITAL
} DEVICE_CONFIG_t;

/*------------------------------------- STRUCTS -------------------------------------------------------------*/

typedef struct __attribute__((__packed__))
{
	uint32_t  		record_len;
	uint8_t         firmware_version[10];
	DEVICE_TYPE_t 	device_type;
	uint32_t  		device_id;
	uint8_t  		patient_id[18];
	uint8_t  		day;
	uint8_t  		month;
	uint8_t  		year;
	uint8_t  		hour;
	uint8_t  		minute;
	uint8_t  		second;
	VITAL_TYPE_t  	test_type;
	uint16_t 		sampling_frequency;
	uint32_t 		nbr_samples;
	uint32_t 		calculated_Data;
}RECORD_HEADER_STRUCT_t;

typedef struct __attribute__((__packed__))
{
	uint8_t pid[18];
	bool     male_female;
	uint8_t  age;
} PID_STRUCT;

typedef struct __attribute__((__packed__))
{
	uint32_t registration_config	:1;//hospital/individual configyration
	uint32_t offline_support_status :1;// OFFLINE CONFIGURATION to store the data in flash
	uint32_t spare					:30;
	uint8_t tests_finished;
}DEVICE_CONFIG;

typedef struct __attribute__((__packed__))
{
	uint32_t		record_marker;
	uint16_t 		first_time_power_up_value;
	SYSTEM_MODE_t	sys_mode;
	DEVICE_CONFIG 	device_config;
	uint32_t 		bg_write_addr;
	uint32_t 		bg_read_addr;
	uint32_t 		temp_write_addr;
	uint32_t 		temp_read_addr;
	uint32_t 		spo2_write_addr;
	uint32_t 		spo2_read_addr;
	uint32_t 		bp1_write_addr;
	uint32_t 		bp1_read_addr;
	uint32_t 		bp2_write_addr;
	uint32_t 		bp2_read_addr;
	uint32_t 		ecg_1_lead_write_addr;
	uint32_t 		ecg_1_lead_read_addr;
	uint32_t 		ecg_3_lead_write_addr;
	uint32_t 		ecg_3_lead_read_addr;
	uint32_t		ecg_12_lead_write_addr;
	uint32_t		ecg_12_lead_read_addr;
	uint32_t 		pid_write_addr;
	uint32_t 		pid_read_addr;
	//these test count are related to off-line
	uint32_t		offline_config_record_write_addr;
	uint32_t		offline_config_record_read_addr;
	bool			offline_config_record_new;
} DATA_STORE_FLASH_STRUCT_t;

typedef struct __attribute__((__packed__))
{
	uint32_t write_addr;
	uint32_t read_addr;
	uint32_t start_addr;
	uint32_t end_addr;
	uint32_t *new_write_addr;
	uint32_t *new_read_addr;
	uint32_t one_record_len;
}RECORD_POINTER_t;

typedef struct __attribute__((__packed__))
{
	uint32_t  		record_len;
	uint8_t         firmware_version[10];
	DEVICE_TYPE_t 	device_type;
	uint32_t  		device_id;
	uint8_t  		patient_id[18];
	uint8_t  		day;
	uint8_t  		month;
	uint8_t  		year;
}RECORD_HEADER_STRUCT_OFFLINE_CONFIG;


typedef struct __attribute__((__packed__))
{
	uint16_t bp1_records;
	uint16_t bp2_records;
	uint16_t bg_records;
	uint16_t spo2_records;
	uint16_t temp_records;
	uint16_t ecg_1_records;
	uint16_t ecg_3_records;
	uint16_t ecg_12_lead_ecord;
	uint16_t offline_config_records;
}  TOTAL_RECORDS_STRUCT_t;

typedef struct __attribute__((__packed__))
{
	uint8_t  		hour;
	uint8_t  		minute;
	uint8_t  		second;
	VITAL_TYPE_t  	test_type;
	uint32_t		test_result;
} OFFLINE_RECORD_TESTFIELD;


typedef enum __attribute__((__packed__))
{
	BloodPressure1,
	BloodPressure2,
	BloodGlucose,
	ECGSingleLead,
	ECGFilterDataLead1,
	ECGLead1,
	ECGLead3,
	ECGLead6,
	BloodOxygen,
	BodyTemperature,
	ECGLead12,
	RespiratoryRate,

}Test_Type_t;

typedef struct __attribute__((__packed__))
{
  float TEMP;
  uint16_t SBP1;
  uint16_t DBP1;
  uint16_t SBP2;
  uint16_t DBP2;
  uint16_t HR;
  uint16_t BG;
  uint16_t SPO2;
  uint16_t Lead12;

}VITAL_RESULT_t;
/*---------------------------------- GLOBAL VARIABLES -----------------------------------------------------*/
extern DATA_STORE_FLASH_STRUCT_t flash_data;
extern TOTAL_RECORDS_STRUCT_t total_records;
extern RECORD_HEADER_STRUCT_t record_header;
extern VITAL_RESULT_t Vital_result;
extern PID_STRUCT PatientID;

extern RECORD_HEADER_STRUCT_OFFLINE_CONFIG offline_record_header;
extern OFFLINE_RECORD_TESTFIELD test_info;
extern VITAL_TYPE_t test_type;

extern uint8_t hospital_pid[MAX_PID_RECORDS];
extern uint8_t old_pid[MAX_PID_RECORDS];
extern uint8_t new_pid[MAX_PID_RECORDS];
extern bool IsValidRecordsInFlash;
/*---------------------------------- GLOBAL FUNCTIONS -----------------------------------------------------*/
uint8_t API_Flash_Initialize_Data_pointers(void);
bool Flash_Pointers_Update(void);
void get_record_pointer_details (VITAL_TYPE_t vital, RECORD_POINTER_t *record_pointer);
uint32_t get_records_count(VITAL_TYPE_t vital_type);
RECORD_OPS_STATUS API_Flash_Write_Record(VITAL_TYPE_t vital_type, void* source_buffer);
RECORD_OPS_STATUS API_Flash_Read_Record(VITAL_TYPE_t vital, void *read_buffer);
void erase_one_record(VITAL_TYPE_t vital_type);
void  API_Get_Total_Record_Count (TOTAL_RECORDS_STRUCT_t *total_records);
void set_device_config(bool status);
bool get_device_configuration_status();
void API_Update_Record_Header(VITAL_TYPE_t type, RECORD_HEADER_STRUCT_t  *record_header);
uint16_t  Get_all_PIDs(void *buffer);
void  Sort_PIDS_and_Store_in_Flash(uint8_t buff[]);
void API_Udate_Test_Field(VITAL_TYPE_t type, uint32_t result);

/*------------------------------------Offline records managing functions------------------------ */
void API_Set_Offline_Config(bool status);
uint8_t API_Get_Offline_Config_Records_Count(void);
void API_Update_Offline_Record_Header(RECORD_HEADER_STRUCT_OFFLINE_CONFIG *record_header_offline);
RECORD_OPS_STATUS API_Flash_Write_Offline_Config_Record(TEST_TYPE_OFFLINE_CONFIG_t test_type, void* source_buffer);
RECORD_OPS_STATUS API_Flash_Read_Offline_Config_Records(void *read_buffer);
void API_Flash_Pointer_Update_Offline_Config(void);
void API_Flash_Remove_Offline_Config_Records(void);
void get_offline_config_test_offset_and_length (TEST_TYPE_OFFLINE_CONFIG_t test_type, uint32_t *offset, uint32_t *record_len);


bool API_Flash_Org_Check_For_Memory_Free(void);
void API_Flash_Check_Update_Valid_Record_Status(void);

#endif /* API_API_FLASH_ORG_H_ */
