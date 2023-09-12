#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <esp_ota_ops.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "errno.h"
#include "esp_log.h"

#include "OTA_Upgrade.h"
#include "API_utility.h"
#include "API_Flash.h"
#include "API_Display.h"
#include "API_adc.h"
#include "API_IO_Exp.h"
#include <stdint.h>
#include <stdbool.h>
#include "API_timer.h"

#define BUFFSIZE 			1024
#define HASH_LEN 			32 /* SHA-256 digest length */
#define TIME_OUT_DELAY		60   //wait delay in sec

#define LOG_LOCAL_LEVEL ESP_LOG_ERROR

static const char *TAG = "ota";
/*an ota data write buffer ready to write to the flash*/
static uint8_t ota_write_data[BUFFSIZE + 1] = { 0 };

static FIRMWARE_UPGRADE_STATE_t Upgrade_state;

static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

static bool ota_firmware_upgrade(void)
{
	int timer;
	int data_read;
    esp_err_t err;

    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA firmware update");

	Upgrade_state = DEVICE_UPGRADE_START;
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08lx, but running from offset 0x%08lx",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08lx)",
             running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx",
             update_partition->subtype, update_partition->address);

    int binary_file_length = 0;
    /*deal with all received packets*/
    bool image_header_was_checked = false;

	timer = TIME_OUT_DELAY * 2;
    while (1)
    {
		int data_read = read_firmware_data (ota_write_data, BUFFSIZE);

        if (data_read > 0) {
            if (image_header_was_checked == false) {
            	image_header_was_checked = true;
				Upgrade_state = DEVICE_UPGRADING;
                err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                if (err != ESP_OK) {
	                ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                    //esp_ota_abort(update_handle);
    	        }
                ESP_LOGI(TAG, "esp_ota_begin succeeded");
            }
            esp_log_buffer_hex("OTA", ota_write_data, data_read);
            err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
	            ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                //esp_ota_abort(update_handle);
            }
            binary_file_length += data_read;
            ESP_LOGI(TAG, "Written image length %d", binary_file_length);
			timer = TIME_OUT_DELAY * 2;
        } else if (data_read == 0) {
			if (FW_complete_data_received == true){
          		ESP_LOGI(TAG, "Received complete firmware file..");
				break;
			}
        	vTaskDelay(500 / portTICK_PERIOD_MS);
			timer--;
			if (timer <= 0){
          		ESP_LOGI(TAG, "TIME-OUT, firmware packet not received...");
				break;
			}
        }
    }



    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (FW_complete_data_received != true) {
        ESP_LOGE(TAG, "Error in receiving complete file");
       // esp_ota_abort(update_handle);
		Upgrade_state = DEVICE_UPGRADE_FAIL;
		return false;
    }
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
		Upgrade_state = DEVICE_UPGRADE_FAIL;
		return false;
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
		Upgrade_state = DEVICE_UPGRADE_FAIL;
		return false;
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
	Upgrade_state = DEVICE_UPGRADE_SUCCESS;

    return true;
}

static bool diagnostic(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
//    io_conf.pin_bit_mask = (1ULL << CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Diagnostics (5 sec)...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

//    bool diagnostic_is_ok = gpio_get_level(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);

//    gpio_reset_pin(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
//    return diagnostic_is_ok;
	  return 0;
}

bool start_firmware_Upgrade(void)
{
	bool ret;

    ESP_LOGI(TAG, "OTA example app_main start");

    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...
            bool diagnostic_is_ok = diagnostic();
            if (diagnostic_is_ok) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
	
	ret = ota_firmware_upgrade();
	return ret;
}

bool Firmware_upgrade (void)
{
	API_DISP_Display_Screen(DISP_DEVICE_UPGRADING);

	if(start_firmware_Upgrade())
	{
		API_DISP_Display_Screen(DISP_DEVICE_UPGRADED);
	}

	else
	{
		API_DISP_Display_Screen(DISP_DEVICE_UPGRADATION_FAIL);
	}

	Delay_ms(5000);
	esp_restart();

	return true;
}
