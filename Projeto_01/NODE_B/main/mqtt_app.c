#include "mqtt_app.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "mqtt_client.h"

#define MQTT_CONNECTED_BIT BIT0

static esp_mqtt_client_handle_t mqtt_client = NULL;
static EventGroupHandle_t mqtt_event_group = NULL;
static mqtt_message_callback_t message_callback = NULL;

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    switch ((esp_mqtt_event_id_t)event_id) {

        case MQTT_EVENT_CONNECTED:

            printf("MQTT: conectado ao broker\n");

            xEventGroupSetBits(
                mqtt_event_group,
                MQTT_CONNECTED_BIT
            );

            break;

        case MQTT_EVENT_DISCONNECTED:

            printf("MQTT: desconectado do broker\n");

            xEventGroupClearBits(
                mqtt_event_group,
                MQTT_CONNECTED_BIT
            );

            break;

        case MQTT_EVENT_SUBSCRIBED: {
            esp_mqtt_event_handle_t event =
                (esp_mqtt_event_handle_t)event_data;

            printf(
                "MQTT: inscrito no topico. msg_id = %d\n",
                event->msg_id
            );

            break;
        }

        case MQTT_EVENT_DATA: {
            esp_mqtt_event_handle_t event =
                (esp_mqtt_event_handle_t)event_data;

            printf("MQTT: mensagem recebida\n");

            printf(
                "Topico: %.*s\n",
                event->topic_len,
                event->topic
            );

            printf(
                "Payload: %.*s\n",
                event->data_len,
                event->data
            );

            if (message_callback != NULL) {
                message_callback(
                    event->topic,
                    event->topic_len,
                    event->data,
                    event->data_len
                );
            }

            break;
        }

        case MQTT_EVENT_ERROR:

            printf("MQTT: ocorreu um erro\n");

            break;

        default:
            break;
    }
}

esp_err_t app_mqtt_start(
    const char *broker_uri,
    const char *client_id
)
{
    mqtt_event_group = xEventGroupCreate();

    if (mqtt_event_group == NULL) {
        printf("MQTT: falha ao criar EventGroup\n");
        return ESP_FAIL;
    }

    /*
     * A URI deve ser, neste caso:
     *
     * ws://test.mosquitto.org:8080/mqtt
     *
     * Isso faz o ESP-MQTT usar WebSocket.
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

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL) {
        printf("MQTT: falha ao criar cliente MQTT\n");
        return ESP_FAIL;
    }

    esp_err_t ret = esp_mqtt_client_register_event(
        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    if (ret != ESP_OK) {
        printf("MQTT: falha ao registrar event handler\n");
        return ret;
    }

    printf(
        "MQTT: conectando ao broker %s\n",
        broker_uri
    );

    ret = esp_mqtt_client_start(mqtt_client);

    if (ret != ESP_OK) {
        printf("MQTT: falha ao iniciar o cliente\n");
        return ret;
    }

    /*
     * Espera até 15 segundos pelo MQTT_EVENT_CONNECTED.
     */
    EventBits_t bits = xEventGroupWaitBits(
        mqtt_event_group,
        MQTT_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(15000)
    );

    if (!(bits & MQTT_CONNECTED_BIT)) {
        printf("MQTT: timeout ao conectar ao broker\n");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t app_mqtt_subscribe(const char *topic)
{
    if (mqtt_client == NULL) {
        printf("MQTT: cliente nao inicializado\n");
        return ESP_ERR_INVALID_STATE;
    }

    EventBits_t bits = xEventGroupGetBits(
        mqtt_event_group
    );

    if (!(bits & MQTT_CONNECTED_BIT)) {
        printf("MQTT: cliente nao conectado\n");
        return ESP_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_subscribe(
        mqtt_client,
        topic,
        1
    );

    if (msg_id == -1) {
        printf("MQTT: falha ao assinar topico\n");
        return ESP_FAIL;
    }

    printf(
        "MQTT: solicitando assinatura de '%s'\n",
        topic
    );

    return ESP_OK;
}

void app_mqtt_set_message_callback(
    mqtt_message_callback_t callback
)
{
    message_callback = callback;
}