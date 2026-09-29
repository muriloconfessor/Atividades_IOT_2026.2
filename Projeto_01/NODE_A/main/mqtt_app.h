#ifndef MQTT_APP_H
#define MQTT_APP_H

#include "esp_err.h"

esp_err_t app_mqtt_start(
    const char *broker_uri,
    const char *client_id
);

esp_err_t app_mqtt_publish(
    const char *topic,
    const char *payload
);

#endif