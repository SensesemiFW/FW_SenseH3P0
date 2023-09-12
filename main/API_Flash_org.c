#include "API_Flash_org.h"
#include "rtc.h"
#include "API_Flash.h"
#include "API_utility.h"
#include "string.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Firmware_version.h"
#include "ProjectConfiguration.h"

#define RECORD_MARKER_VALUE			0xABCD

DATA_STORE_FLASH_STRUCT_t flash_data;
TOTAL_RECORDS_STRUCT_t total_records;
RECORD_HEADER_STRUCT_t record_header;
VITAL_RESULT_t Vital_result;
PID_STRUCT PatientID;
bool IsValidRecordsInFlash;

uint8_t API_Flash_Initialize_Data_pointers(void)
{
	API_FLASH_Read(DATA_POINTERS_START_ADDR, &flash_data, sizeof(flash_data));

	//if(1)
	if(flash_data.first_time_power_up_value != 0x55AA)
	{
			flash_data.record_marker				= RECORD_MARKER_VALUE;
			flash_data.first_time_power_up_value 	= 0x55AA;
			flash_data.sys_mode						= DEVICE_ACTIVE_MODE;

			flash_data.bg_write_addr				= BG_START_ADDR;
			flash_data.bg_read_addr					= BG_START_ADDR;

			flash_data.temp_write_addr				= TEMP_START_ADDR;
			flash_data.temp_read_addr				= TEMP_START_ADDR;

			flash_data.spo2_write_addr				= SPO2_START_ADDR;
			flash_data.spo2_read_addr				= SPO2_START_ADDR;

			flash_data.bp1_write_addr				= BP1_START_ADDR;
			flash_data.bp1_read_addr				= BP1_START_ADDR;

			flash_data.bp2_write_addr				= BP2_START_ADDR;
			flash_data.bp2_read_addr				= BP2_START_ADDR;

			flash_data.ecg_1_lead_write_addr		= ECG_1_LEAD_START_ADDR;
		    flash_data.ecg_1_lead_read_addr			= ECG_1_LEAD_START_ADDR;

			flash_data.ecg_3_lead_write_addr		= ECG_3_LEAD_START_ADDR;
			flash_data.ecg_3_lead_read_addr			= ECG_3_LEAD_START_ADDR;

			flash_data.ecg_12_lead_write_addr		= ECG_12_LEAD_START_ADDR;
			flash_data.ecg_12_lead_read_addr		= ECG_12_LEAD_START_ADDR;

			flash_data.pid_write_addr				= PID_START_ADDR;
			flash_data.pid_read_addr				= PID_START_ADDR;

			flash_data.offline_config_record_write_addr	= OFFLINE_CONFIG_RECORD_START_ADDR;
			flash_data.offline_config_record_read_addr 	= OFFLINE_CONFIG_RECORD_END_ADDR;
			flash_data.offline_config_record_new	= true;
		}


		if (Flash_Pointers_Update() == true){ //write these values in flash for persistant storage
			return RECORDS_UPDATE_SUCCESS;
		}

	return RECORDS_UPDATE_FAILED;
}
	
bool Flash_Pointers_Update(void)
{
    bool flash_update_status  = FALSE;

    if(API_FLASH_Sector_Erase(DATA_POINTERS_START_ADDR, FLASH_SECTOR_SIZE) == FLASH_SUCCESS){
        if(API_FLASH_Write(DATA_POINTERS_START_ADDR, &flash_data, sizeof(flash_data)) == FLASH_WRITE_SUCCESS) {
            flash_update_status  = TRUE;
        }
        else
          {
          	printf("\n Fail:API_FLASH_Write");
          }
    }

    else
    {
    	printf("\n Fail:API_FLASH_Sector_Erase");
    }

    return flash_update_status;
}

void get_record_pointer_details (VITAL_TYPE_t vital, RECORD_POINTER_t *record_pointer)
{
	switch(vital){
		case BG:
			record_pointer->read_addr = flash_data.bg_read_addr;
			record_pointer->write_addr = flash_data.bg_write_addr;
			record_pointer->new_write_addr = &flash_data.bg_write_addr;
			record_pointer->new_read_addr = &flash_data.bg_read_addr;
			record_pointer->start_addr = BG_START_ADDR;
			record_pointer->end_addr = BG_END_ADDR;
			record_pointer->one_record_len = BG_ONE_REC_LEN;
			break;
		case BP1:
			record_pointer->write_addr = flash_data.bp1_write_addr;
			record_pointer->read_addr = flash_data.bp1_read_addr;
			record_pointer->new_write_addr = &flash_data.bp1_write_addr;
			record_pointer->new_read_addr = &flash_data.bp1_read_addr;
			record_pointer->start_addr = BP1_START_ADDR;
			record_pointer->end_addr = BP1_END_ADDR;
			record_pointer->one_record_len = BP1_ONE_REC_LEN;
			break;
		case BP2:
			record_pointer->write_addr = flash_data.bp2_write_addr;
			record_pointer->read_addr = flash_data.bp2_read_addr;
			record_pointer->new_write_addr = &flash_data.bp2_write_addr;
			record_pointer->new_read_addr = &flash_data.bp2_read_addr;
			record_pointer->start_addr = BP2_START_ADDR;
			record_pointer->end_addr = BP2_END_ADDR;
			record_pointer->one_record_len = BP2_ONE_REC_LEN;
			break;
		case SPO2:
			record_pointer->write_addr = flash_data.spo2_write_addr;
			record_pointer->read_addr = flash_data.spo2_read_addr;
			record_pointer->new_write_addr = &flash_data.spo2_write_addr;
			record_pointer->new_read_addr = &flash_data.spo2_read_addr;
			record_pointer->start_addr = SPO2_START_ADDR;
			record_pointer->end_addr = SPO2_END_ADDR;
			record_pointer->one_record_len = SPO2_ONE_REC_LEN;
			break;
		case SENSE_TEMP: 
			record_pointer->write_addr = flash_data.temp_write_addr;
			record_pointer->read_addr = flash_data.temp_read_addr;
			record_pointer->new_write_addr = &flash_data.temp_write_addr;
			record_pointer->new_read_addr = &flash_data.temp_read_addr;
			record_pointer->start_addr = TEMP_START_ADDR;
			record_pointer->end_addr = TEMP_END_ADDR;
			record_pointer->one_record_len = TEMP_ONE_REC_LEN;
			break;

		case ECG_1_Lead:
			record_pointer->write_addr = flash_data.ecg_1_lead_write_addr;
			record_pointer->read_addr = flash_data.ecg_1_lead_read_addr;
			record_pointer->new_write_addr = &flash_data.ecg_1_lead_write_addr;
			record_pointer->new_read_addr = &flash_data.ecg_1_lead_read_addr;
			record_pointer->start_addr = ECG_1_LEAD_START_ADDR;
			record_pointer->end_addr = ECG_1_LEAD_END_ADDR;
			record_pointer->one_record_len = ECG_1_LEAD_REC_LEN;
			break;

		case ECG_6_Lead:
			record_pointer->write_addr = flash_data.ecg_3_lead_write_addr;
			record_pointer->read_addr = flash_data.ecg_3_lead_read_addr;
			record_pointer->new_write_addr = &flash_data.ecg_3_lead_write_addr;
			record_pointer->new_read_addr = &flash_data.ecg_3_lead_read_addr;
			record_pointer->start_addr = ECG_3_LEAD_START_ADDR;
			record_pointer->end_addr = ECG_3_LEAD_END_ADDR;
			record_pointer->one_record_len = ECG_3_LEAD_REC_LEN;
			break;
		case ECG_12_LEAD:
			record_pointer->write_addr = flash_data.ecg_12_lead_write_addr;
			record_pointer->read_addr = flash_data.ecg_12_lead_read_addr;
			record_pointer->new_write_addr = &flash_data.ecg_12_lead_write_addr;
			record_pointer->new_read_addr = &flash_data.ecg_12_lead_read_addr;
			record_pointer->start_addr = ECG_12_LEAD_START_ADDR;
			record_pointer->end_addr = ECG_12_LEAD_END_ADDR;
			record_pointer->one_record_len = ECG_12_LEAD_ONE_REC_LEN;
			break;
		case PID:
			record_pointer->write_addr = flash_data.pid_write_addr;
			record_pointer->read_addr = flash_data.pid_read_addr;
			record_pointer->new_write_addr = &flash_data.pid_write_addr;
			record_pointer->new_read_addr = &flash_data.pid_read_addr;
			record_pointer->start_addr = PID_START_ADDR;
			record_pointer->end_addr = PID_END_ADDR;
			record_pointer->one_record_len = PID_ONE_REC_LEN;
			break;
		default:
			break;
	}
//	printf("Vital type: %d, read_addr: %d, write_addr: %d, start_addr: %d, end_addr: %d, one_record_len: %d\n", vital, record_pointer->read_addr, record_pointer->write_addr, record_pointer->start_addr, record_pointer->end_addr, record_pointer->one_record_len);
}

uint32_t get_records_count(VITAL_TYPE_t vital_type)
{
	RECORD_POINTER_t records_pointer;
	uint32_t records_count = 0;
	
	get_record_pointer_details (vital_type, &records_pointer);

	if (records_pointer.read_addr > records_pointer.write_addr){
		records_count = (records_pointer.end_addr - records_pointer.read_addr) / records_pointer.one_record_len;
		records_count += (records_pointer.start_addr - records_pointer.write_addr) / records_pointer.one_record_len;   
	}
	else {
		records_count = (records_pointer.write_addr - records_pointer.read_addr) / records_pointer.one_record_len;
	} 
//	printf("Vital record count: %d\n", records_count);
	return records_count;
}

/**
 * @brief Save the vital records to internal flash memory.
 *
 * @param[vital_type] Type of the vital record that needs to be saved to flash
 * @param[source_buffer] buffer containing vital record data
 *
 * @return
 *  - WRITE_RECORDS_SUCCESS: Success
 *  - NO_SPACE_IN_FLASH: No space left to store records 
 *  - FLASH_ERASE_FAILED: Erase failed
 *  - Others: Fail
 */
RECORD_OPS_STATUS API_Flash_Write_Record(VITAL_TYPE_t vital_type, void* source_buffer)
{
	RECORD_POINTER_t records_pointer;
	uint32_t *new_write_addr;
	uint32_t temp_write_addr;
	uint32_t erase_sector_addr;
	uint32_t total_bytes_to_erase = 0;
	uint32_t record_count = 0;
	bool erase_required = false;

	record_count = get_records_count(vital_type);
	if((record_count) >= MAX_RECORDS){
		return  NO_SPACE_IN_FLASH;
	}		
	if((vital_type == PID) && (record_count >= MAX_PID_RECORDS)){
		printf("Max PID registeration count reached\n");
		return PID_REGISTRATION_LIMIT_REACHED;
	}
	get_record_pointer_details (vital_type, &records_pointer);

	if (records_pointer.write_addr == records_pointer.start_addr){ //For first time write
		erase_sector_addr = records_pointer.write_addr;
		erase_required = true;
	}
	//check if write address crosses the total record boundary
	else if ((records_pointer.write_addr + records_pointer.one_record_len) >= records_pointer.end_addr){
		//roll over to starting record address
		records_pointer.write_addr = records_pointer.start_addr;
		erase_sector_addr = records_pointer.write_addr;
		erase_required = true;
	}
	//check if number of bytes written will cross the sector boundary
	else if((records_pointer.write_addr + records_pointer.one_record_len) > (((records_pointer.write_addr / FLASH_SECTOR_SIZE) + 1) * FLASH_SECTOR_SIZE)){
		// Select next sector for erasing before writing data 
		erase_sector_addr = ((records_pointer.write_addr / FLASH_SECTOR_SIZE) + 1) * FLASH_SECTOR_SIZE;
		erase_required = true;
	}
	else { 	
		erase_required = false; //No sector erase required
	}			
	if (erase_required){
		//Number of bytes to erase if sector errase is required
		total_bytes_to_erase = ((records_pointer.one_record_len / FLASH_SECTOR_SIZE) + 1) * FLASH_SECTOR_SIZE ;
//		printf ("erase_sector_addr: %d, total_bytes_to_erase: %d\n", records_pointer.write_addr, total_bytes_to_erase);

		if(API_FLASH_Sector_Erase(erase_sector_addr, total_bytes_to_erase) != FLASH_SUCCESS) {
			printf("Flash write error\n");
			return FLASH_ERASE_FAILED;
		}
	}
	if (API_FLASH_Write(records_pointer.write_addr, source_buffer, records_pointer.one_record_len) == FLASH_WRITE_SUCCESS){
		//update record write address for successful write operation
		new_write_addr = records_pointer.new_write_addr;
		temp_write_addr = records_pointer.write_addr + records_pointer.one_record_len;
		if(temp_write_addr >= records_pointer.end_addr){
			*new_write_addr = records_pointer.start_addr;
		}
		else{
			*new_write_addr = temp_write_addr; //Else point to next record
		}
	}
	Flash_Pointers_Update();
	return WRITE_RECORDS_SUCCESS;
}

/**
 * @brief retrive the vital records from internal flash memory.
 *
 * @param[vital] Type of the vital record that needs to be retrived
 * @param[source_buffer] buffer big enough to hold requested vital record data
 *
 * @return
 *  - READ_RECORDS_SUCCESS: on success
 *  - NO_RECORDS_IN_FLASH: No records available in flash for requested vital type  
 *  - FLASH_READ_FAILED: flash read failure
 *  - Others: Fail
 */
RECORD_OPS_STATUS API_Flash_Read_Record(VITAL_TYPE_t vital, void *read_buffer)
{
	RECORD_POINTER_t records_pointer;
	uint8_t ret_val;

	if (get_records_count(vital)){
    	get_record_pointer_details (vital, &records_pointer);	
		//Read BG record into the buffer 
		ret_val = API_FLASH_Read(records_pointer.read_addr, read_buffer, records_pointer.one_record_len);
		//Not incrementing the flash read addr counter, need read pointer in case of unsuccessful record sync 
		//Read pointer is updated in function erase_one_record()	
		if(ret_val == FLASH_READ_SUCCESS){
			return READ_RECORDS_SUCCESS;
		}
		else 
			return ret_val; 
	}	
	return 	NO_RECORDS_IN_FLASH;
}


/**
 * @brief remove one vital record from internal flash memory.
 * @param[vital_type] Type of the vital record that needs to be deleted from flash
 */
void erase_one_record(VITAL_TYPE_t vital_type)
{
	RECORD_POINTER_t records_pointer;
	uint32_t *new_read_addr;
	uint32_t temp_read_addr;

	if(!get_records_count(vital_type)){
		return;
	}
   	get_record_pointer_details (vital_type, &records_pointer);	

	new_read_addr = records_pointer.new_read_addr;
	temp_read_addr = records_pointer.read_addr + records_pointer.one_record_len;
 	//Roll over record read pointer to start of record 
	if(temp_read_addr >= records_pointer.end_addr){
		*new_read_addr = records_pointer.start_addr;
	}
	else {
		*new_read_addr = temp_read_addr; //Else point to next record
	}
	Flash_Pointers_Update();

	API_Flash_Check_Update_Valid_Record_Status();
}

/**
 * @brief report number of records of all vital saved in flash
 *
 * @param[total_records] pointer to sturct of type TOTAL_RECORDS_STRUCT to hold number of records 
 */
void  API_Get_Total_Record_Count (TOTAL_RECORDS_STRUCT_t *total_records)
{
	//BG
	total_records->bg_records = get_records_count(BG);
	//SenseTemp
	total_records->temp_records = get_records_count(SENSE_TEMP);
	//SPO2
	total_records->spo2_records = get_records_count(SPO2);
	//BP1
	total_records->bp1_records = get_records_count(BP1);
	//BP2
	total_records->bp2_records = get_records_count(BP2);

	total_records->ecg_1_records = get_records_count(ECG_1_Lead);
	//3 lead ecg record count
	total_records->ecg_3_records = get_records_count(ECG_6_Lead);
	//12 lead ecg record count
	total_records->ecg_12_lead_ecord = get_records_count(ECG_12_LEAD);
	total_records->offline_config_records = API_Get_Offline_Config_Records_Count();

}

void set_device_config(bool status)
{
	if(status){
		flash_data.device_config.registration_config = 1;
	}
	else if(status==FALSE){
		flash_data.device_config.registration_config = 0;
	}
	Flash_Pointers_Update();
}

bool get_device_configuration_status()
{
	bool status;

	if(flash_data.device_config.registration_config == 1){
		status=TRUE;
	}
	else if(flash_data.device_config.registration_config == 0){
		status=FALSE;
	}
	return status;
}


void API_Update_Record_Header(VITAL_TYPE_t type, RECORD_HEADER_STRUCT_t  *record_header)
{
	char date[20]={0};
	
	API_RTC_Get_Date_Time(date,"%d");
	record_header->day = atoi(date);

	API_RTC_Get_Date_Time(date,"%m");
	record_header->month = atoi(date);

	API_RTC_Get_Date_Time(date,"%y");
	record_header->year	 = atoi(date);

	API_RTC_Get_Date_Time(date,"%H");
	record_header->hour	 = atoi(date);

	API_RTC_Get_Date_Time(date,"%M");
	record_header->minute = atoi(date);

	API_RTC_Get_Date_Time(date,"%S");
	record_header->second = atoi(date);

	record_header->device_id = DEVICE_ID;

	record_header->device_type = SENSE_H3P0;

	MemCpy(record_header->patient_id, PatientID.pid ,sizeof(PatientID.pid));

	MemCpy(record_header->firmware_version, FIRMWARE_VERSION, sizeof(record_header->firmware_version));
	
	switch (type){
		case BG:
			record_header->record_len = BG_ONE_REC_LEN; 
			record_header->nbr_samples = 1;
			record_header->calculated_Data = Vital_result.BG;
			record_header->test_type=BloodGlucose;
			break;
		case SENSE_TEMP:
			record_header->record_len = TEMP_ONE_REC_LEN;
			record_header->nbr_samples = 1;
			record_header->calculated_Data =Vital_result.TEMP;
			record_header->test_type=BodyTemperature;
			break;
		case BP1:
			record_header->record_len = BP1_ONE_REC_LEN;
			record_header->sampling_frequency = SAMPLING_FREQUENCY_PPG_ECG;
			record_header->nbr_samples = TOTAL_SAMPLES*4;
			record_header->calculated_Data =  (Vital_result.SBP1 <<16) | (Vital_result.DBP1);
			record_header->test_type=BloodPressure1;
			break;
		case BP2:
			record_header->record_len = BP2_ONE_REC_LEN;
			record_header->sampling_frequency = SAMPLING_FREQUENCY_PPG_ECG;
			record_header->nbr_samples = TOTAL_SAMPLES*4;
			record_header->calculated_Data =  (Vital_result.SBP2 <<16) | (Vital_result.DBP2);
			record_header->test_type=BloodPressure2;
			break;

		case ECG_1_Lead:
			record_header->record_len = ECG_1_LEAD_REC_LEN;
			record_header->sampling_frequency = SAMPLING_FREQUENCY_PPG_ECG;
			record_header->nbr_samples = TOTAL_SAMPLES*3;
			record_header->calculated_Data = Vital_result.HR;
			record_header->test_type=ECGLead1;

			break;

		case ECG_6_Lead:
			record_header->record_len = ECG_3_LEAD_REC_LEN;
			record_header->sampling_frequency = SAMPLING_FREQUENCY_PPG_ECG;
			record_header->nbr_samples = TOTAL_SAMPLES*3;
			record_header->calculated_Data = Vital_result.HR;
			record_header->test_type=ECGLead3;

			break;

		case ECG_12_LEAD:
			record_header->record_len = ECG_12_LEAD_ONE_REC_LEN;
			record_header->sampling_frequency = SAMPLING_FREQUENCY_PPG_ECG;
			record_header->nbr_samples = ((ECG_12_LEAD_ONE_REC_LEN-REC_HEADER_LEN)/4);
			record_header->calculated_Data = Vital_result.HR;
			record_header->test_type=ECGLead12;

			break;

		case SPO2:
			record_header->record_len = SPO2_ONE_REC_LEN;
			record_header->sampling_frequency = SAMPLING_FREQUENCY_PPG_ECG;
			record_header->nbr_samples = TOTAL_SAMPLES*2;
			record_header->calculated_Data = Vital_result.SPO2;
			record_header->test_type=BloodOxygen;

			break;
		case PID:
			record_header->record_len = PID_ONE_REC_LEN;
			break;
		default:
			break;
	}	
//	printf("Rec. len: %d, device type: %d, device ID: %d, day: %d, month: %d, year: %d, Test type: %d, Sampling feq: %d\n", record_header->record_len, record_header->device_type, record_header->device_id, record_header->day, record_header->month, record_header->year, record_header->test_type, record_header->sampling_frequency);
}

uint16_t  Get_all_PIDs(void *buffer)
{
    uint16_t pids_count = 0;

	pids_count = get_records_count (PID);
    API_FLASH_Read(PID_START_ADDR, buffer, pids_count * PID_ONE_REC_LEN);

    return pids_count;
}

void Deregister_Old_PID_add_NewPID(uint8_t old_pid[], uint8_t new_pid[])
{
	uint8_t total_PIDS = 0;
	uint8_t stored_pid[PID_ONE_REC_LEN * MAX_PID_RECORDS] = {0};
	uint8_t inavlid_PID[PID_ONE_REC_LEN] = {0x00};
	uint8_t new_VALID_PIDity = 0xFF;
	int i;

	new_VALID_PIDity = memcmp(new_pid, inavlid_PID, sizeof(inavlid_PID));
	total_PIDS = get_records_count(PID);

	Get_all_PIDs(stored_pid);

	for(i=0; i < total_PIDS; i++){
		if(memcmp((stored_pid + (PID_ONE_REC_LEN * i)), old_pid, PID_ONE_REC_LEN) == 0) {
			MemCpy((stored_pid + (PID_ONE_REC_LEN * i)), new_pid, PID_ONE_REC_LEN);

			if(new_VALID_PIDity == 0x00 ){ // Condition will become if PID is invalid
            	Sort_PIDS_and_Store_in_Flash(stored_pid);
			}
			break;
		}
	}
}

void  Sort_PIDS_and_Store_in_Flash(uint8_t buff[])
{
	uint8_t write_buff[PID_ONE_REC_LEN * MAX_PID_RECORDS] = {0x00};
	uint8_t cmp_buff[PID_ONE_REC_LEN] = {0x00};
	uint8_t total_valid_PIDS = 0x00;
	uint8_t cmp_output = 0x00;
    uint8_t index1 = 0x00;
    uint8_t index2 = 0x00;
    uint8_t count = 0;

	API_FLASH_Sector_Erase(PID_START_ADDR, PID_TOTAL_SECTOR * FLASH_SECTOR_SIZE);

	for(count = 0; count < MAX_PID_RECORDS; count++) {
		cmp_output = memcmp((buff + (index1 * PID_ONE_REC_LEN)), cmp_buff, PID_ONE_REC_LEN);
        if(cmp_output == 0){
        	index1 ++;
        }
        else {
			MemCpy((write_buff + (index2 * PID_ONE_REC_LEN)), (buff + (index1 * PID_ONE_REC_LEN)),PID_ONE_REC_LEN);
			index2 ++;
			index1 ++;
			total_valid_PIDS ++;
        }
	}
	flash_data.pid_write_addr = PID_START_ADDR;
	flash_data.pid_read_addr = PID_START_ADDR;

	API_FLASH_Write(flash_data.pid_write_addr, write_buff,(total_valid_PIDS * PID_ONE_REC_LEN) );

	flash_data.pid_write_addr += (total_valid_PIDS * PID_ONE_REC_LEN);
	Flash_Pointers_Update();
}


/*
* void API_update_test_field(VITAL_TYPE_t type , uint32_t result)
* \brief		Function to update vital test field
* param[in]		Test type, result
* \retval      	void
*/
void API_Udate_Test_Field(VITAL_TYPE_t type, uint32_t result)
{
	char date[50];
	
	memset(date,'\0',sizeof(date));	
	API_RTC_Get_Date_Time(date,"%d/%m/%y %H:%M:%S");

	test_info.hour	= atoi(&date[9]);
	test_info.minute = atoi(&date[12]);
	test_info.second = atoi(&date[15]);

	test_info.test_type	= type;
	test_info.test_result = result;
}

void API_Set_Offline_Config(bool status)
{
	 if(status == TRUE) {
		 flash_data.device_config.offline_support_status = 1;
	 }
	 else {
		 flash_data.device_config.offline_support_status = 0;
	 }
	 Flash_Pointers_Update();
}

/*
* uint8_t API_Get_Offline_Config_Records_Count(void)
* \brief		Function to get the record count for off-line conf
* param[in]		void
* \retval      	Record Count
*/
uint8_t API_Get_Offline_Config_Records_Count(void)
{
	uint8_t record_count = 0;

	if(flash_data.offline_config_record_write_addr == flash_data.offline_config_record_read_addr) {
		record_count = 0;
	}
	else if(flash_data.offline_config_record_write_addr > flash_data.offline_config_record_read_addr) {
		record_count = (flash_data.offline_config_record_write_addr - flash_data.offline_config_record_read_addr)/ONE_OFFLINE_RECORD_LENGTH;
	}
	else {
		record_count = (OFFLINE_CONFIG_RECORD_END_ADDR - flash_data.offline_config_record_read_addr)/ONE_OFFLINE_RECORD_LENGTH;
		record_count += (flash_data.offline_config_record_write_addr - OFFLINE_CONFIG_RECORD_START_ADDR)/ONE_OFFLINE_RECORD_LENGTH;
	}
	return record_count;
}


/*
* void API_update_record_header_offline(RECORD_HEADER_STRUCT  *record_header)
* \brief		function to update the offline record header
* param[in]		void
* \retval      	void
*/
void API_Update_Offline_Record_Header(RECORD_HEADER_STRUCT_OFFLINE_CONFIG *record_header_offline)
{
	char date[50];

	memset(date,'\0',sizeof(date));	
	API_RTC_Get_Date_Time(date,"%d/%m/%y %H:%M:%S");

	record_header_offline->day	= atoi(&date[0]);
	record_header_offline->month = atoi(&date[3]);
	record_header_offline->year	= atoi(&date[6]);

	record_header_offline->record_len = ONE_OFFLINE_RECORD_LENGTH;

//	record_header_offline->device_id	= DEVINFO->UNIQUEL;
//	record_header_offline->device_type = PROTO_1P0_HANDHELD;
	MemCpy(record_header_offline->firmware_version, FIRMWARE_VERSION, 10);
	MemCpy(record_header_offline->patient_id, hospital_pid, 10);
}

/*------------------------------------Offline records managing functions------------------------ */

RECORD_OPS_STATUS API_Flash_Write_Offline_Config_Record(TEST_TYPE_OFFLINE_CONFIG_t test_type, void* source_buffer)
{
	uint32_t write_addr;
	uint32_t record_count;
	uint32_t test_type_offset;
	uint32_t test_type_length;
	uint32_t erase_addr;

	record_count = API_Get_Offline_Config_Records_Count();
	if((record_count) >= MAX_RECORDS){
		return  NO_SPACE_IN_FLASH;
	}		
	//Errase flash sectors for new records writes
	if (flash_data.offline_config_record_new){ 
		erase_addr = (((flash_data.offline_config_record_write_addr / FLASH_SECTOR_SIZE) + 1) * FLASH_SECTOR_SIZE);
		if(API_FLASH_Sector_Erase(erase_addr, (5 * FLASH_SECTOR_SIZE)) != FLASH_SUCCESS) {
			printf("Flash erase failed\n");
			return FLASH_ERASE_FAILED;
		}
		flash_data.offline_config_record_new = false;
	}

	get_offline_config_test_offset_and_length (test_type, &test_type_offset, &test_type_length);
	write_addr = flash_data.offline_config_record_write_addr + test_type_offset;

	if (API_FLASH_Write(write_addr, source_buffer, test_type_length) != FLASH_WRITE_SUCCESS){
		printf("Flash Write failed\n");
		return FLASH_WRITE_FAILED;
	}
	printf("Fash write success\n");
	return FLASH_SUCCESS;
}

RECORD_OPS_STATUS API_Flash_Read_Offline_Config_Records(void *read_buffer)
{
	uint8_t ret_val;

	ret_val = API_FLASH_Read(flash_data.offline_config_record_write_addr, read_buffer, ONE_OFFLINE_RECORD_LENGTH);
	if(ret_val == FLASH_READ_SUCCESS){
		return READ_RECORDS_SUCCESS;
	}
	return ret_val; 
}

void get_offline_config_test_offset_and_length (TEST_TYPE_OFFLINE_CONFIG_t test_type, uint32_t *offset, uint32_t *record_len)
{
	switch(test_type) {
		case RECORD_SUMMARY:
			*offset = OFFLINE_RECORD_SUMMARY_OFFSET;
			*record_len = RECORD_SUMMARY_LENGTH;
			break;

		case BG_DATA:
			*offset = OFFLINE_BG_OFFSET;
			*record_len = OFFLINE_BG_LEN;
			break;

		case BP1_DATA:
			*offset = OFFLINE_BP_OFFSET;
			*record_len = OFFLINE_BP1_LEN;
			break;

		case BP2_DATA:
			*offset = OFFLINE_BP_OFFSET;
			*record_len = OFFLINE_BP2_LEN;
			break;

		case SENSETEMP_DATA:
			*offset = OFFLINE_SENSETEMP_OFFSET;
			*record_len = OFFLINE_SENSETEMP_LEN;
			break;

		case SPO2_DATA:
			*offset = OFFLINE_SPO2_OFFSET;
			*record_len = OFFLINE_SPO2_LEN;
			break;

		case ECG_1LEAD:
			*offset = OFFLINE_ECG_1LEAD_OFFSET;
			*record_len = OFFLINE_ECG_1LEAD_LEN;
			break;

		case ECG_6LEAD:
			*offset = OFFLINE_ECG_6LEAD_OFFSET;
			*record_len = OFFLINE_ECG_6LEAD_LEN;
			break;

		case ECG_LEAD1:
			*offset = OFFLINE_LEAD1_OFFSET; 
			*record_len = OFFLINE_LEAD_LEN;
			break;

		case ECG_LEAD2:
			*offset = OFFLINE_LEAD2_OFFSET; 
			*record_len = OFFLINE_LEAD_LEN;
			break;

		case ECG_V1_LEAD:
			*offset = OFFLINE_V1_LEAD_OFFSET;
			*record_len = OFFLINE_VLEAD_LEN;
			break;

		case ECG_V2_LEAD:
			*offset = OFFLINE_V2_LEAD_OFFSET;
			*record_len = OFFLINE_VLEAD_LEN;
			break;

		case ECG_V3_LEAD:
			*offset = OFFLINE_V3_LEAD_OFFSET;
			*record_len = OFFLINE_VLEAD_LEN;
			break;

		case ECG_V4_LEAD:
			*offset = OFFLINE_V4_LEAD_OFFSET;
			*record_len = OFFLINE_VLEAD_LEN;
			break;

		case ECG_V5_LEAD:
			*offset = OFFLINE_V5_LEAD_OFFSET;
			*record_len = OFFLINE_VLEAD_LEN;
			break;

		case ECG_V6_LEAD:
			*offset = OFFLINE_V6_LEAD_OFFSET;
			*record_len = OFFLINE_VLEAD_LEN;
			break;

		default:
			break;
	}
}

void API_Flash_Pointer_Update_Offline_Config(void)
{
	flash_data.offline_config_record_write_addr += ONE_OFFLINE_RECORD_LENGTH;
	flash_data.offline_config_record_new = true;

	if(flash_data.offline_config_record_write_addr >= OFFLINE_CONFIG_RECORD_END_ADDR) {
			flash_data.offline_config_record_write_addr = OFFLINE_CONFIG_RECORD_START_ADDR;
	}
	Flash_Pointers_Update();
}

void API_Flash_Remove_Offline_Config_Records(void)
{
	flash_data.offline_config_record_read_addr += ONE_OFFLINE_RECORD_LENGTH;
	if(flash_data.offline_config_record_read_addr >= OFFLINE_CONFIG_RECORD_END_ADDR) {
			flash_data.offline_config_record_read_addr = OFFLINE_CONFIG_RECORD_START_ADDR;
	}
	Flash_Pointers_Update();
}

bool API_Flash_Org_Check_For_Memory_Free(void)
{
	bool status = false;
	TOTAL_RECORDS_STRUCT_t nbf_records;

	API_Get_Total_Record_Count(&nbf_records);
	if(nbf_records.bg_records < MAX_RECORDS)
		{
			if(nbf_records.bp1_records < MAX_RECORDS)
				{
				if(nbf_records.bp2_records < MAX_RECORDS)
					{
					if(nbf_records.ecg_12_lead_ecord < MAX_RECORDS)
					{
						if(nbf_records.ecg_1_records < MAX_RECORDS)
						{
							if(nbf_records.ecg_3_records < MAX_RECORDS)
								{
								if(nbf_records.spo2_records < MAX_RECORDS)
									{
									if(nbf_records.temp_records < MAX_RECORDS)
										{
											status = true;
										}
									}
								}
						 }
					}
				}
			}
		}

	return status;
}

void API_Flash_Check_Update_Valid_Record_Status(void)
{
	IsValidRecordsInFlash = true;
	TOTAL_RECORDS_STRUCT_t nbf_records;

	API_Get_Total_Record_Count(&nbf_records);

	printf("\n bg:%d\n bp1:%d\n bp2:%d\n ecg12:%d\n ecg1:%d\n ecg3:%d\n spo2:%d\n temp:%d\n\n",nbf_records.bg_records,
				nbf_records.bp1_records,nbf_records.bp2_records,nbf_records.ecg_12_lead_ecord,nbf_records.ecg_1_records,nbf_records.ecg_3_records,
				nbf_records.spo2_records,nbf_records.temp_records);

	if(nbf_records.bg_records == 0)
		{
			if(nbf_records.bp1_records  == 0)
				{
				if(nbf_records.bp2_records  == 0)
				{
				if(nbf_records.ecg_12_lead_ecord  == 0)
					{
						if(nbf_records.ecg_1_records  == 0)
						{
							if(nbf_records.ecg_3_records  == 0)
								{
								if(nbf_records.spo2_records  == 0)
									{
									if(nbf_records.temp_records  == 0)
										{
											IsValidRecordsInFlash = false;

										}
									}
								}
						 }
					}
				}
			}
		}

}
