
from flask import Flask, jsonify, render_template, request
import sqlite3
from pathlib import Path
from datetime import datetime, timedelta
from paho.mqtt import publish as mqtt_publish

app = Flask(__name__)

# Caminho do banco de dados
BANCO = Path(__file__).resolve().parent / "gotham.db"

# Configurações MQTT
MQTT_BROKER = "localhost"
MQTT_PORTA = 1883

TOPICO_COMANDO = "gotham/dpgc/murilo_caua_esdras/command"


def consultar_banco():
    conexao = sqlite3.connect(BANCO)
    conexao.row_factory = sqlite3.Row
    return conexao


# Página principal do Dashboard
@app.route("/")
def inicio():
    return render_template("index.html")


# Consulta o estado mais recente do Bat-Sinal
@app.route("/api/status")
def consultar_status():
    conexao = consultar_banco()
    cursor = conexao.cursor()

    cursor.execute("""
        SELECT estado, timestamp
        FROM eventos
        ORDER BY id DESC
        LIMIT 1
    """)

    ultimo_evento = cursor.fetchone()
    conexao.close()

    if ultimo_evento:
        return jsonify({
            "estado": ultimo_evento["estado"],
            "timestamp": ultimo_evento["timestamp"]
        })

    return jsonify({
        "estado": "SEM_DADOS",
        "timestamp": None
    })


# Consulta os eventos das últimas 24 horas
@app.route("/api/historico")
def consultar_historico():
    conexao = consultar_banco()
    cursor = conexao.cursor()

    limite = (
        datetime.now().astimezone() - timedelta(hours=24)
    ).isoformat(timespec="milliseconds")

    cursor.execute("""
        SELECT estado, timestamp
        FROM eventos
        WHERE timestamp < ?
        ORDER BY id DESC
        LIMIT 1
    """, (limite,))

    evento_anterior = cursor.fetchone()

    cursor.execute("""
        SELECT estado, timestamp
        FROM eventos
        WHERE timestamp >= ?
        ORDER BY id ASC
    """, (limite,))

    eventos = [dict(registro) for registro in cursor.fetchall()]

    if evento_anterior:
        eventos.insert(0, dict(evento_anterior))

    conexao.close()

    return jsonify(eventos)


# Envia comandos MQTT para controlar o Bat-Sinal
@app.route("/api/comando", methods=["POST"])
def enviar_comando():

    dados = request.get_json(silent=True)

    if not dados:
        return jsonify({
            "sucesso": False,
            "mensagem": "Dados inválidos."
        }), 400

    comando = dados.get("comando")

    # Aceita somente os dois comandos autorizados
    if comando not in ["BAT_SIGNAL_ON", "BAT_SIGNAL_OFF"]:
        return jsonify({
            "sucesso": False,
            "mensagem": "Comando não permitido."
        }), 400

    try:
        mqtt_publish.single(
            topic=TOPICO_COMANDO,
            payload=comando,
            hostname=MQTT_BROKER,
            port=MQTT_PORTA,
            qos=1,
            retain=False
        )

        print(f"Comando enviado pelo Dashboard: {comando}")

        return jsonify({
            "sucesso": True,
            "mensagem": "Comando enviado com sucesso!",
            "comando": comando
        })

    except Exception as erro:
        print(f"Erro ao enviar comando MQTT: {erro}")

        return jsonify({
            "sucesso": False,
            "mensagem": "Não foi possível enviar o comando MQTT."
        }), 500


if __name__ == "__main__":
    print("Iniciando o Dashboard de Gotham...")
    app.run(host="0.0.0.0", port=5000, debug=False)
