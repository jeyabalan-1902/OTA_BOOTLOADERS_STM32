#include "ota_update.h"
#include "flash_cmd.h"
#include "wifi.h"
#include "http.h"
#include "mqtt.h"


bool firmware_update_requested = false;
char firmware_url[256];

static const char *TAG = "OTA_UPDATE";

static esp_err_t flash_stm32_firmware(void) {
    ESP_LOGI(TAG, "Starting STM32 firmware update process");
    
    if (download_firmware(firmware_url) != ESP_OK) {
        ESP_LOGE(TAG, "Firmware download failed");
        return ESP_FAIL;
    }
    
    esp_err_t result = flash_downloaded_firmware();
    
    if (download_buffer) {
        free(download_buffer);
        download_buffer = NULL;
    }
    
    return result;
}

static void firmware_update_task(void *pvParameters) {
    while (1) {
        if (firmware_update_requested) {
            firmware_update_requested = false;
            
            ESP_LOGI(TAG, "Processing firmware update request");
            
            if (flash_stm32_firmware() == ESP_OK) {
                ESP_LOGI(TAG, "Firmware update completed successfully");
                send_mqtt_status("Success", "STM32 firmware updated successfully");
            } else {
                ESP_LOGE(TAG, "Firmware update failed");
                send_mqtt_status("Failed", "STM32 firmware update failed");
            }
        }       
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "STM32 Bootloader with MQTT starting...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    uart_init();
    ESP_LOGI(TAG, "UART initialized");

    wifi_init_sta();
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    mqtt_app_start();
    ESP_LOGI(TAG, "MQTT client started");

    xTaskCreatePinnedToCore(firmware_update_task, "firmware_update", 8192, NULL, 5, NULL, 1);
    
    ESP_LOGI(TAG, "System ready. Waiting for firmware update requests...");
}
