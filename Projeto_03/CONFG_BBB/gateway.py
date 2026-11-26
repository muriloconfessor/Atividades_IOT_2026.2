import sqlite3
from pathlib import Path
from datetime import datetime

import paho.mqtt.client as mqtt


# CONFIGURACOES
BROKER = "localhost"
PORTA = 1883

TOPICO = "gotham/dpgc/murilo_caua_esdras/batsignal"
TOPICO_COMANDO = "gotham/dpgc/murilo_caua_esdras/command"

BANCO = Path(__file__).resolve().parent / "gotham.db"


# GRAVAR EVENTO NO BANCO
def salvar_evento(topico, estado):

    conexao = sqlite3.connect(BANCO)
    cursor = conexao.cursor()

    timestamp = datetime.now().astimezone().isoformat(
        timespec="milliseconds"
    )

    cursor.execute("""
        INSERT INTO eventos (topico, estado, timestamp)
        VALUES (?, ?, ?)
    """, (topico, estado, timestamp))

    conexao.commit()
    conexao.close()

    print(f"Evento registrado: {estado} | {timestamp}")


# EVENTO DE CONEXAO MQTT
def ao_conectar(client, userdata, flags, reason_code, properties):

    if reason_code == 0:
        print("Gateway conectado ao Mosquitto!")

        client.subscribe(TOPICO, qos=1)

        print(f"Inscrito no topico: {TOPICO}")

    else:
        print(f"Falha na conexao MQTT: {reason_code}")


# EVENTO DE RECEBIMENTO MQTT
def ao_receber(client, userdata, mensagem):

    topico = mensagem.topic
    estado = mensagem.payload.decode("utf-8").strip()

    print(f"Mensagem recebida: {estado}")

    if estado in ["BAT_SIGNAL_ON", "BAT_SIGNAL_OFF"]:

        # 1. Registrar evento no banco de dados
        salvar_evento(topico, estado)

        # 2. Encaminhar o estado para o NODE_B
        resultado = client.publish(
            TOPICO_COMANDO,
            estado,
            qos=1,
            retain=False
        )

        if resultado.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"Estado encaminhado ao NODE_B: {estado}")
        else:
            print(f"Erro ao encaminhar estado: {resultado.rc}")

    else:
        print("Mensagem ignorada: estado desconhecido")


# INICIALIZACAO DO GATEWAY
def iniciar_gateway():

    cliente = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id="GOTHAM_GATEWAY"
    )

    cliente.on_connect = ao_conectar
    cliente.on_message = ao_receber

    print("Iniciando Gateway de Gotham...")

    cliente.connect(BROKER, PORTA, 60)

    cliente.loop_forever()


if __name__ == "__main__":
    iniciar_gateway()

