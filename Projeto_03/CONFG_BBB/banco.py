import sqlite3

BANCO = "gotham.db"


def inicializar_banco():
    conexao = sqlite3.connect(BANCO)

    cursor = conexao.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS eventos (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            topico TEXT NOT NULL,
            estado TEXT NOT NULL,
            timestamp TEXT NOT NULL
        )
    """)

    conexao.commit()
    conexao.close()

    print("Banco de dados inicializado com sucesso!")


if __name__ == "__main__":
    inicializar_banco()
