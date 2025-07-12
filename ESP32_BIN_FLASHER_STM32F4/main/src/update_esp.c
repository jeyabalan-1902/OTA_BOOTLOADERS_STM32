#include "update_esp.h"

#define MAX_LEN 32

static const char *OTA_TAG = "esp_update";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

void ota_task(char *ota_firmware_url)
{
    ESP_LOGI(OTA_TAG, "Starting OTA task");
    send_mqtt_status("Starting","ESP32 ota updated started");
    esp_http_client_config_t config = {
        .url = ota_firmware_url,
        .cert_pem = (char *)server_cert_pem_start,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    ESP_LOGI(OTA_TAG, "Attempting to download update from %s", config.url);
    send_mqtt_status("downloading", "downloading firmware file");
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        send_mqtt_status("Success", "ESP32 OTA update completed");
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        ESP_LOGI(OTA_TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        send_mqtt_status("Failed", "ESP32 OTA update failed");
        ESP_LOGE(OTA_TAG, "Firmware upgrade failed");
    }
}

static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[MAX_LEN * 2 + 1];
    hash_print[MAX_LEN * 2] = 0;
    for (int i = 0; i < MAX_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(OTA_TAG, "%s %s", label, hash_print);
}

static void get_sha256_of_partitions(void)
{
    uint8_t sha_256[MAX_LEN] = { 0 };
    esp_partition_t partition;
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
}

void ota_init(char *ota_firmware_url)
{
    get_sha256_of_partitions();
    ota_task(ota_firmware_url);
}