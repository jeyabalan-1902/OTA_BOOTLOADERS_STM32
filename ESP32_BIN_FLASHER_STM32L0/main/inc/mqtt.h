#ifndef MQTT_H
#define MQTT_H

#include "ota_update.h"


#define MQTT_STM32_FIRMWARE             "onwords/stm32f/firmware"
#define MQTT_ESP32_FIRMWARE             "onwords/esp32/firmware"
#define MQTT_STATUS_PUBLISH             "onwords/ota/currentStatus"

void mqtt_app_start(void) ;
void send_mqtt_status(const char* status, const char* message);

extern esp_mqtt_client_handle_t mqtt_client;
extern bool mqtt_connected;

#endif