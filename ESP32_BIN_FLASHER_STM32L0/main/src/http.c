#include "http.h"

uint8_t *download_buffer = NULL;
size_t total_firmware_size = 0;
size_t bytes_downloaded = 0;
bool download_complete = false;
size_t allocated_buffer_size = 0;

extern bool firmware_update_requested;
extern char firmware_url[256];

static const char *TAG = "HTTP_HANDLER";

esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            bytes_downloaded = 0;
            download_complete = false;
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            if (strcasecmp(evt->header_key, "Content-Length") == 0) {
                total_firmware_size = atoi(evt->header_value);
                ESP_LOGI(TAG, "Firmware size: %zu bytes", total_firmware_size);
                
                if (total_firmware_size > MAX_FIRMWARE_SIZE) {
                    ESP_LOGE(TAG, "Firmware too large: %zu bytes (max: %d)", total_firmware_size, MAX_FIRMWARE_SIZE);
                    return ESP_FAIL;
                }
                
                allocated_buffer_size = total_firmware_size;
                
                if (!download_buffer) {
                    download_buffer = malloc(allocated_buffer_size);
                    if (!download_buffer) {
                        ESP_LOGE(TAG, "Failed to allocate download buffer of %zu bytes", allocated_buffer_size);
                        return ESP_ERR_NO_MEM;
                    }
                    ESP_LOGI(TAG, "Allocated download buffer: %zu bytes for %zu byte firmware", 
                            allocated_buffer_size, total_firmware_size);
                }
            }
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            
            if (download_buffer && evt->data_len > 0) {
                if (bytes_downloaded + evt->data_len <= allocated_buffer_size) {
                    memcpy(download_buffer + bytes_downloaded, evt->data, evt->data_len);
                    bytes_downloaded += evt->data_len;
                    
                    if (bytes_downloaded % 2048 == 0) {
                        ESP_LOGI(TAG, "Downloaded: %zu/%zu bytes (%.1f%%)", 
                               bytes_downloaded, total_firmware_size,
                               (float)bytes_downloaded * 100.0 / total_firmware_size);
                    }
                } else {
                    ESP_LOGE(TAG, "Download buffer overflow! Tried to write %d bytes at offset %zu (buffer size: %zu)", 
                           evt->data_len, bytes_downloaded, allocated_buffer_size);
                    return ESP_FAIL;
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP download completed. Total bytes: %zu/%zu", bytes_downloaded, total_firmware_size);
            download_complete = true;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

esp_err_t download_firmware(const char *url) {
    ESP_LOGI(TAG, "Downloading firmware from: %s", url);
    send_mqtt_status("Downloading", "bin file downloading");
    bytes_downloaded = 0;
    total_firmware_size = 0;
    download_complete = false;
    allocated_buffer_size = 0;
    
    if (download_buffer) {
        free(download_buffer);
        download_buffer = NULL;
    }
    
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .buffer_size = HTTP_BUFFER_SIZE,
        .timeout_ms = 60000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    
    if (err == ESP_OK && download_complete && bytes_downloaded > 0) {
        ESP_LOGI(TAG, "Firmware download completed successfully. Size: %zu bytes", bytes_downloaded);
        send_mqtt_status("Downloaded", "bin file downloaded successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Firmware download failed: %s", esp_err_to_name(err));
        send_mqtt_status("Failed", "bin file download failed");
        if (download_buffer) {
            free(download_buffer);
            download_buffer = NULL;
        }
        return ESP_FAIL;
    }
}
