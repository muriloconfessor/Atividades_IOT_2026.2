#pragma once

#include "esp_err.h"

esp_err_t wifi_init(void);

esp_err_t wifi_connect(
  const char *wifi_ssid,
  const char *wifi_password
);

esp_err_t wifi_disconnect(void);

esp_err_t app_wifi_deinit(void);