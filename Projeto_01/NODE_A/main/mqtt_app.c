#include "mqtt_app.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "mqtt_client.h"


#define MQTT_CONNECTED_BIT BIT0


static esp_mqtt_client_handle_t mqtt_client = NULL;

static EventGroupHandle_t mqtt_event_group = NULL;


// --------------------------------------------------
// Handler dos eventos MQTT
// --------------------------------------------------
static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    switch ((esp_mqtt_event_id_t)event_id) {

        case MQTT_EVENT_CONNECTED:

            printf(
                "MQTT: conectado ao broker\n"
            );

            xEventGroupSetBits(
                mqtt_event_group,
                MQTT_CONNECTED_BIT
            );

            break;


        case MQTT_EVENT_DISCONNECTED:

            printf(
                "MQTT: desconectado do broker\n"
            );

            xEventGroupClearBits(
                mqtt_event_group,
                MQTT_CONNECTED_BIT
            );

            break;


        case MQTT_EVENT_PUBLISHED: {

            esp_mqtt_event_handle_t event =
                (esp_mqtt_event_handle_t)event_data;

            printf(
                "MQTT: mensagem publicada. msg_id = %d\n",
                event->msg_id
            );

            break;
        }


        case MQTT_EVENT_ERROR:

            printf(
                "MQTT: ocorreu um erro\n"
            );

            break;


        default:
            break;
    }
}


// --------------------------------------------------
// Inicia o cliente MQTT
// --------------------------------------------------
esp_err_t app_mqtt_start(
    const char *broker_uri,
    const char *client_id
)
{
    if (broker_uri == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (client_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }


    // Cria o Event Group
    mqtt_event_group = xEventGroupCreate();

    if (mqtt_event_group == NULL) {

        printf(
            "MQTT: falha ao criar Event Group\n"
        );

        return ESP_ERR_NO_MEM;
    }


    /*
     * Configuração do cliente MQTT.
     *
     * A URI vem do main.c:
     *
     * ws://test.mosquitto.org:8080/mqtt
     *
     * O prefixo ws:// faz o ESP-MQTT usar WebSocket.
     */
    esp_mqtt_client_config_t mqtt_config = {

        .broker.address.uri = broker_uri,

        .credentials = {
            .client_id = client_id
        },

        .network = {
            .timeout_ms = 15000,
            .reconnect_timeout_ms = 5000
        },

        .session = {
            .keepalive = 60
        }
    };


    // Cria o cliente MQTT
    mqtt_client =
        esp_mqtt_client_init(
            &mqtt_config
        );

    if (mqtt_client == NULL) {

        printf(
            "MQTT: falha ao inicializar cliente\n"
        );

        return ESP_FAIL;
    }


    // Registra o handler de eventos
    esp_err_t ret =
        esp_mqtt_client_register_event(
            mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL
        );

    if (ret != ESP_OK) {

        printf(
            "MQTT: falha ao registrar event handler\n"
        );

        return ret;
    }


    printf(
        "MQTT: conectando ao broker %s\n",
        broker_uri
    );


    // Inicia o cliente
    ret =
        esp_mqtt_client_start(
            mqtt_client
        );

    if (ret != ESP_OK) {

        printf(
            "MQTT: falha ao iniciar cliente\n"
        );

        return ret;
    }


    printf(
        "MQTT: aguardando conexao com broker...\n"
    );


    /*
     * Aguarda até 15 segundos pela conexão.
     */
    EventBits_t bits =
        xEventGroupWaitBits(
            mqtt_event_group,
            MQTT_CONNECTED_BIT,
            pdFALSE,
            pdTRUE,
            pdMS_TO_TICKS(15000)
        );


    if (!(bits & MQTT_CONNECTED_BIT)) {

        printf(
            "MQTT: timeout ao conectar ao broker\n"
        );

        return ESP_ERR_TIMEOUT;
    }


    return ESP_OK;
}


// --------------------------------------------------
// Publica uma mensagem MQTT
// --------------------------------------------------
esp_err_t app_mqtt_publish(
    const char *topic,
    const char *payload
)
{
    if (mqtt_client == NULL) {

        printf(
            "MQTT: cliente nao foi inicializado\n"
        );

        return ESP_ERR_INVALID_STATE;
    }


    if (topic == NULL || payload == NULL) {
        return ESP_ERR_INVALID_ARG;
    }


    EventBits_t bits =
        xEventGroupGetBits(
            mqtt_event_group
        );


    if (!(bits & MQTT_CONNECTED_BIT)) {

        printf(
            "MQTT: cliente nao conectado\n"
        );

        return ESP_ERR_INVALID_STATE;
    }


    int msg_id =
        esp_mqtt_client_publish(
            mqtt_client,
            topic,
            payload,
            strlen(payload),
            1,
            0
        );


    if (msg_id == -1) {

        printf(
            "MQTT: falha ao publicar a mensagem\n"
        );

        return ESP_FAIL;
    }


    printf(
        "MQTT: publicando '%s' em '%s'\n",
        payload,
        topic
    );


    return ESP_OK;
}