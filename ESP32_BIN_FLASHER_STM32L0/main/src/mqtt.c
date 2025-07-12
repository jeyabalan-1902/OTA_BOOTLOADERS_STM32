#include "mqtt.h"
#include "update_esp.h"

static const char *TAG = "MQTT_HANDLER";

esp_mqtt_client_handle_t mqtt_client;
bool mqtt_connected = false;
extern bool firmware_update_requested;
extern char firmware_url[256];

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_SERVER_CONNECTED");
        mqtt_connected = true;
        esp_mqtt_client_subscribe(client, MQTT_STM32_FIRMWARE, 1);
        esp_mqtt_client_subscribe(client, MQTT_ESP32_FIRMWARE, 1);
        send_mqtt_status("Connected", "Device ready to receive cmd");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        
        if(strncmp(MQTT_STM32_FIRMWARE, event->topic, event->topic_len) == 0 && strlen(MQTT_STM32_FIRMWARE) == event->topic_len)
        {
            char *json_string = malloc(event->data_len + 1);
            if (json_string) {
                memcpy(json_string, event->data, event->data_len);
                json_string[event->data_len] = '\0';
                
                cJSON *json = cJSON_Parse(json_string);
                if (json) {
                    cJSON *firmware_sts = cJSON_GetObjectItem(json, "firmware_sts");
                    cJSON *firmwareUrl = cJSON_GetObjectItem(json, "firmwareUrl");
                    
                    if (cJSON_IsNumber(firmware_sts) && cJSON_IsString(firmwareUrl)) {
                        if (firmware_sts->valueint == 1) {
                            ESP_LOGI(TAG, "Firmware update requested: %s", firmwareUrl->valuestring);
                            strncpy(firmware_url, firmwareUrl->valuestring, sizeof(firmware_url) - 1);
                            firmware_update_requested = true;
                        }
                    }
                    cJSON_Delete(json);
                }
                free(json_string);
            }
        }
        else if (strncmp(MQTT_ESP32_FIRMWARE, event->topic, event->topic_len) == 0 && strlen(MQTT_ESP32_FIRMWARE) == event->topic_len)
        {
            cJSON *json = cJSON_Parse(event->data);
            if (cJSON_HasObjectItem(json,"firmwareUrl") && cJSON_HasObjectItem(json,"firmware_sts"))
            {
                ESP_LOGI(TAG, "Topic match success!");
                cJSON *ota_url_json = cJSON_GetObjectItem(json, "firmwareUrl");
                cJSON *ota_sts_json = cJSON_GetObjectItem(json, "firmware_sts");
                char *ota_url = ota_url_json -> valuestring ;
                int ota_sts = ota_sts_json -> valueint ;
                if (ota_sts == 1)
                {
                    ota_init(ota_url);
                }
            }
        }
        
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void send_mqtt_status(const char* status, const char* message) {
    if (!mqtt_connected) return;
    
    cJSON *json = cJSON_CreateObject();
    cJSON *status_item = cJSON_CreateString(status);
    cJSON *message_item = cJSON_CreateString(message);
    
    cJSON_AddItemToObject(json, "status", status_item);
    cJSON_AddItemToObject(json, "message", message_item);
    
    char *json_string = cJSON_Print(json);
    esp_mqtt_client_publish(mqtt_client, MQTT_STATUS_PUBLISH, json_string, 0, 1, 0);
    
    free(json_string);
    cJSON_Delete(json);
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://Nikhil:Nikhil8182@mqtt.onwords.in",
        //.broker.address.uri = "mqtt://Bala:19022004@49.206.117.106:1883",
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}