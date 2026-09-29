#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_err.h"
#include "wifi.h"
#include "mqtt_app.h"

#define MQTT_BROKER_URI "ws://test.mosquitto.org:8080/mqtt"

#define MQTT_BAT_SIGNAL_TOPIC "gotham/dpgc/batsignal"

#define WIFI_SSID     "GCOMPI"
#define WIFI_PASSWORD "Ifpb@Gcompi10"

#define LED_GPIO GPIO_NUM_14

#define MQTT_CLIENT_ID "GOTHAM_NODE_B"

static void led_on(void)
{
    gpio_set_level(LED_GPIO, 1);
}

static void led_off(void)
{
    gpio_set_level(LED_GPIO, 0);
}

static void led_init(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    led_off();
}

static bool mqtt_text_equals(
    const char *data,
    int data_len,
    const char *expected
)
{
    int expected_len = strlen(expected);

    return data_len == expected_len &&
           strncmp(data, expected, expected_len) == 0;
}

static void on_mqtt_message(
    const char *topic,
    int topic_len,
    const char *payload,
    int payload_len
)
{
    if (!mqtt_text_equals(topic, topic_len, MQTT_BAT_SIGNAL_TOPIC)) {
        return;
    }

    if (mqtt_text_equals(payload, payload_len, "BAT_SIGNAL_ON")) {
        printf("Bat-sinal recebido: ON\n");
        led_on();
    }
    else if (mqtt_text_equals(payload, payload_len, "BAT_SIGNAL_OFF")) {
        printf("Bat-sinal recebido: OFF\n");
        led_off();
    }
    else {
        printf("Payload nao reconhecido.\n");
    }
}

void app_main(void)
{
    led_init();

    printf("Inicializando WIFI...\n");

    ESP_ERROR_CHECK(wifi_init());

    ESP_ERROR_CHECK(
        wifi_connect(WIFI_SSID, WIFI_PASSWORD)
    );

    printf("WIFI conectado com sucesso.\n");

    ESP_ERROR_CHECK(
        app_mqtt_start(
            MQTT_BROKER_URI,
            MQTT_CLIENT_ID
        )
    );

    printf("MQTT conectado com sucesso.\n");

    app_mqtt_set_message_callback(on_mqtt_message);

    ESP_ERROR_CHECK(
        app_mqtt_subscribe(MQTT_BAT_SIGNAL_TOPIC)
    );
}