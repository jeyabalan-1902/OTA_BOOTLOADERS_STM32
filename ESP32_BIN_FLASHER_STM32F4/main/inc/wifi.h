#ifndef WIFI_H
#define WIFI_H

#include "ota_update.h"

#define WIFI_SSID                       "Rnd"
#define WIFI_PASS                       "nikhil8182"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define MAX_RETRY 5

void wifi_init_sta(void);
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#endif