import asyncio
from gmqtt import Client as MQTTClient
import PrusaLinkPy
import time
import random
import requests
import signal
import sys
import logging

# Logging konfigurieren
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Druckerinstanz
prusaMk4 = PrusaLinkPy.PrusaLinkPy("192.168.xxx.xxx", "xxxxxxxxxxxxxx")

# MQTT-Konfiguration
broker = '192.168.xxx.xxx'
port = 1883
topic = "python/mqtt"
client_id = f'prusa-mqtt-{random.randint(0, 1000)}'

# MQTT-Client erstellen
client = MQTTClient(client_id)

async def main():
    try:
        await client.connect(broker, port)
        logging.info(f"Verbunden mit MQTT-Broker {broker}:{port}")
        await timer()  # Starte die Timer-Funktion
    except Exception as e:
        logging.error(f"Fehler beim Verbinden mit MQTT-Broker: {e}")

async def timer():
    while True:
        try:
            getPrint = prusaMk4.get_printer()
            temp_bed = getPrint.json()["telemetry"]["temp-bed"]
            logging.info(f"Betttemperatur: {temp_bed}")
            msg = f"messages: {temp_bed}"
            client.publish(topic, msg.encode())  # Nachricht als Bytes senden
            logging.info("MQTT-Nachricht gesendet")
        except requests.exceptions.ConnectionError as e:
            logging.error(f"Verbindungsfehler zum Drucker: {e}")
        except KeyError as e:
            logging.error(f"Fehler beim Zugriff auf JSON Daten: {e}")
        except Exception as e:
            logging.error(f"Ein unerwarteter Fehler ist aufgetreten: {e}")
        await asyncio.sleep(3)  # Asynchrones Warten

async def stop_script(signal, frame):
    logging.info("Skript wird beendet...")
    await client.disconnect()
    logging.info("MQTT-Verbindung getrennt")
    sys.exit(0)

if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.add_signal_handler(signal.SIGINT, lambda: asyncio.create_task(stop_script(signal.SIGINT, None)))
    loop.create_task(main())
    loop.run_forever()