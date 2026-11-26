#pragma once

#include "esp_err.h"

typedef void (*mqtt_message_callback_t)(
  const char *topic,
  int topic_len,
  const char *payload,
  int payload_len
);

esp_err_t app_mqtt_start(const char *broker_uri, const char *client_id);

esp_err_t app_mqtt_subscribe(const char *topic);

// func para pegar o payload e topic do broker

void app_mqtt_set_message_callback(mqtt_message_callback_t callback);
