#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_err.h"

#include "wifi.h"
#include "mqtt_app.h"


#define BUTTON_GPIO GPIO_NUM_27
#define BUTTON_PRESSED_LEVEL 0

// Wi-Fi do ESP32 físico
#define WIFI_SSID "Murilo_quarto"
#define WIFI_PASSWORD "Daby450a"

// MQTT via WebSocket
#define MQTT_BROKER_URI "ws://test.mosquitto.org:8080/mqtt"

#define MQTT_TEST_TOPIC "gotham/dpgc/batsignal/test"

#define MQTT_BAT_SIGNAL_TOPIC "gotham/dpgc/batsignal"

#define MQTT_CLIENT_ID "GOTHAM_NODE_A"


static bool bat_signal_active = false;


// --------------------------------------------------
// Configuração do botão
// --------------------------------------------------
static void button_init(void)
{
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(
        gpio_config(&button_config)
    );
}


// --------------------------------------------------
// Alterna estado do Bat-Sinal e publica no MQTT
// --------------------------------------------------
static void toggle_bat_sinal(void)
{
    bat_signal_active = !bat_signal_active;

    esp_err_t ret;

    if (bat_signal_active) {

        printf("Bat-sinal: Ativado\n");

        ret = app_mqtt_publish(
            MQTT_BAT_SIGNAL_TOPIC,
            "BAT_SIGNAL_ON"
        );

    } else {

        printf("Bat-sinal: Desativado\n");

        ret = app_mqtt_publish(
            MQTT_BAT_SIGNAL_TOPIC,
            "BAT_SIGNAL_OFF"
        );
    }

    if (ret != ESP_OK) {
        printf(
            "Erro ao publicar estado do Bat-sinal\n"
        );
    }
}


// --------------------------------------------------
// Task responsável pelo botão
// --------------------------------------------------
static void button_task(void *pvParameters)
{
    while (1) {

        if (
            gpio_get_level(BUTTON_GPIO)
            == BUTTON_PRESSED_LEVEL
        ) {

            // Debounce do pressionamento
            vTaskDelay(
                pdMS_TO_TICKS(50)
            );

            if (
                gpio_get_level(BUTTON_GPIO)
                == BUTTON_PRESSED_LEVEL
            ) {

                printf("Botao pressionado\n");

                toggle_bat_sinal();

                // Aguarda liberação
                while (
                    gpio_get_level(BUTTON_GPIO)
                    == BUTTON_PRESSED_LEVEL
                ) {

                    vTaskDelay(
                        pdMS_TO_TICKS(20)
                    );
                }

                // Debounce da liberação
                vTaskDelay(
                    pdMS_TO_TICKS(50)
                );
            }
        }

        vTaskDelay(
            pdMS_TO_TICKS(20)
        );
    }
}


// --------------------------------------------------
// Aplicação principal
// --------------------------------------------------
void app_main(void)
{
    // 1. Inicializa o botão
    button_init();

    printf("Node A iniciado\n");
    printf("Bat-Sinal: OFF\n");


    // 2. Inicializa o Wi-Fi
    printf("Inicializando Wi-Fi...\n");

    ESP_ERROR_CHECK(
        wifi_init()
    );


    // 3. Conecta ao Wi-Fi
    ESP_ERROR_CHECK(
        wifi_connect(
            WIFI_SSID,
            WIFI_PASSWORD
        )
    );

    printf(
        "Wi-Fi conectado com sucesso\n"
    );


    // 4. Inicia e conecta ao MQTT
    ESP_ERROR_CHECK(
        app_mqtt_start(
            MQTT_BROKER_URI,
            MQTT_CLIENT_ID
        )
    );

    printf(
        "MQTT conectado com sucesso\n"
    );


    // 5. Publicação de teste
    ESP_ERROR_CHECK(
        app_mqtt_publish(
            MQTT_TEST_TOPIC,
            "NODE_A_ONLINE"
        )
    );


    // 6. Inicia monitoramento do botão
    xTaskCreate(
        button_task,
        "button_task",
        2048,
        NULL,
        5,
        NULL
    );
}