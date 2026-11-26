import ssl
import time
import getpass
import paho.mqtt.client as mqtt

# Broker local (Gateway)
BROKER_LOCAL = "localhost"
PORTA_LOCAL = 1883

# Broker na nuvem (EMQX)
BROKER_NUVEM = "ba6b1b1b.ala.us-east-1.emqxsl.com"
PORTA_NUVEM = 8883

# Tópicos MQTT
TOPICO_TELEMETRIA = "gotham/dpgc/murilo_caua_esdras/batsignal"
TOPICO_COMANDO = "gotham/dpgc/murilo_caua_esdras/command"

USUARIO = "gotham_gateway"


# Conexão com a nuvem
def ao_conectar_nuvem(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Conectado ao EMQX na nuvem!")

        client.subscribe(TOPICO_COMANDO, qos=1)
        print(f"Monitorando comandos: {TOPICO_COMANDO}")
    else:
        print(f"Falha na conexao com a nuvem: {reason_code}")


# Conexão com o broker local
def ao_conectar_local(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Conectado ao Mosquitto local!")

        client.subscribe(TOPICO_TELEMETRIA, qos=1)
        print(f"Monitorando telemetria: {TOPICO_TELEMETRIA}")
    else:
        print(f"Falha na conexao local: {reason_code}")


# Gateway -> Nuvem
def encaminhar_telemetria(client, userdata, mensagem):
    estado = mensagem.payload.decode("utf-8").strip()

    print(f"Telemetria recebida do Gateway: {estado}")

    if not cliente_nuvem.is_connected():
        print("Nuvem desconectada. Telemetria nao encaminhada.")
        return

    resultado = cliente_nuvem.publish(
        TOPICO_TELEMETRIA,
        estado,
        qos=1,
        retain=True
    )

    if resultado.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"Telemetria encaminhada para a nuvem: {estado}")
    else:
        print(f"Erro ao encaminhar telemetria: {resultado.rc}")


# Nuvem -> Gateway
def encaminhar_comando(client, userdata, mensagem):
    comando = mensagem.payload.decode("utf-8").strip()

    print(f"Comando recebido da nuvem: {comando}")

    if comando not in ("BAT_SIGNAL_ON", "BAT_SIGNAL_OFF"):
        print("Comando desconhecido. Ignorando.")
        return

    if not cliente_local.is_connected():
        print("Gateway local desconectado. Comando nao encaminhado.")
        return

    resultado = cliente_local.publish(
        TOPICO_COMANDO,
        comando,
        qos=1,
        retain=False
    )

    if resultado.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"Comando encaminhado ao NODE_B: {comando}")
    else:
        print(f"Erro ao encaminhar comando: {resultado.rc}")


if __name__ == "__main__":

    senha = getpass.getpass("Digite a senha do usuario EMQX: ")

    # Cliente da nuvem
    cliente_nuvem = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id="GOTHAM_GATEWAY_CLOUD"
    )

    cliente_nuvem.username_pw_set(USUARIO, senha)

    cliente_nuvem.tls_set(
        cert_reqs=ssl.CERT_REQUIRED,
        tls_version=ssl.PROTOCOL_TLS_CLIENT
    )

    cliente_nuvem.on_connect = ao_conectar_nuvem
    cliente_nuvem.on_message = encaminhar_comando

    # Cliente local
    cliente_local = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id="GOTHAM_CLOUD_BRIDGE"
    )

    cliente_local.on_connect = ao_conectar_local
    cliente_local.on_message = encaminhar_telemetria

    try:
        print("Conectando ao EMQX na nuvem...")

        cliente_nuvem.connect(BROKER_NUVEM, PORTA_NUVEM, 60)
        cliente_nuvem.loop_start()

        for tentativa in range(15):
            if cliente_nuvem.is_connected():
                break
            time.sleep(1)

        if not cliente_nuvem.is_connected():
            raise ConnectionError("Nao foi possivel conectar ao EMQX.")

        print("Conectando ao Mosquitto local...")

        cliente_local.connect(BROKER_LOCAL, PORTA_LOCAL, 60)

        cliente_local.loop_forever()

    except KeyboardInterrupt:
        print("\nEncerrando a ponte MQTT...")

    finally:
        cliente_local.disconnect()
        cliente_nuvem.loop_stop()
        cliente_nuvem.disconnect()
