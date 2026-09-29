#include "wifi.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "nvs_flash.h"

#define TAG "wifi"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define WIFI_RETRY_ATTEMPT 3


static int wifi_retry_count = 0;

static esp_netif_t *wifi_netif = NULL;

static esp_event_handler_instance_t ip_event_handler;
static esp_event_handler_instance_t wifi_event_handler;

static EventGroupHandle_t wifi_event_group = NULL;


// handler do wifi para testar eventos
static void wifi_event_cb(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    ESP_LOGI(
        TAG,
        "Handling Wi-Fi event, event code: %ld",
        event_id
    );

    switch (event_id) {

        case WIFI_EVENT_STA_START:

            ESP_LOGI(
                TAG,
                "Wi-Fi started, connecting to AP..."
            );

            esp_wifi_connect();
            break;


        case WIFI_EVENT_STA_CONNECTED:

            ESP_LOGI(
                TAG,
                "Wi-Fi connected"
            );

            break;


        case WIFI_EVENT_STA_DISCONNECTED:

            ESP_LOGI(
                TAG,
                "Wi-Fi disconnected"
            );

            if (wifi_retry_count < WIFI_RETRY_ATTEMPT) {

                ESP_LOGI(
                    TAG,
                    "Retrying to connect to Wi-Fi network..."
                );

                wifi_retry_count++;

                esp_wifi_connect();

            } else {

                xEventGroupSetBits(
                    wifi_event_group,
                    WIFI_FAIL_BIT
                );
            }

            break;


        case WIFI_EVENT_STA_STOP:

            ESP_LOGI(
                TAG,
                "Wi-Fi stopped"
            );

            break;


        default:
            break;
    }
}

static void ip_event_cb(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    ESP_LOGI(
        TAG,
        "Handling IP event, event code: %ld",
        event_id
    );

    if (event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *) event_data;

        ESP_LOGI(
            TAG,
            "Got IP: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        wifi_retry_count = 0;

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}
esp_err_t wifi_init(void)
{
    esp_err_t ret;


    // Inicializa NVS
    ret = nvs_flash_init();

    if (
        ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND
    ) {
        ESP_ERROR_CHECK(
            nvs_flash_erase()
        );

        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to initialize NVS"
        );

        return ret;
    }


    wifi_event_group =
        xEventGroupCreate();


    ret = esp_netif_init();

    if (ret != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Failed to initialize TCP/IP network stack"
        );

        return ret;
    }


    ret = esp_event_loop_create_default();

    if (ret != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Failed to create default event loop"
        );

        return ret;
    }


    wifi_netif =
        esp_netif_create_default_wifi_sta();

    if (wifi_netif == NULL) {

        ESP_LOGE(
            TAG,
            "Failed to create default Wi-Fi STA interface"
        );

        return ESP_FAIL;
    }


    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();


    ret = esp_wifi_init(&cfg);

    if (ret != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Failed to initialize Wi-Fi driver"
        );

        return ret;
    }


    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_cb,
            NULL,
            &wifi_event_handler
        )
    );


    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            &ip_event_cb,
            NULL,
            &ip_event_handler
        )
    );


    return ESP_OK;
}

esp_err_t wifi_connect(
    const char *wifi_ssid,
    const char *wifi_password
)
{
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_OPEN
        }
    };


    strncpy(
        (char *) wifi_config.sta.ssid,
        wifi_ssid,
        sizeof(wifi_config.sta.ssid)
    );


    strncpy(
        (char *) wifi_config.sta.password,
        wifi_password,
        sizeof(wifi_config.sta.password)
    );


    ESP_ERROR_CHECK(
        esp_wifi_set_ps(WIFI_PS_NONE)
    );


    ESP_ERROR_CHECK(
        esp_wifi_set_storage(
            WIFI_STORAGE_RAM
        )
    );


    ESP_ERROR_CHECK(
        esp_wifi_set_mode(
            WIFI_MODE_STA
        )
    );


    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );


    ESP_LOGI(
        TAG,
        "Connecting to Wi-Fi network: %s",
        wifi_config.sta.ssid
    );


    ESP_ERROR_CHECK(
        esp_wifi_start()
    );


    EventBits_t bits =
        xEventGroupWaitBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT |
            WIFI_FAIL_BIT,

            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );


    if (bits & WIFI_CONNECTED_BIT) {

        ESP_LOGI(
            TAG,
            "Connected to Wi-Fi network: %s",
            wifi_config.sta.ssid
        );

        return ESP_OK;
    }


    if (bits & WIFI_FAIL_BIT) {

        ESP_LOGE(
            TAG,
            "Failed to connect to Wi-Fi network: %s",
            wifi_config.sta.ssid
        );

        return ESP_FAIL;
    }


    ESP_LOGE(
        TAG,
        "Unexpected Wi-Fi error"
    );

    return ESP_FAIL;
}

esp_err_t wifi_disconnect(void)
{
    if (wifi_event_group != NULL) {

        vEventGroupDelete(
            wifi_event_group
        );

        wifi_event_group = NULL;
    }

    return esp_wifi_disconnect();
}

esp_err_t app_wifi_deinit(void)
{
    esp_err_t ret =
        esp_wifi_stop();


    if (
        ret != ESP_OK &&
        ret != ESP_ERR_WIFI_NOT_INIT
    ) {
        return ret;
    }


    ESP_ERROR_CHECK(
        esp_wifi_deinit()
    );


    if (wifi_netif != NULL) {

        esp_netif_destroy(
            wifi_netif
        );

        wifi_netif = NULL;
    }


    ESP_ERROR_CHECK(
        esp_event_handler_instance_unregister(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            ip_event_handler
        )
    );


    ESP_ERROR_CHECK(
        esp_event_handler_instance_unregister(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_event_handler
        )
    );


    return ESP_OK;
}