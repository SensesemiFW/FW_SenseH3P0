#include <string.h>
#include <assert.h>
#include "esp_partition.h"
#include "esp_log.h"
#include "API_Flash.h"
#include "Error_Handling.h"

/*------------------------------------- GLOBAL VARIABLES------------------------------------------------*/
//Pointer to partition information structure, allocated at the application init by API_FLASH_init() 
const esp_partition_t * data_partition; 

/*-------------------------------------LOCAL FUNCTIONS---------------------------------------------------*/

static const char *TAG = "API_Flash";

/*
 * The flash sample partition table.  
 *
 *  nvs,        data, nvs,      0x9000,  0x6000,
 *  phy_init,   data, phy,      0xf000,  0x1000,
 *  factory,    app,  factory,  0x10000, 1M,
 *  storage,    data,        ,        ,  0x40000,
 */

/*
 * Flash init: find the flash partition based on the partition table.
 * This pointer is used while calling other esp_partition_* function
 */
void API_FLASH_init ()
{

    data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
    if(data_partition == NULL){

      Catch_RunTime_Error(FLASH_INIT_FAIL);
        //ESP_LOGE(TAG, "Failed to get a valied flash partition, check partition table");
    }
   /* assert(data_partition != NULL);
    ESP_LOGI(TAG, "Flash Partition Init Success");
    
    ESP_LOGI(TAG, "Flash storage partition address: %x", data_partition -> address);
    ESP_LOGI(TAG, "Flash storage partition sectors: %d", data_partition -> size / SPI_FLASH_SEC_SIZE);
    ESP_LOGI(TAG, "Flash storage partition size   : %d bytes", data_partition -> size);
*/
}  

/*
 * @Description: Read data from flash partition to internal RAM
 * src_addr – Address where the data should be written, relative to the beginning of the partition.
 * dest_buff – Pointer to the destination buffer.
 * byte_length – Size of data to be read, in bytes.
 */
uint8_t API_FLASH_Read(uint32_t src_addr, void *dest_buff, uint32_t byte_length)
{
    if ((src_addr > data_partition -> size) || (byte_length > data_partition -> size)){
    	ESP_LOGE(TAG, "Flash read: Invalid parameters");
    	return FLASH_INVALID_ARGS;
    }
    esp_err_t err = esp_partition_read(data_partition, src_addr, dest_buff, byte_length);
//    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Flash read failed: %s", esp_err_to_name(err));
        return FLASH_READ_FAILED;
    } 
    return FLASH_READ_SUCCESS;
}

/*
 * @Description: Write data to flash partition
 * dest_addr – Address where the data should be written, relative to the beginning of the partition.
 * src_buff – Pointer to the source buffer. Pointer must be non-NULL and buffer must be at least ‘byte_length’ bytes long.
 * byte_length – Size of data to be written, in bytes.
 * Warning -- Before writing data to flash, that particular sector will be erased.
 */
uint8_t API_FLASH_Write(uint32_t dest_addr, void *src_buff, uint32_t byte_length)
{
	esp_err_t err;

	if ((dest_addr > data_partition -> size) || (byte_length > data_partition -> size)){
	    	ESP_LOGE(TAG, "Flash write: Invalid parameters");
	    	return FLASH_INVALID_ARGS;
	}

	//Calculate sector for aligned to 4 kilobytes.
//	uint32_t sect = (dest_addr/SPI_FLASH_SEC_SIZE) * SPI_FLASH_SEC_SIZE;
//	printf("dest_addr: %d\n", dest_addr);
//	printf("Sector 4k boundary: %d\n", sect);

	//Calculate bytes length for aligned to 4 kilobytes.
//	uint32_t bytes = (byte_length/SPI_FLASH_SEC_SIZE + 1) * SPI_FLASH_SEC_SIZE;

    err = esp_partition_write(data_partition, dest_addr, src_buff, byte_length);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Flash Write failed: %s", esp_err_to_name(err));
        return FLASH_WRITE_FAILED;
    } 
    return FLASH_WRITE_SUCCESS;
}

/*
 * @Description: To erase full storage flash partition
 * flash – Pointer to partition structure obtained using API_FLASH_init, must be non-NULL.
 */
uint8_t API_FLASH_Chip_Erase(void)
{
    esp_err_t err = esp_partition_erase_range(data_partition, 0, data_partition -> size);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Flash Chip Erase failed: %s", esp_err_to_name(err));
        return FLASH_ERASE_FAILED;
    } 
    return FLASH_SUCCESS;
}

/*
 * @Description: To erase range of flash sectors (1 sector = 4096 bytes)
 * offset – Offset from the beginning of partition where erase operation should start. Must be sector aligned.
 * no_bytes – Size of the range which should be erased, in bytes. Must be divisible by 4096 (sector size).
 */
uint8_t API_FLASH_Sector_Erase(uint32_t offset, uint32_t no_bytes)
{
	if ((offset > data_partition -> size) ){
	    	ESP_LOGE(TAG, "Flash sector erase: Invalid parameters");
	    	return FLASH_INVALID_ARGS;
	}
/*	uint32_t sect = (offset/SPI_FLASH_SEC_SIZE) * SPI_FLASH_SEC_SIZE;
	printf("Sect: %d\n", sect);
	uint32_t bytes = (size/SPI_FLASH_SEC_SIZE + 1) * SPI_FLASH_SEC_SIZE;
	printf("bytes: %d\n", bytes);
*/
	esp_err_t err = esp_partition_erase_range(data_partition, offset, no_bytes);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Flash Sector Erase failed: %s", esp_err_to_name(err));
        return FLASH_ERASE_FAILED;
    } 
    return FLASH_SUCCESS;
}

