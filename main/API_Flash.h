#include <stdbool.h>
#include <stdint.h>

// Return Message
typedef enum {
	FLASH_SUCCESS = 0,
	FLASH_INIT_ERROR,
	FLASH_INVALID_ARGS,
	FLASH_ADDRESS_INVALID,
	FLASH_READ_SUCCESS,
	FLASH_WRITE_SUCCESS,
	FLASH_READ_FAILED,
	FLASH_WRITE_FAILED,
	FLASH_ERASE_FAILED

}FLASH_STATUS;

void API_FLASH_init (void);

uint8_t API_FLASH_Read(uint32_t src_addr, void *dest_buff, uint32_t byte_length);
//uint8_t API_FLASH_Read_Page(uint32_t src_addr, void *dest_buff, uint32_t byte_length);

uint8_t API_FLASH_Write(uint32_t dest_addr, void *src_buff, uint32_t byte_length);
//uint8_t API_FLASH_Write_Page(uint32_t dest_addr, void *src_buff, uint16_t byte_length);

uint8_t API_FLASH_Chip_Erase(void);
uint8_t API_FLASH_Sector_Erase(uint32_t offset, uint32_t no_bytes);
uint8_t API_FLASH_Partition_Erase(void);

//uint8_t API_FLASH_Write_OTP(uint32_t dest_addr, void *src_buff, uint32_t byte_length);
//uint8_t API_FLASH_Read_OTP(uint32_t src_addr, void *dest_buff, uint32_t byte_length);

