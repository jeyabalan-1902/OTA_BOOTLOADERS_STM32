
#include "flash_cmd.h"

static const char *TAG = "FLASH_CMD";

esp_err_t send_sync_command(void) {
    ESP_LOGI(TAG, "Sending sync command (0x50) twice with 1000ms interval");
    
    uart_flush_rx_buffer();
    
    for (int i = 0; i < 2; i++) {
        ESP_LOGI(TAG, "Sending sync command %d/2", i + 1);
        uart_write_byte(0x50);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    vTaskDelay(pdMS_TO_TICKS(500));
    send_mqtt_status("Success", "Bootloader started successfully");
    return ESP_OK;
}

esp_err_t send_get_cid_command(void) {
    ESP_LOGI(TAG, "Command ==> BL_GET_CID");
    send_mqtt_status("Requested", "Requested Chip ID");
    uart_flush_rx_buffer();
    
    uint8_t data_buf[COMMAND_BL_GET_CID_LEN];
    data_buf[0] = COMMAND_BL_GET_CID_LEN - 1;  
    data_buf[1] = COMMAND_BL_GET_CID;          
    
    uint32_t crc32 = get_crc(data_buf, COMMAND_BL_GET_CID_LEN - 4);
    data_buf[2] = word_to_byte(crc32, 1);
    data_buf[3] = word_to_byte(crc32, 2);
    data_buf[4] = word_to_byte(crc32, 3);
    data_buf[5] = word_to_byte(crc32, 4);
    send_bootloader_packet(data_buf, COMMAND_BL_GET_CID_LEN);
    
    uint16_t chip_id;
    size_t response_len;
    if (read_bootloader_reply(COMMAND_BL_GET_CID, (uint8_t*)&chip_id, &response_len) == ESP_OK) {
        ESP_LOGI(TAG, "Chip ID: 0x%04x", chip_id);
        char chipId[50];
        snprintf(chipId, sizeof(chipId), "Chip ID: 0x%04x...got successfully", chip_id);
        send_mqtt_status("Success", chipId);
        return ESP_OK;
    }   
    return ESP_FAIL;
}

esp_err_t send_flash_erase_command(uint8_t sector, uint8_t num_sectors) {
    ESP_LOGI(TAG, "Command ==> BL_FLASH_ERASE");
    send_mqtt_status("Sending", "Sending flash erase cmd");
    uart_flush_rx_buffer();
    
    uint8_t data_buf[COMMAND_BL_FLASH_ERASE_LEN];
    data_buf[0] = COMMAND_BL_FLASH_ERASE_LEN - 1;
    data_buf[1] = COMMAND_BL_FLASH_ERASE;
    data_buf[2] = sector;
    data_buf[3] = num_sectors;
    
    uint32_t crc32 = get_crc(data_buf, COMMAND_BL_FLASH_ERASE_LEN - 4);
    data_buf[4] = word_to_byte(crc32, 1);
    data_buf[5] = word_to_byte(crc32, 2);
    data_buf[6] = word_to_byte(crc32, 3);
    data_buf[7] = word_to_byte(crc32, 4);
    
    send_bootloader_packet(data_buf, COMMAND_BL_FLASH_ERASE_LEN);
    
    uint8_t erase_status;
    size_t response_len;
    if (read_bootloader_reply(COMMAND_BL_FLASH_ERASE, &erase_status, &response_len) == ESP_OK) {
        if (erase_status == Flash_HAL_OK) {
            ESP_LOGI(TAG, "Erase Status: Success Code: Flash_HAL_OK");
            send_mqtt_status("Success", "Flash erased successfully");
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Erase Status: Fail Code: 0x%02x", erase_status);
            return ESP_FAIL;
        }
    }   
    return ESP_FAIL;
}

esp_err_t send_mem_write_command(uint32_t base_address, uint8_t *data, uint8_t length) {
    ESP_LOGD(TAG, "Command ==> BL_MEM_WRITE - Address: 0x%08" PRIx32 ", Length: %d", base_address, length);
    
    uart_flush_rx_buffer();
    
    uint32_t mem_write_cmd_total_len = COMMAND_BL_MEM_WRITE_BASE_LEN + length + 4; 
    uint8_t data_buf[mem_write_cmd_total_len];
    
    data_buf[0] = mem_write_cmd_total_len - 1;    
    data_buf[1] = COMMAND_BL_MEM_WRITE;            
    data_buf[2] = word_to_byte(base_address, 1);   
    data_buf[3] = word_to_byte(base_address, 2);
    data_buf[4] = word_to_byte(base_address, 3);
    data_buf[5] = word_to_byte(base_address, 4);
    data_buf[6] = length;                          
    
    memcpy(&data_buf[7], data, length);

    uint32_t crc32 = get_crc(&data_buf[0], mem_write_cmd_total_len - 4);
    data_buf[7 + length] = word_to_byte(crc32, 1);
    data_buf[8 + length] = word_to_byte(crc32, 2);
    data_buf[9 + length] = word_to_byte(crc32, 3);
    data_buf[10 + length] = word_to_byte(crc32, 4);
    
    send_bootloader_packet(data_buf, mem_write_cmd_total_len);

    vTaskDelay(pdMS_TO_TICKS(100));
    
    uint8_t write_status;
    size_t response_len;
    if (read_bootloader_reply(COMMAND_BL_MEM_WRITE, &write_status, &response_len) == ESP_OK) {
        if (write_status == Flash_HAL_OK) {
            ESP_LOGD(TAG, "Write_status: Flash_HAL_OK");
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Write_status: FAIL - Code: 0x%02x", write_status);
            return ESP_FAIL;
        }
    } 
    return ESP_FAIL;
}

esp_err_t send_go_reset() {
    ESP_LOGI(TAG, "Command ==> BL_GO_TO_ADDR");
    
    uart_flush_rx_buffer();
    
    uint8_t data_buf[COMMAND_BL_GO_TO_RESET_LEN];
    data_buf[0] = COMMAND_BL_GO_TO_RESET_LEN - 1;
    data_buf[1] = COMMAND_BL_GO_TO_RESET;
    uint32_t crc32 = get_crc(data_buf, COMMAND_BL_GO_TO_RESET_LEN - 4);
    data_buf[2] = word_to_byte(crc32, 1);
    data_buf[3] = word_to_byte(crc32, 2);
    data_buf[4] = word_to_byte(crc32, 3);
    data_buf[5] = word_to_byte(crc32, 4);
    
    send_bootloader_packet(data_buf, COMMAND_BL_GO_TO_RESET_LEN);
  
    uint8_t addr_status;
    size_t response_len;
    if (read_bootloader_reply(COMMAND_BL_GO_TO_RESET, &addr_status, &response_len) == ESP_OK) {
        ESP_LOGI(TAG, "Address Status: 0x%02x", addr_status);
        send_mqtt_status("Success", "Go to reset cmd executed succesfully");
        return ESP_OK;
    }   
    return ESP_FAIL;
}

esp_err_t flash_downloaded_firmware(void) {
    if (!download_buffer || bytes_downloaded == 0) {
        ESP_LOGE(TAG, "No firmware data to flash");
        return ESP_FAIL;
    }
    
    send_mqtt_status("Starting", "STM32 firmware flashing started");
    ESP_LOGI(TAG, "Starting STM32 firmware flashing");
    
    // Step 1: Send sync command
   send_mqtt_status("Sending", "Sending bootloader enable cmd");
    ESP_LOGI(TAG, "Step 1: Sending sync commands");
    if (send_sync_command() != ESP_OK) {
        ESP_LOGE(TAG, "Sync command failed");
        send_mqtt_status("Failed", "Bootloader enable failed");
        return ESP_FAIL;
    }
    
    // Step 2: Get version
    ESP_LOGI(TAG, "Step 2: Checking bootloader version");
    if (send_get_cid_command() != ESP_OK) {
        ESP_LOGE(TAG, "Get version command failed");
        send_mqtt_status("Failed", "Bootloader version get failed");
        return ESP_FAIL;
    }
    
    // Step 3: Flash erase
    ESP_LOGI(TAG, "Step 3: Erasing flash sectors");
    if (send_flash_erase_command(FLASH_ERASE_BASE_SECTOR, FLASH_ERASE_SECTORS) != ESP_OK) {
        ESP_LOGE(TAG, "Flash erase command failed");
        send_mqtt_status("Failed", "Flash erase failed");
        return ESP_FAIL;
    }
    
    // Step 4: Write firmware data
    ESP_LOGI(TAG, "Step 4: Writing firmware data (%zu bytes)", bytes_downloaded);
    send_mqtt_status("Starting", "Firmware writing started");
    uint32_t base_mem_address = FLASH_BASE_ADDRESS;
    size_t bytes_remaining = bytes_downloaded;
    size_t bytes_sent = 0;
    uint8_t *data_ptr = download_buffer;
    int retry_count = 0;
    const int max_retries = 3;
    
    while (bytes_remaining > 0) {
        size_t len_to_read;
        
        if (bytes_remaining >= 128) {
            len_to_read = 128;
        } else {
            len_to_read = bytes_remaining;
        }
        
        ESP_LOGI(TAG, "base mem address = 0x%08" PRIx32, base_mem_address);
        ESP_LOGI(TAG, "bytes_so_far_sent:%zu -- bytes_remaining:%zu", bytes_sent, bytes_remaining);
        
        esp_err_t result = send_mem_write_command(base_mem_address, data_ptr, len_to_read);
        
        if (result == ESP_OK) {
            base_mem_address += len_to_read;
            data_ptr += len_to_read;
            bytes_sent += len_to_read;
            bytes_remaining -= len_to_read;
            retry_count = 0;
            float progress_percentage = (float)bytes_sent * 100.0 / bytes_downloaded;
    
            char status_firm[300];
            snprintf(status_firm, sizeof(status_firm), "Flash progress: %zu/%zu bytes (%.1f%%)", 
                    bytes_sent, bytes_downloaded, progress_percentage);
            send_mqtt_status("Writing", status_firm);
            
            ESP_LOGI(TAG, "Flash progress: %zu/%zu bytes (%.1f%%)", 
                    bytes_sent, bytes_downloaded, progress_percentage);
        } 
        else {
            retry_count++;
            if (retry_count <= max_retries) {
                ESP_LOGW(TAG, "Write failed, retrying (%d/%d)...", retry_count, max_retries);
                vTaskDelay(pdMS_TO_TICKS(200));
            } else {
                ESP_LOGE(TAG, "Failed to write chunk after %d retries", max_retries);
                send_mqtt_status("Failed", "Firmware flash write failed");
                return ESP_FAIL;
            }
        }
    }

    //Step 5: Go to Reset command
    ESP_LOGI(TAG, "Step 5: Firmware write completed, sending RESET command");
    send_mqtt_status("Completed", "Firmware write completed, sending GO to RESET command");
    
    if (send_go_reset() != ESP_OK) {
        ESP_LOGE(TAG, "GO command failed");
        send_mqtt_status("Failed", "goto reset failed");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "STM32 firmware update completed successfully!");
    return ESP_OK;
}