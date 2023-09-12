#include "bluetooth.h"
#include "API_utility.h"
#include "API_Flash_org.h"
#include "API_Bluetooth.h"
#include "API_Flash.h"
#include "crc_16.h"
#include "rtc.h"

#include "OTA_Upgrade.h"
#include "Firmware_version.h"
#include "ProjectConfiguration.h"
#include "API_Display.h"

/************************************************ MACROS *****************************************************************/
#define BT_RAW_DATA_LENGTH			  	580	 //BT_PACKET_SIZE = BT_RAW_DATA_LENGTH + payload bytes, this should not cross MTU:600   
#define OFFLINE_REC_SUMMARY_LEN			81  /*need to verify */
#define TOTAL_HEADER_LENGTH_OFFLINE		(OFFLINE_REC_SUMMARY_LEN + BT_PACKET_FIELD_LENGTH)//81 BYTES HEADER AND(PID 6 VITAL PARAMETERS RESULTS)+7 BYTES BT
#define NACK_CRC_FRAME_LENGTH			6
#define MAX_RETRY_COUNT   				3
#define RESULT_LENGTH 					2  		//LENGTH OF THE BG AND SENSETEMP DATA
#define CHKSUM_SIZE						2   	//2 BYTES FOR CHKSUM
#define LENGTH_SIZE						2   	// SIZE OF THE LENGTH FIELD
#define CMD_SIZE						1
#define EOR_SIZE						1
#define	SOS_SIZE						1
#define COR_SIZE						1
#define EOS_SIZE						1 		//1 BYTE FOR END OF SESSION
#define NACK_SIZE						1
#define DATA_LENGTH						1
#define BT_PACKET_FIELD_LENGTH			(SOS_SIZE + CMD_SIZE + LENGTH_SIZE + EOS_SIZE + CHKSUM_SIZE)	//SOS+CMD+LENGTH+EOS+CRC
#define BT_PACKET_SIZE				  	(BT_RAW_DATA_LENGTH + BT_PACKET_FIELD_LENGTH)

//#define BT_PROTOCAL_FIRST_BYTE_VALUE    0xC0
//#define LENGTH_OF_ACK_FILED             0X01
//Watchdog
#define BT_ROUND_TRIP_TIME              2 // 2sec Kept maximum
#define BT_EXTRA_PROCESSING_TIME        3 // 3 sec which is relatively large , kept by considering maximum delay encountering during the firmware upgrade
#define VITAL_REC_LEN					2


/******************************static functions*******************************/

static bool bt_send_single_response(VITAL_TYPE_t vital, uint16_t one_record_len);
static bool bt_send_multi_response(VITAL_TYPE_t vital, uint16_t one_record_len);
static void Load_record_header_to_buffer(void);
static void Load_Raw_Data_To_Buffer(uint8_t command);
static void bt_send_ack_or_nack_response(uint8_t command);
static void send_record_snapsot_response(void);
static void store_device_configuration(void);
static void bt_firmware_request_response(void);

static uint8_t  FW_buffer[BUFFER_SIZE];

/*
static void bt_send_device_self_diagnosis_response(void);
static void bt_send_device_full_diagnosis_response(void);
//static void bt_pid_registered_ack(void);
static void bt_copy_and_process_application_data();// copies the application data into the firmware_buffer and the firmware_buffer index will be incremented
static bool patient_id_regristration(void);

static bool bt_bp_extract_calib_factors(void);
void BT_Prepare_device_to_upgrade(void);


//OFFLINE FUNCTIONS
bool bt_send_offline_rec();
void bt_load_transmite_record_header_offline(SYNC_HEADER_STRUCT_OFFLINE_CONFIG * test );
static void send_record_snapshot_responce_offline_config(void);
void Load_Raw_Data_To_Buffer_Offline_Config(uint8_t command);
*/

/********************************************* ENUM DEFINITIONS ********************************************************/

static VITALS_RESPONSE_STATE bt_response_state = INIT_STATE;

/********************************************* GLOBAL VARAIABLES ********************************************************/

static uint32_t total_length_to_send = 0;
static uint8_t bt_tx_buff[600] = {0};
static uint8_t bt_rx_buff[600] = {0};
static uint8_t client_request_cmd = 0; 
static uint16_t bt_total_received_bytes = 0;
static uint16_t count = 0;
static uint16_t skip_count = 0;
static uint32_t remainder = 0;
static uint8_t retry_count = 0;
static char mobile_number[10] = {'\0'};
static bool is_arrived_timesynq_req = FALSE;
static bool is_OTA_request_arrived  = FALSE;
uint8_t BT_flash_buffer[DATA_BUFFER3_LENGTH];
/************************strctures*******************************/

bool FW_complete_data_received = false;
bool Is_Test_In_Progress;
bool BT_ongoing_session;

//#define LOG_LOCAL_LEVEL ESP_LOG_ERROR

/*void BT_receive(void)
* \brief        blue-tooth receive function
* \param[in]	void
* \return		void
*/

static uint32_t countReq =0;

static uint64_t Application_len;
static uint32_t FW_buff_index;
static uint32_t FW_data_len;


void BT_process_requests(void)
{
	static bool bt_session_complete    = FALSE;
	char date_info[50];
	int len;

	memset(bt_rx_buff, 0x00, sizeof(bt_rx_buff));
	bt_total_received_bytes = API_BLE_Receive(bt_rx_buff);
	if (BUF_EMPTY == bt_total_received_bytes){
		bt_send_ack_or_nack_response(NACK_INVALID_PACKET);
		printf("No command received from APP\n");
		return; 
	}
    //Condition for not to respond to any Junk data from bluetooth
	if((bt_rx_buff[0] != SOS) && ((bt_rx_buff[bt_total_received_bytes-3]) != EOS)){
		bt_send_ack_or_nack_response(NACK_INVALID_PACKET);
        printf("Not a valid command request string from BT/APP\n");
        return;
    }
    if((bt_rx_buff[1] == REC_SNAPSHOT_REQ) || (bt_rx_buff[1] == TIME_SYNC_REQ)){
		BT_ongoing_session = FALSE;
		bt_response_state = INIT_STATE;

	}

    if(Is_Test_In_Progress)
    {
    	bt_send_ack_or_nack_response(NACK_INVALID_PACKET);
    	printf("\nTest is in progress, So can not sync the data");
    	return;
    }
	//New request from the BT client for record sync  	
	if(BT_ongoing_session == FALSE){
		client_request_cmd = bt_rx_buff[1];
		BT_ongoing_session = TRUE;
		bt_response_state = INIT_STATE;
		skip_count = 0;
		count = 0;
		countReq=0;
	}
	//TODO: handle if we don't get ACK for on going sync process but there is new request for record sync 

	if(Data_sync_in_progress)
	{
		 API_TIMER_Register_Timer(DATA_SYNC_TIMEOUT);
	}


//	printf("client_request_cmd: %x\n", client_request_cmd);
	switch (client_request_cmd){
		case BP1_data_req:
			printf("\nBP1 Sync started");
			Data_sync_in_progress = TRUE;
			bt_session_complete = bt_send_multi_response(BP1, BP1_ONE_REC_LEN);
			printf("bt_session_complete is %d\n",bt_session_complete);
			printf("\ncount=%ld",countReq++);
			if (bt_session_complete){
		    	BT_ongoing_session = false;
				client_request_cmd = NO_CMD_REQ;
				printf("\nBP1 Sync END");
			}
			break;

		case BP2_data_req:
			printf("\nBP2 Sync started");
			Data_sync_in_progress = TRUE;
			bt_session_complete = bt_send_multi_response(BP2, BP2_ONE_REC_LEN);
			printf("bt_session_complete is %d\n",bt_session_complete);
			printf("\ncount=%ld",countReq++);
			if (bt_session_complete){
				BT_ongoing_session = false;
				client_request_cmd = NO_CMD_REQ;
				printf("\nBP2 Sync END");
			}
			break;

		case BG_data_req:
			Data_sync_in_progress = TRUE;
			bt_session_complete = bt_send_single_response(BG, BG_ONE_REC_LEN);
			if (bt_session_complete){
				BT_ongoing_session = false;
				client_request_cmd = NO_CMD_REQ; 
			}
			break;

		case SENSETEMP_data_req:
			Data_sync_in_progress  = TRUE;
			bt_session_complete = bt_send_single_response(SENSE_TEMP, TEMP_ONE_REC_LEN);
			if (bt_session_complete){
				BT_ongoing_session = false;
				client_request_cmd = NO_CMD_REQ; 
			}
			break;

		case ECG_1_Lead_data_req:
				    //send ecg response over bluetooth

			printf("\nECG L1 test starter");
					Data_sync_in_progress = TRUE;
					bt_session_complete = bt_send_multi_response(ECG_1_Lead, ECG_1_LEAD_REC_LEN);
					if (bt_session_complete){
						BT_ongoing_session = false;
						client_request_cmd = NO_CMD_REQ;
						printf("ECG Record sync completed\n");
					}
					break;

		case ECG_6_Lead_data_req:
		    //send ecg response over bluetooth
			Data_sync_in_progress = TRUE;
			bt_session_complete = bt_send_multi_response(ECG_6_Lead, ECG_3_LEAD_REC_LEN);
			if (bt_session_complete){
				BT_ongoing_session = false;
				client_request_cmd = NO_CMD_REQ; 
			}
			break;

		case ECG_12_Lead_data_req:
			Data_sync_in_progress = TRUE;
			bt_session_complete = bt_send_multi_response(ECG_12_LEAD, ECG_12_LEAD_ONE_REC_LEN);
			if (bt_session_complete){
				BT_ongoing_session = 0x00;
				client_request_cmd = NO_CMD_REQ; 
			}
			break;

		case SPO2_data_req:
			Data_sync_in_progress = TRUE;
			printf("\nSPO2 Sync started");

			bt_session_complete = bt_send_multi_response(SPO2, SPO2_ONE_REC_LEN);
			printf("\ncount=%ld",countReq++);

			if (bt_session_complete){
				BT_ongoing_session = false;
				client_request_cmd = NO_CMD_REQ; 
				printf("\nSPO2 Sync End");
			}

			break;
		case TIME_SYNC_REQ:  //date format ddmmyyhhmmss in decimal, if passed value in hex then need to convert to decimal

			API_RTC_Update_Date_Time(bt_rx_buff[4], bt_rx_buff[5], bt_rx_buff[6] + 2000, bt_rx_buff[7], bt_rx_buff[8], bt_rx_buff[9]);

			memset(date_info,'\0',sizeof(date_info));
			API_RTC_Get_Date_Time(date_info,"%d/%m/%y %H:%M:%S");
			printf("\nDate/Time Received: %s\n",date_info);
			
			bt_send_ack_or_nack_response(ACK);
			BT_ongoing_session = FALSE;
			is_arrived_timesynq_req = TRUE;
			break;

		//which gives information about  the number of records in the flash(for all vital parameters)
		case REC_SNAPSHOT_REQ:
			send_record_snapsot_response();
			bt_session_complete = TRUE;
			if (bt_session_complete){
				BT_ongoing_session = 0;
				client_request_cmd = NO_CMD_REQ; 
			}
			break;

		case OFFLINE_DATA_REQ:
			Data_sync_in_progress = TRUE;
//			bt_session_complete = bt_send_offline_rec();
			if (bt_session_complete){
				BT_ongoing_session = 0x00;
			}
			break;

		case REC_SNAPSHOT_REQ_OFFLINE_CONF:
//			send_record_snapshot_responce_offline_config();
			bt_session_complete = TRUE;
			if (bt_session_complete){
				BT_ongoing_session = 0x00;
			}
			break;
			
    	//this is to register the patient id
		case Patient_Reg_req:

			memcpy(PatientID.pid,bt_rx_buff+5,sizeof(PatientID.pid));
			printf("\n");
			printf("\nReceived pid  request.\n");

			for(int i=0;i<18;i++)
				{
					 printf("%c",(char)PatientID.pid[i]);
				}
			bt_send_ack_or_nack_response(ACK);

			BT_ongoing_session = FALSE;
			Selected_PID_type = VALID_PID;

			break;

		case Device_Configuration:
			store_device_configuration();//storing the device configuration into the flash field
			bt_send_ack_or_nack_response(ACK);//send the ack after storing the device
			BT_ongoing_session = 0x00;
			break;

		case FW_VER_REQ :
			bt_firmware_request_response();
			BT_ongoing_session = FALSE;
			break;

		case BT_MOBILE_NUM_SYNC_REQ:
			MemCpy(mobile_number, (bt_rx_buff + 3) ,sizeof(mobile_number));
			bt_send_ack_or_nack_response(ACK);
			BT_ongoing_session = FALSE;
		    break;

		case BT_FULL_DIAGNOSTICS_REQ:
			BT_ongoing_session  = FALSE;
			break;

		case BT_SELF_DIAGNOSTICS_REQ:
//			bt_send_device_self_diagnosis_response();
			BT_ongoing_session = FALSE;
            break;

		case OTA_FW_UPGRADE_REQ :
			len  = bt_rx_buff[2] << 8; //computing length field 
			len |= bt_rx_buff[3];
			if (bt_total_received_bytes - 4 != len){ //comparing with received with payload len
				printf("Incorrect length, received: %d, packet len: %d\n", bt_total_received_bytes - 4, len);
				bt_send_ack_or_nack_response(NACK_INVALID_PACKET);
				break;
			}
			//TODO: Check CRC for the received packet

			bt_send_ack_or_nack_response(NACK_DEVICE_BUSY); //Processing data
			if(is_OTA_request_arrived == FALSE){
				is_OTA_request_arrived = TRUE;
				BT_ongoing_session     = TRUE;
				Data_sync_in_progress  = TRUE;
				FW_buff_index 	       = 0;
				Application_len        = 0;
				FW_complete_data_received = false;	
//				Upgradation_progress = DISP_DEVICE_UPGRADING;
			}
			int ret = BTL_validate_and_copy2buf(bt_rx_buff + 4);	
			if (ret == FW_PACKET_CURRUPTED) {
				bt_send_ack_or_nack_response(NACK_INVALID_PACKET);
			}
			else if (ret == END_OF_FILE_RECORD){
				FW_complete_data_received = true;	
				is_OTA_request_arrived = FALSE;
				//BT_ongoing_session = FALSE;
			} //ACK response will be sent after reading firmware data in case of recevied packet is good
			break;

		default:
    		//If invalid request, default response is NACK_REQ_DATA_NOT_SUPPORTED
			bt_send_ack_or_nack_response(NACK_REQ_DATA_NOT_SUPPORTED);
			BT_ongoing_session = 0x00;
			break;
	}
    //Clear the BT receive buffer
	MemSet(bt_rx_buff, 0x00, sizeof(bt_rx_buff));
}

/*bool bt_send_single_response(VITAL_TYPE_t vital, uint16_t one_record_len)
 * \brief       Responce for record size fits in single BT MTU packet
 * \param[in]	void
 * \return		bool true if the response session completes
 */
bool bt_send_single_response(VITAL_TYPE_t vital, uint16_t one_record_len)
{
	uint16_t crc_value = 0;
	uint8_t status = FALSE;
	uint16_t total_length = (one_record_len + EOS_SIZE + CHKSUM_SIZE);

	bool bt_response_session_complete = FALSE;
	if(bt_rx_buff[1] == 0xFF){
		if(retry_count < MAX_RETRY_COUNT){
			if(bt_response_state == WAIT_ACK_EOR_STATE){
				Load_record_header_to_buffer();
			}
			else{
				API_BLE_Transmit(bt_tx_buff, total_length_to_send);
			}
			retry_count++;
			return (bt_response_session_complete);
		}
		else{
			bt_send_ack_or_nack_response(NACK_DEVICE_BUSY);
			BT_ongoing_session = FALSE;
			bt_response_state = INIT_STATE;
			retry_count = 0;
		}
	}
	else{
		total_length_to_send = (one_record_len + BT_PACKET_FIELD_LENGTH);
		switch (bt_response_state) {
			case INIT_STATE:
				status = API_Flash_Read_Record(vital, BT_flash_buffer);
				//If no data in the flash than we send ack as data not available
				if(status == NO_RECORDS_IN_FLASH) {
					bt_send_ack_or_nack_response(NACK_DATA_NOT_AVAILABLE);
					bt_response_session_complete = TRUE;
					break;
				}
				bt_tx_buff[0] = SOS;
				bt_tx_buff[1] = EOR;
				MemCpy(bt_tx_buff + SOS_SIZE + CMD_SIZE, &total_length, LENGTH_SIZE);
				MemCpy((bt_tx_buff + SOS_SIZE + CMD_SIZE + LENGTH_SIZE), BT_flash_buffer, one_record_len);
				bt_tx_buff[(SOS_SIZE + CMD_SIZE + LENGTH_SIZE + one_record_len)] = EOS;
				crc_value = compute_crc_16(bt_tx_buff, total_length_to_send - CHKSUM_SIZE);
				MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE + one_record_len + EOS_SIZE ), &crc_value,CHKSUM_SIZE);
				API_BLE_Transmit(bt_tx_buff, total_length_to_send);
				bt_response_state = WAIT_ACK_EOR_STATE;
				break;

			case WAIT_ACK_EOR_STATE:
				erase_one_record(vital);
				bt_response_state = INIT_STATE;
				bt_response_session_complete = TRUE;
				Data_sync_in_progress = FALSE;
				break;
			
			default:
				break;
		}
	}
	return (bt_response_session_complete);
}

/*bool bt_send_multi_response(VITAL_TYPE_t vital, uint16_t one_record_len)
 * \brief       Responce for record size requires multiple BT packets 
 * \param[in]	void
 * \return		bool true response session completes
 */

uint16_t BP_Length_tx = 0;
TOTAL_RECORDS_STRUCT_t Total_Read_CurrentRecords;

bool bt_send_multi_response(VITAL_TYPE_t vital, uint16_t one_record_len)
{
	uint16_t crc_value = 0;
	uint8_t status = FALSE;

	bool bt_response_session_complete = FALSE;

	// in this block we serve the retry request
	// if the retry_count crosses more then 3 then killing ongoing session
	if(bt_rx_buff[1] == 0xFF) {
		if(retry_count < MAX_RETRY_COUNT) {
			if(bt_response_state == WAIT_ACK_SOR_STATE) {
				Load_record_header_to_buffer();
			}
			else {
				printf("1. %s --> Length: %ld\n", __func__, total_length_to_send);
				API_BLE_Transmit(bt_tx_buff, total_length_to_send);
			}
			retry_count++;
			return(bt_response_session_complete);
		}
		else{
			bt_send_ack_or_nack_response(NACK_DEVICE_BUSY);
			BT_ongoing_session = FALSE;
			bt_response_state = INIT_STATE;
			retry_count = 0;
		}
	}
	else {
		retry_count = 0;
		// this block is to serve the BP_RECORD REQUEST
		switch (bt_response_state){
			case INIT_STATE:

				BP_Length_tx = 0;

				total_length_to_send = (REC_HEADER_LEN + BT_PACKET_FIELD_LENGTH);
				status = API_Flash_Read_Record(vital, BT_flash_buffer);
				/*this block will send DATA_NOT_AVAILABLE nack if no record found */
				if(status == NO_RECORDS_IN_FLASH){
					bt_send_ack_or_nack_response(NACK_DATA_NOT_AVAILABLE);
					bt_response_session_complete = TRUE;
					break;
				}
				Load_record_header_to_buffer();
				bt_response_state = WAIT_ACK_SOR_STATE;
				break;

				//to send the continuation of the record
			case WAIT_ACK_SOR_STATE:
				total_length_to_send = BT_PACKET_SIZE;
				printf("total_length_to_send: %ld\n", total_length_to_send);
				//here we are dividing the whole record into 580 bytes of packet and sending packet by packet (MTU: 600)
				count = (one_record_len - REC_HEADER_LEN)/BT_RAW_DATA_LENGTH;
				remainder = (one_record_len - REC_HEADER_LEN)%BT_RAW_DATA_LENGTH;
				printf("count: %d, remainder: %ld\n", count, remainder);
				if(count){
					Load_Raw_Data_To_Buffer(COR);	//load the raw data to the buffer
					skip_count ++;					//increment the skip_counter to point to next 240 bytes
					count --;						//after sending every packet we decrement the count
					printf("2. %s --> Length: %ld\n", __func__, total_length_to_send);
					API_BLE_Transmit(bt_tx_buff, total_length_to_send);
					bt_response_state = WAIT_ACK_COR_STATE;
				}
				break;

			case WAIT_ACK_COR_STATE:
				MemSet(bt_tx_buff,0x00,sizeof(bt_tx_buff));
				/*in this block we are sending remaining packets*/
				printf("Packet count: %d\n", count);
				if(count){
					total_length_to_send = BT_PACKET_SIZE;
					printf("total_length_to_send: %ld\n", total_length_to_send);
					if(count == 1 && remainder == 0){
						Load_Raw_Data_To_Buffer(EOR);								//   IF No remainder then overwrite the COR by EOR
						bt_response_state = WAIT_ACK_EOR_STATE;
						printf("3. %s --> Length: %ld\n", __func__, total_length_to_send);
						API_BLE_Transmit(bt_tx_buff, total_length_to_send);
					}
					else{
						Load_Raw_Data_To_Buffer(COR);
						API_BLE_Transmit(bt_tx_buff, total_length_to_send);
						bt_response_state = WAIT_ACK_COR_STATE;
					}
					++skip_count;
					--count;
					break;
				}
				/*in this block at last we send the remainder bytes which completes the whole record*/
				else if(remainder){
					uint8_t	bt_remainder_packet = (remainder + CHKSUM_SIZE + EOS_SIZE);
					total_length_to_send = (remainder + BT_PACKET_FIELD_LENGTH);
					MemSet(bt_tx_buff, 0x0, sizeof(bt_tx_buff));
					bt_tx_buff[0] = SOS;
					bt_tx_buff[1] = EOR;
					MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE), &bt_remainder_packet, LENGTH_SIZE);
					MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE), ((uint8_t*)BT_flash_buffer + REC_HEADER_LEN + (BT_RAW_DATA_LENGTH*skip_count)), remainder);
					bt_tx_buff[remainder + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE)] = EOS;//fill eos after raw data(240)and 4 bytes sos,cor,length 2 bytes
					crc_value = compute_crc_16(bt_tx_buff,(total_length_to_send-CHKSUM_SIZE));
					MemCpy(bt_tx_buff + remainder + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE + EOS_SIZE),&crc_value,CHKSUM_SIZE);
					printf("4. %s --> Length: %ld\n", __func__, total_length_to_send);
					API_BLE_Transmit(bt_tx_buff, total_length_to_send);

					bt_response_state = WAIT_ACK_EOR_STATE;
					break;
				}
				else
					bt_response_state = WAIT_ACK_EOR_STATE;
				break;

			case WAIT_ACK_EOR_STATE:
				erase_one_record(vital);
				count =0;
				skip_count = 0;
				//send final set of data and set state as INIT_BP_REQ
				bt_response_state = INIT_STATE;
				bt_response_session_complete = TRUE;
				Data_sync_in_progress = FALSE;

				memset(&Total_Read_CurrentRecords,0,sizeof(Total_Read_CurrentRecords));

				API_Get_Total_Record_Count(&Total_Read_CurrentRecords);

				printf("ecg_1_records : %d\n", Total_Read_CurrentRecords.ecg_1_records);

				break;
		}
	}

	if(bt_response_state != WAIT_ACK_EOR_STATE){
	BP_Length_tx += total_length_to_send-BT_PACKET_FIELD_LENGTH;
	//printf("BP_Length_tx = %d\n",BP_Length_tx);

	}

	return(bt_response_session_complete);
}

/*	static void bt_ack_or_nack_response(uint8_t command)
 *	\brief Function is used to send ack or nack depending upon the request
 *	\param[in]	command either ack or nack
 *	\return		void
 */
static void bt_send_ack_or_nack_response(uint8_t command)
{
	ACK_AND_NACK_RESPONSE response;
	response.sos = SOS;
	response.cmd = command;
	response.length = EOS_SIZE + CHKSUM_SIZE;
	response.eos = EOS;
	response.chksum = compute_crc_16((uint8_t *)&response,(sizeof(response)-CHKSUM_SIZE));
//	printf("%s: -- ACK/NACK: %x\n", __func__, command); 
	API_BLE_Transmit((uint8_t *)&response, sizeof(response));
}

/*	void Load_record_header_to_buffer(void)
 *	\brief      loading the record header
 *	\param[in]	record header structure
 *	\return		void
 */
void Load_record_header_to_buffer(void)
{
	SYNC_HEADER_STRUCT temp;
	temp.sos = SOS;
	temp.cmd = SOR;
	//temp.length = TOTAL_HEADER_LENGTH ;	// Ask
	temp.length = REC_HEADER_LEN + EOS_SIZE + CHKSUM_SIZE;
	MemCpy(temp.data, BT_flash_buffer, REC_HEADER_LEN);
	temp.eos = EOS;
	temp.chksum = compute_crc_16((uint8_t *)&temp,(sizeof(temp) - CHKSUM_SIZE));
	printf("%s --> Length: %d\n", __func__, sizeof(temp)); 
	API_BLE_Transmit((uint8_t *)&temp, sizeof(temp));
}

/*	void Load_Raw_Data_To_Buffer()
 *	\brief      loading the record header
 *	\param[in]	void
 *	\return		void
 */
void Load_Raw_Data_To_Buffer(uint8_t command)
{
	uint16_t length = BT_RAW_DATA_LENGTH + EOS_SIZE + CHKSUM_SIZE;
	uint16_t crc_value = 0;

	bt_tx_buff[0] = SOS;
	bt_tx_buff[1] = command;
	MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE), &length, LENGTH_SIZE); //2 is to skip those many bytes (SOS, COR each one byte)
	MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE),(((uint8_t*)(BT_flash_buffer)) + REC_HEADER_LEN + (BT_RAW_DATA_LENGTH * skip_count)), BT_RAW_DATA_LENGTH);
	bt_tx_buff[BT_RAW_DATA_LENGTH + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE)] = EOS; //fill EOS after raw data(240)and 4 bytes SOS, COR,length 2 bytes
	crc_value = compute_crc_16(bt_tx_buff,(BT_PACKET_SIZE - CHKSUM_SIZE));
	MemCpy(bt_tx_buff + (BT_RAW_DATA_LENGTH + SOS_SIZE + CMD_SIZE + LENGTH_SIZE + EOS_SIZE), &crc_value, CHKSUM_SIZE); //fill the checksum after filling all the filds
}

/*	static void send_record_snapsot_response(void)
 *	\brief      which will gives the record count in the flash
 *	\param[in]	void
 *	\return		void
 */
static void send_record_snapsot_response(void)
{
	REC_SNAP_RES record_snapshot_res;
	uint16_t crc_value = 0;

	record_snapshot_res.sos = SOS;
	record_snapshot_res.cmd = REC_SNAPSHOT_REQ;
	record_snapshot_res.length = (VITAL_REC_LEN * 7) + EOS_SIZE + CHKSUM_SIZE;//6+1 IS THE VITAL PARAMETERS (Bp,bg,ecg,spo2,sensetemp,12 lead ecg)

	//we get number of records from this file
	API_Get_Total_Record_Count(&total_records);
	record_snapshot_res.bp1_rec = total_records.bp1_records;
	record_snapshot_res.bp2_rec = total_records.bp2_records;
	record_snapshot_res.bg_rec = total_records.bg_records;
	record_snapshot_res.ecg1_rec = total_records.ecg_1_records;
	record_snapshot_res.ecg6_rec = total_records.ecg_3_records;// 6 lead data was derived from 3 lead data
	record_snapshot_res.spo2_rec = total_records.spo2_records;
	record_snapshot_res.sensetemp_rec = total_records.temp_records;
	record_snapshot_res.ecg_12lead_rec = total_records.ecg_12_lead_ecord;
	record_snapshot_res.eos = EOS;

	printf("\n\nRecord Snap shot request::\n"
			"record_snapshot_res.bp1_rec:%d\n"
			"record_snapshot_res.bp2_rec:%d\n"
			"record_snapshot_res.bg_rec:%d\n"
			"record_snapshot_res.ecg1_rec:%d\n"
			"record_snapshot_res.ecg6_rec:%d\n"
			"record_snapshot_res.spo2_rec:%d\n"
			"record_snapshot_res.sensetemp_rec:%d\n"
			"record_snapshot_res.ecg_12lead_rec:%d\n\n",
			record_snapshot_res.bp1_rec,record_snapshot_res.bp1_rec,
			record_snapshot_res.bg_rec,record_snapshot_res.ecg1_rec,
			record_snapshot_res.ecg6_rec,record_snapshot_res.spo2_rec,
			record_snapshot_res.sensetemp_rec,record_snapshot_res.ecg_12lead_rec);

	crc_value = compute_crc_16((uint8_t *)&record_snapshot_res,(sizeof(record_snapshot_res)-CHKSUM_SIZE));
	record_snapshot_res.chksum = crc_value;
	API_BLE_Transmit((uint8_t *)&record_snapshot_res, sizeof(record_snapshot_res));
}


/*	void store_device_configuration()
 *	\brief      used to store the configuration of the device
 */
void store_device_configuration()
{
	DEVICE_CONFIG config_status;

	MemCpy(&config_status, &bt_rx_buff[4], 4);

	if(((config_status.registration_config) & 0x01) == 1){
		flash_data.device_config.registration_config = 1;
	}
	else if(((config_status.registration_config) & 0x01) == 0){
		flash_data.device_config.registration_config = 0;
	}

	/*--------------off-line configuration support status-----------------------*/
//	if(((config_status.offline_support_status) & 0x01)== TRUE){
//		flash_data.device_config.offline_support_status = 1;
//	}
//	else {
//		flash_data.device_config.offline_support_status = 0;
//	}

	Flash_Pointers_Update();
}

/*	void bt_firmware_request_response()
 *	\brief Function is used to send present FW version in device
 *	\param[in]	void
 *	\return		void
 */
void bt_firmware_request_response(void)
{
	FW_VER_RESPONSE frame;
	frame.sos = SOS;
	frame.cmd = EOR;
	frame.length = FW_VER_SIZE + EOS_SIZE + CHKSUM_SIZE;
	MemCpy(&frame.data, FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION));
	frame.eos = EOS;
	frame.chksum = compute_crc_16((uint8_t*)&frame, sizeof(frame)-CHKSUM_SIZE);
	API_BLE_Transmit((uint8_t*)&frame, sizeof(frame));
}


/**********************************************************************************	
*	.Hex file record is decoded as follows:	
*	:10246200464C5549442050524F46494C4500464C33
*	|||||||||||                              CC->Checksum
*	|||||||||DD->Data
*	|||||||TT->Record Type
*	|||AAAA->Address
*	|LL->Record Length
*	:->Colon
*
*
* TT --> 00 - data record
* TT --> 01 - end-of-file record
*
***********************************************************************************/
int BTL_validate_and_copy2buf(uint8_t * fw_buff)
{
	uint32_t index1 	 		   = 0;
	uint32_t index2 	 		   = 0;
	uint8_t  record_len   		   = 0;
	uint16_t calculated_check_sum  = 0;
	bool     validation_completed  = FALSE;
	FIRMWARE_DATA_STATE_t fw_status = FW_PACKET_GOOD; 


	FW_data_len = 0;
	FW_buff_index += bt_total_received_bytes - 7;
	printf ("bt_total_received_bytes: %d\n", bt_total_received_bytes);
	while (validation_completed != TRUE) { 
		if(fw_buff[index1] == 0){ // indicates that end the record arrived
			FW_complete_data_received = true;
			validation_completed = TRUE;
			break;
		}
		if(fw_buff[index1 + OFFSET_TO_RECORD_TYPE] == FIRMWARE_RECORD_TYPE1){ //End-of-File record  
			fw_status = END_OF_FILE_RECORD;
			FW_complete_data_received = true;
			validation_completed = TRUE;
			break;
    	} 

		if(fw_buff[index1 + OFFSET_TO_RECORD_TYPE] == FIRMWARE_RECORD_TYPE0) {
			record_len  = fw_buff[index1] + 5; //5 is the header and crc length
			//Calculating checksum
			for(int byte_count = 0; byte_count < record_len-1; byte_count++) {
				calculated_check_sum += fw_buff[index1 + byte_count];
			}
			calculated_check_sum  = ~calculated_check_sum ;
			calculated_check_sum += 1;
			calculated_check_sum = (uint8_t)calculated_check_sum;
			//Verify Calculated checksum with the checksum appended in the record 
			if(calculated_check_sum != fw_buff[index1 + record_len-1]) {
				fw_status = FW_PACKET_CURRUPTED;
				validation_completed = TRUE;
			}
			calculated_check_sum = 0x00;
			memcpy((FW_buffer + index2), (fw_buff + index1+4), (record_len-5));
//			esp_log_buffer_hex("BLE packet", FW_buffer+index2, 16);
			index2 += record_len-5; // valid data length of the particular record
			index1 += record_len;
	 	}
		// Indicates arrived the end of the partial firmware
//		printf("Hex record: %d, FW data: %d\n", index1, index2);
		if(index1 >= (bt_total_received_bytes - 7)) {
			validation_completed = TRUE;
		}
	}
	FW_data_len = index2;
	Application_len += index2; //Total size of the application binary
	printf("Application len: %lld\n", Application_len);
//	esp_log_buffer_hex("FW ", FW_buffer, index2);
//	FW_complete_data_received = true;
	return fw_status;
}

int read_firmware_data (uint8_t * ota_write_data, int BUFFSIZE)
{
	int data_len;

	if (FW_data_len > BUFFSIZE){
		printf("Firmware data is more than the buffer size\n");
		return -1;
	}
	
	data_len = FW_data_len;
	if (data_len > 0){	
		memcpy(ota_write_data, FW_buffer, data_len);
		memset(FW_buffer, 0x00, data_len);
		FW_data_len = 0;
		//Send ACK response, ready to receive next data packet
		bt_send_ack_or_nack_response(ACK);
	}
//	FW_complete_data_received = false;
	return data_len;
}

bool is_firmware_data_available (void)
{
    if (FW_data_len){
        printf("Firwmare data avialable..\n");                         
        return true;
    }
    else {
        return false;
    }
}   
// -----------------------------------------------------------------------------
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
#if 0

bool bt_send_offline_rec()
{
	static SYNC_HEADER_STRUCT_OFFLINE_CONFIG offline_record_summary;
	uint16_t crc_value = 0;
	uint8_t status = FALSE;

	bool bt_six_lead_ecg_response_session_complete = FALSE;

	if(bt_rx_buff[1] == 0xFF)
	{
		if(retry_count < MAX_RETRY_COUNT)
		{
			if(bt_offline_req_response_state == OFFLINE_REC_REQ_INIT_STATE)
			{
				bt_load_transmite_record_header_offline(&offline_record_summary);
				//API_BLE_Transmit((uint8_t*)&offline_record_summary, total_length_to_send);
			}
			else
			{
				API_BLE_Transmit(bt_tx_buff,total_length_to_send);
			}
			retry_count++;
			return (bt_six_lead_ecg_response_session_complete);
		}
		else
		{
			t_send_ack_or_nack_response(NACK_DEVICE_BUSY);
			BT_ongoing_session = FALSE;
			bt_offline_req_response_state = OFFLINE_REC_REQ_INIT_STATE;
			retry_count = 0;
		}

	}
	else
	{
		retry_count = 0;
		total_length_to_send = 0;//REC_HEADER_LEN+BT_PACKET_FIELD_LENGTH;
		switch (bt_offline_req_response_state)
		{

			case OFFLINE_REC_REQ_INIT_STATE:
				status = read_test_record_flash(BT_flash_buffer ,ECG_OFFLINE);
				if(status == ZERO_OFFLINE_REC_IN_FLASH)
				{
					bt_send_ack_or_nack_response(NACK_DEVICE_BUSY);
					bt_six_lead_ecg_response_session_complete = TRUE;
					break;
				}
				total_length_to_send = OFFLINE_REC_SUMMARY_LEN+BT_PACKET_FIELD_LENGTH;

				bt_load_transmite_record_header_offline(&offline_record_summary);

				bt_offline_req_response_state = OFFLINE_REC__WAIT_ACK_SOR_STATE;
				break;

			case OFFLINE_REC__WAIT_ACK_SOR_STATE :
				{
					count = (ONE_OFFLINE_RECORD_LENGTH - OFFLINE_REC_SUMMARY_LEN)/BT_RAW_DATA_LENGTH;
					remainder = (ONE_OFFLINE_RECORD_LENGTH - OFFLINE_REC_SUMMARY_LEN)%BT_RAW_DATA_LENGTH;
					MemSet(bt_tx_buff,0x00,sizeof(bt_tx_buff));
					total_length_to_send = BT_PACKET_SIZE;
					Load_Raw_Data_To_Buffer_Offline_Config(COR);
					++skip_count;
					--count;
					API_BLE_Transmit(bt_tx_buff, total_length_to_send);
					bt_offline_req_response_state = OFFLINE_REC__WAIT_ACK_COR_STATE;
					break;
				}
			case OFFLINE_REC__WAIT_ACK_COR_STATE :
				MemSet(bt_tx_buff,0x00,sizeof(bt_tx_buff));
				if(count)
				{
					total_length_to_send = BT_PACKET_SIZE;

					if(count == 1 && remainder == 0)
					{
						Load_Raw_Data_To_Buffer_Offline_Config(EOR);
						bt_offline_req_response_state = ECG_WAIT_ACK_EOR_STATE;
						API_BLE_Transmit(bt_tx_buff, total_length_to_send);
					}
					else
					{
						Load_Raw_Data_To_Buffer_Offline_Config(COR);
						API_BLE_Transmit(bt_tx_buff, total_length_to_send);
						bt_offline_req_response_state = ECG_WAIT_ACK_COR_STATE;
					}


					++skip_count;
					--count;
					break;
				}
				else if(remainder)
				{
					//set_chunk_size(remainder);
					//read_test_record_flash(bt_rx_buff ,SPO2);
					uint8_t	bt_remainder_packet = remainder + EOS_SIZE + CHKSUM_SIZE;
					total_length_to_send = remainder + BT_PACKET_FIELD_LENGTH;
					bt_tx_buff[0] = SOS;
					bt_tx_buff[1] = EOR;
					MemCpy(bt_tx_buff + SOS_SIZE + CMD_SIZE,&bt_remainder_packet,LENGTH_SIZE);
					MemCpy(bt_tx_buff + SOS_SIZE + CMD_SIZE + LENGTH_SIZE,((uint8_t*)BT_flash_buffer+ OFFLINE_REC_SUMMARY_LEN+(BT_RAW_DATA_LENGTH*skip_count)),remainder);
					bt_tx_buff[remainder + SOS_SIZE + CMD_SIZE + LENGTH_SIZE] = EOS;//fill eos after raw data(240)and 4 bytes sos,cor,length 2 bytes
					crc_value = compute_crc_16(bt_tx_buff,remainder + SOS_SIZE + CMD_SIZE + LENGTH_SIZE + EOS_SIZE);//calculate the crc
					MemCpy(bt_tx_buff + remainder + SOS_SIZE + CMD_SIZE + LENGTH_SIZE + EOS_SIZE,&crc_value,CHKSUM_SIZE);
					API_BLE_Transmit(bt_tx_buff, total_length_to_send);
					bt_offline_req_response_state = OFFLINE_REC__WAIT_ACK_EOR_STATE;
					break;
				}
				else
				{
					bt_offline_req_response_state = OFFLINE_REC__WAIT_ACK_EOR_STATE;//if there is no reminder change responce state
				}
				break;

			case OFFLINE_REC__WAIT_ACK_EOR_STATE:
				erase_one_record(ECG_OFFLINE);			//after receiving last ack erase one record from flash
				flash_offline_test_count_reset();
				count = 0;
				skip_count = 0;
				bt_offline_req_response_state = OFFLINE_REC_REQ_INIT_STATE;
				bt_six_lead_ecg_response_session_complete = TRUE;
				Data_sync_in_progress = FALSE;
				break;
		}
	}
	return (bt_six_lead_ecg_response_session_complete);
}

static void send_record_snapshot_responce_offline_config(void)
{
	REC_SNAP_RES_OFFLINE_CONFIG rec_info;

	uint16_t checksum = 0;

	rec_info.sos 	= SOS;
	rec_info.cmd 	= REC_SNAPSHOT_REQ_OFFLINE_CONF ;
	rec_info.length = 0x04;
	rec_info.data = get_number_of_record_count_offline_config();
	rec_info.eos	= EOS;
	checksum = compute_crc_16((uint8_t *)&rec_info,(sizeof(rec_info)-CHKSUM_SIZE));
	rec_info.chksum	= checksum;

	API_BLE_Transmit((uint8_t *)&rec_info, sizeof(rec_info));

}

/*	void Load_Raw_Data_To_Buffer()
 *	\brief      loading the record header
 *	\param[in]	void
 *	\return		void
 */


//void Load_record_header_to_buffer_offline_config(SYNC_HEADER_STRUCT_OFFLINE_CONFIG * test )
void bt_load_transmite_record_header_offline(SYNC_HEADER_STRUCT_OFFLINE_CONFIG * test )
{
	SYNC_HEADER_STRUCT_OFFLINE_CONFIG temp;

	temp.sos = SOS;
	temp.cmd = SOR;
	temp.length = ( OFFLINE_REC_SUMMARY_LEN + EOS_SIZE+CHKSUM_SIZE );
	MemCpy(temp.data,BT_flash_buffer,OFFLINE_REC_SUMMARY_LEN);
	temp.eos 	= EOS;
	temp.chksum	= compute_crc_16((uint8_t *)&temp,(sizeof(temp)-CHKSUM_SIZE));

	API_BLE_Transmit((uint8_t*)&temp,sizeof(temp));
}



void Load_Raw_Data_To_Buffer_Offline_Config(uint8_t command)
{
	uint16_t length = BT_RAW_DATA_LENGTH + EOS_SIZE + CHKSUM_SIZE;
	uint16_t crc_value = 0;
	bt_tx_buff[0] = SOS;
	bt_tx_buff[1] = command;
	MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE),&length,LENGTH_SIZE);//2 is to skip those many bytes (sos,cor each one byte)
	MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE),(((uint8_t*)(BT_flash_buffer))+ OFFLINE_REC_SUMMARY_LEN+(BT_RAW_DATA_LENGTH*skip_count)),BT_RAW_DATA_LENGTH);
	bt_tx_buff[BT_RAW_DATA_LENGTH + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE)] = EOS;//fill eos after raw data(240)and 4 bytes sos,cor,length 2 bytes
	crc_value = compute_crc_16(bt_tx_buff,(BT_PACKET_SIZE - CHKSUM_SIZE));
	MemCpy(bt_tx_buff + (BT_RAW_DATA_LENGTH + SOS_SIZE + CMD_SIZE + LENGTH_SIZE +  EOS_SIZE),&crc_value,CHKSUM_SIZE);//fill the checksum after filling all the filds
}

/*	void bt_pid_full_nack()
 *	\brief      sending the nack response if pid storage full
 *	\param[in]	void
 *	\return		void
 *

void bt_pid_full_nack()
{

	uint16_t crc_value = 0;
	uint16_t length = (CHKSUM_SIZE + EOS_SIZE + DATA_LENGTH);//one byte is for data field
	bt_tx_buff[0] = SOS;
	bt_tx_buff[1] = COR;
	MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE), &length, LENGTH_SIZE);
	bt_tx_buff[4] = 0x80;//this is data field
	bt_tx_buff[5] = EOS;
	crc_value = compute_crc_16(bt_tx_buff,(SOS_SIZE + CMD_SIZE + LENGTH_SIZE + DATA_LENGTH + EOS_SIZE));
	MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE + DATA_LENGTH + EOS_SIZE),&crc_value,CHKSUM_SIZE);
	API_BLE_Transmit(bt_tx_buff,(SOS_SIZE + CMD_SIZE + LENGTH_SIZE + DATA_LENGTH + EOS_SIZE + CHKSUM_SIZE));//8 IS total number of bytes to transfer
}*/

/*	void bt_pid_registered_ack()
 *	\brief      sending the ack response for pid registration
 *	\param[in]	void
 *	\return		void
 *

void bt_pid_registered_ack()
{
	uint16_t crc_value = 0;
	uint16_t length = (CHKSUM_SIZE + EOS_SIZE + DATA_LENGTH);
	bt_tx_buff[0] = SOS;
	bt_tx_buff[1] = ACK;
	MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE),&length,LENGTH_SIZE);
	bt_tx_buff[4] = 0x01;//this is data field
	bt_tx_buff[5] = EOS;
	crc_value = compute_crc_16(bt_tx_buff,(SOS_SIZE + CMD_SIZE + LENGTH_SIZE + DATA_LENGTH + EOS_SIZE));
	MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE + DATA_LENGTH + EOS_SIZE),&crc_value,CHKSUM_SIZE);
	API_BLE_Transmit(bt_tx_buff,(SOS_SIZE + CMD_SIZE + LENGTH_SIZE + DATA_LENGTH + EOS_SIZE + CHKSUM_SIZE));
}*/

/*	void patient_id_regristration()
 *	\brief      used to register the pid which is received from app
 *	\param[in]	void
 *	\return		void
 */
static bool patient_id_regristration(void)
{
	uint16_t total_pids_on_flash = 0;
	bool device_configuration_status;
	bool pid_reg_status = TRUE;
	bool pid_calib_status = TRUE;

	device_configuration_status = get_device_configuration_status();//this function returns deviceconfig status

	if((bt_rx_buff[4] & 0x01) == 0)//this field indicates that register the pid
	{
		if(device_configuration_status == TRUE)//if configuration status is 1 means it is hospital registration
		{
			MemCpy(hospital_pid,(bt_rx_buff+15),10);

		}
		else if(device_configuration_status == FALSE) //if the configuration status is 0 then individual registration
		{
			total_pids_on_flash = Get_all_pids_count_in_flash();//Get_all_pids_flash
			if(total_pids_on_flash >= 9)//in individual registration we can store max 9 id's we can't store more than that. checking the
			{
				//bt_send_ack_or_nack_response(NACK_REQ_DATA_NOT_SUPPORTED);
				//bt_pid_full_nack();
				pid_reg_status = FALSE;
			}
			else
			{
				store_pids_flash(bt_rx_buff);
			}
		}
	}
	else if((bt_rx_buff[4] & 0x01) == 1)//FOR DEREGISTRATION
	{

		MemCpy(old_pid,(bt_rx_buff + 5),10);
		MemCpy(new_pid,(bt_rx_buff + 15),12);

		if(device_configuration_status == TRUE)//HOSPITAL
		{
			if((memcmp(old_pid,0x00,10)!= 0)&&(memcmp(old_pid,0x00,10)!= 0xFF))
			{
				MemSet(hospital_pid,0x00,10);
				MemCpy(hospital_pid,(bt_rx_buff + 15),10);
			}
		}
		else if(device_configuration_status == FALSE)
		{
			deregister_old_pid_add_new_pid(old_pid,new_pid);
		}
	}

	//BP
	if(((bt_rx_buff[4] & 0x02) == 2) || ((bt_rx_buff[4] & 0x04) == 4))
	{
		MemCpy(bp_calibration.new_pid,(bt_rx_buff + 15),10);

		pid_calib_status = bt_bp_extract_calib_factors();
	}
	else if(((bt_rx_buff[4] & 0x02) == 0) && ((bt_rx_buff[4] & 0x04) == 0))
	{
		bp_calibration.valid_bp_test = TRUE;
	}

	return (pid_reg_status|pid_calib_status);

}

/*	char* BT_Get_mobile_number(void)
 *	\brief
 *	\param[in]	void
 *	\return		void
 */

char* BT_Get_mobile_number(void)
{
	return mobile_number;
}

bool BT_Get_timesynq_req_flag()
{
	return is_arrived_timesynq_req;
}


static void bt_send_device_self_diagnosis_response()
{

	DEVICE_DIAGNOSIS_STRUCT_t self_diagnosis_data_frame;
	uint64_t self_diagnosis_status[1] = {0x00};

	self_diagnosis_data_frame.sos = SOS;
	self_diagnosis_data_frame.cmd = EOR;

	self_diagnosis_status[0] = Get_Device_self_diagnosis_test_status();
	self_diagnosis_data_frame.length = sizeof(self_diagnosis_status) + CHKSUM_SIZE + EOS_SIZE;

	MemCpy(&self_diagnosis_data_frame.data,self_diagnosis_status,sizeof(self_diagnosis_data_frame.data));
	self_diagnosis_data_frame.eos    = EOS;
	self_diagnosis_data_frame.chksum = 0xFF; // initial dummy value

	self_diagnosis_data_frame.chksum = compute_crc_16((void*)&self_diagnosis_data_frame,(sizeof(self_diagnosis_data_frame)-CHKSUM_SIZE));

	API_BLE_Transmit((uint8_t*)&self_diagnosis_data_frame,sizeof(self_diagnosis_data_frame));

}

static void bt_send_device_full_diagnosis_response()
{
	DEVICE_DIAGNOSIS_STRUCT_t full_diagnosis_data_frame;
	uint64_t full_diagnosis_status = 0x00;
	uint8_t bt_end_point = 0x00;

	full_diagnosis_data_frame.sos = SOS;
	full_diagnosis_data_frame.cmd = EOR;

	API_Wdog_Disable();
	Device_full_diagnosis_status_clear();
	device_full_diagnosis_test_enter();

	full_diagnosis_status = Get_full_diagnostic_status();

	if(full_diagnosis_status != 0x00)
	{
		full_diagnosis_data_frame.length = sizeof(full_diagnosis_status) + CHKSUM_SIZE + EOS_SIZE;

		MemCpy(&full_diagnosis_data_frame.data,&full_diagnosis_status,sizeof(full_diagnosis_status));
		full_diagnosis_data_frame.eos    = EOS;
		full_diagnosis_data_frame.chksum = 0xFF; // initial dummy value

		full_diagnosis_data_frame.chksum = compute_crc_16((void*)&full_diagnosis_data_frame,(sizeof(full_diagnosis_data_frame)-CHKSUM_SIZE));

		bt_end_point = API_BLE_Get_EndPoint();

		if(flash_data.sys_mode == ACTIVE_MODE)
		{
			API_BLE_Transmit_To_EndPoint((uint8_t *)&full_diagnosis_data_frame,sizeof(full_diagnosis_data_frame),bt_end_point);
		}
	}

	else
	{
		bt_send_ack_or_nack_response(NACK_DATA_NOT_AVAILABLE);
		//BT_send_nack();
	}

	API_Delay_1sec(2);
}


static bool bt_bp_multiplier_send_response(void)
{
	uint16_t crc_value = 0;
	uint16_t total_length = 0;
	uint8_t status = FALSE;
	bool bt_bp_multi_response_session_complete = FALSE;

	if(bt_rx_buff[1] == 0xFF)
	{
		if(retry_count < MAX_RETRY_COUNT)
		{
			if(bt_bp_multiplier_response_state == BP_MULTIPLIER_WAIT_ACK_EOR_STATE)
			{
				API_BLE_Transmit(bt_tx_buff,total_length_to_send);
			}
			retry_count++;
			return (bt_bp_multi_response_session_complete);
		}
		else
		{
			bt_send_ack_or_nack_response(NACK_DEVICE_BUSY);
			BT_ongoing_session = FALSE;
			bt_bp_multiplier_response_state = BP_MULTIPLIER_REQ_INIT_STATE;
			retry_count = 0;
		}
	}
	else
	{
		switch (bt_bp_multiplier_response_state)
		{
			case BP_MULTIPLIER_REQ_INIT_STATE:

				total_length_to_send = (BP_MULTIPLIERS_ONE_REC_LEN + BT_PACKET_FIELD_LENGTH);
				status = read_test_record_flash(BT_flash_buffer ,BP_MULTIPLIERS);
				if(status == ZERO_BP_MULTIPLIERS_REC_IN_FLASH)
				{
					bt_send_ack_or_nack_response(NACK_DATA_NOT_AVAILABLE);
					bt_bp_multi_response_session_complete = TRUE;
					break;
				}
				bt_tx_buff[0] = SOS;
				bt_tx_buff[1] = EOR;
				total_length = BP_MULTIPLIERS_ONE_REC_LEN + EOS_SIZE + CHKSUM_SIZE;
				MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE),&total_length,LENGTH_SIZE);
				MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE),BT_flash_buffer,BP_MULTIPLIERS_ONE_REC_LEN);
				bt_tx_buff[(SOS_SIZE + CMD_SIZE + LENGTH_SIZE + BP_MULTIPLIERS_ONE_REC_LEN)] = EOS;
				crc_value = compute_crc_16(bt_tx_buff,(total_length_to_send - CHKSUM_SIZE));
				MemCpy(bt_tx_buff + (SOS_SIZE + CMD_SIZE + LENGTH_SIZE + BP_MULTIPLIERS_ONE_REC_LEN + EOS_SIZE),&crc_value,CHKSUM_SIZE);
				API_BLE_Transmit(bt_tx_buff,total_length_to_send);
				bt_bp_multiplier_response_state = BP_MULTIPLIER_WAIT_ACK_EOR_STATE;
				break;

			case BP_MULTIPLIER_WAIT_ACK_EOR_STATE:

				erase_one_record(BP_MULTIPLIERS);
				bt_bp_multiplier_response_state = BP_MULTIPLIER_REQ_INIT_STATE;
				bt_bp_multi_response_session_complete = TRUE;
				Data_sync_in_progress = FALSE;
				total_length_to_send = 0;
				break;
		}
	}
	return (bt_bp_multi_response_session_complete);
}

static bool bt_bp_extract_calib_factors(void)
{
	uint8_t sbp_hex[4] = {0};
	uint8_t dbp_hex[4] = {0};
	bp_calibration.valid_bp_test = TRUE;

	if((bt_rx_buff[4] & 0x02) == 2)
	{
		bp_calibration.sub_command = BP_CALIBRATION_FACTORS;

		MemCpy(sbp_hex,(bt_rx_buff + 27),4);
		MemCpy(dbp_hex,(bt_rx_buff + 31),4);

		Hex_to_Float(sbp_hex, &bp_calibration.sbp_multiplier_val);
		Hex_to_Float(dbp_hex, &bp_calibration.dbp_multiplier_val);

		if((bp_calibration.sbp_multiplier_val > BP_MAX_SBP_MULTIPIER) || (bp_calibration.dbp_multiplier_val > BP_MAX_DBP_MULTIPIER)
				|| (bp_calibration.sbp_multiplier_val < BP_MIN_SBP_MULTIPIER) || (bp_calibration.dbp_multiplier_val < BP_MIN_DBP_MULTIPIER))
		{
			bp_calibration.valid_bp_test = FALSE;
			bp_calibration.sbp_multiplier_val = 0;
			bp_calibration.dbp_multiplier_val = 0;
		}
	}
	else if((bt_rx_buff[4] & 0x04) == 4)
	{
		bp_calibration.sub_command = BP_STD_VALUES;

		MemCpy(&bp_calibration.sbp_std_val,(bt_rx_buff + 27),4);
		MemCpy(&bp_calibration.dbp_std_val,(bt_rx_buff + 31),4);

		if((bp_calibration.sbp_std_val > BP_MAX_SBP_THRESHOLD) || (bp_calibration.dbp_std_val > BP_MAX_DBP_THRESHOLD)
				|| (bp_calibration.sbp_std_val < BP_MIN_SBP_THRESHOLD) || (bp_calibration.dbp_std_val < BP_MIN_DBP_THRESHOLD))
		{
			bp_calibration.valid_bp_test = FALSE;
			bp_calibration.sbp_std_val = 0;
			bp_calibration.dbp_std_val = 0;
		}
	}

	return (bp_calibration.valid_bp_test);
}


#endif


void BT_Sync_Timeout_Init_State(void)
{
	BT_ongoing_session = FALSE;
}
