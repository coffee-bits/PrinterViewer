import asyncio  # Import the asyncio library for asynchronous programming
from gmqtt import Client as MQTTClient  # Import the MQTT client from gmqtt
import PrusaLinkPy  # Import the PrusaLinkPy library for interacting with Prusa printers
import random  # Import the random library for generating random numbers (used for client ID)
import requests  # Import the requests library for making HTTP requests (used by PrusaLinkPy)
import signal  # Import the signal library for handling system signals (like Ctrl+C)
import sys  # Import the sys library for system-specific parameters and functions
import logging  # Import the logging library for logging messages
import json  # Import the json library for working with JSON files

# Constants (PEP 8)
CONFIG_FILE = "config.json"  # Name of the configuration file

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Load configuration from JSON file
try:
    with open(CONFIG_FILE, "r") as f:
        config = json.load(f)
except FileNotFoundError:
    logging.error(f"Configuration file '{CONFIG_FILE}' not found.")
    sys.exit(1)  # Exit if config file is missing
except json.JSONDecodeError:
    logging.error(f"Error decoding JSON from '{CONFIG_FILE}'.")
    sys.exit(1)  # Exit if JSON is invalid

# Extract configuration parameters (PEP 8)
prusa_ip = config["prusa_ip"]  # IP address of the Prusa printer
prusa_api_key = config["prusa_api_key"]  # API key for the Prusa printer
mqtt_broker = config["mqtt_broker"]  # IP address of the MQTT broker
mqtt_port = config["mqtt_port"]  # Port of the MQTT broker
mqtt_topic = config["mqtt_topic"]  # MQTT topic to publish to

# Instantiate Prusa printer object (PEP 8)
prusa_mk4 = PrusaLinkPy.PrusaLinkPy(prusa_ip, prusa_api_key)

# Create MQTT client (PEP 8)
client = MQTTClient(f'prusa-mqtt-{random.randint(0, 1000)}')  # Generate a random client ID

async def main():
    """Main function to connect to MQTT and start the timer."""
    try:
        await client.connect(mqtt_broker, mqtt_port)  # Connect to the MQTT broker
        logging.info(f"Connected to MQTT broker {mqtt_broker}:{mqtt_port}")
        await timer()  # Start the timer function
    except Exception as e:
        logging.error(f"Error connecting to MQTT broker: {e}")

async def timer():
    """Timer function to periodically fetch printer data and publish to MQTT."""
    while True:
        try:
            get_print = prusa_mk4.get_status()  # Get printer data
            logging.info(get_print.json())  # Log the JSON data
            
            # Extract bed temperature
            temp_bed = get_print.json()["printer"]["temp_bed"]
            logging.info(f"Bed temperature: {temp_bed}")
            msg = f"{temp_bed}"  # Create MQTT message
            client.publish(mqtt_topic+"/bed", msg.encode())  # Publish message to MQTT (encode as bytes)
            
            # Extract nozzle temperature
            temp_nozzle = get_print.json()["printer"]["temp_nozzle"]
            logging.info(f"Nozzle temperature: {temp_nozzle}")
            msg = f"{temp_nozzle}"  # Create MQTT message
            client.publish(mqtt_topic+"/tool", msg.encode())  # Publish message to MQTT (encode as bytes)
            
            # Extract printer state
            state = get_print.json()["printer"]["state"]
            logging.info(f"Printer state: {state}")
            msg = f"{state}"  # Create MQTT message
            client.publish(mqtt_topic+"/state", msg.encode())  # Publish message to MQTT (encode as bytes)
            
            # Extract job progress, set to 0 if 'job' key is not available
            progress = get_print.json().get("job", {}).get("progress", 0)
            logging.info(f"Progress: {progress}")
            msg = f"{progress}"  # Create MQTT message
            client.publish(mqtt_topic+"/printing", msg.encode())  # Publish message to MQTT (encode as bytes)

            logging.info("MQTT message sent")
        except requests.exceptions.ConnectionError as e:
            logging.error(f"Printer connection error: {e}")
        except KeyError as e:
            logging.error(f"Error accessing JSON data: {e}")
        except Exception as e:
            logging.error(f"An unexpected error occurred: {e}")
        await asyncio.sleep(3)  # Asynchronous sleep for 3 seconds

async def stop_script(signal, frame):
    """Function to gracefully stop the script and disconnect from MQTT."""
    logging.info("Stopping script...")
    await client.disconnect()  # Disconnect from the MQTT broker
    logging.info("MQTT connection disconnected")
    sys.exit(0)  # Exit the script

if __name__ == "__main__":
    loop = asyncio.get_event_loop()  # Get the asyncio event loop
    loop.add_signal_handler(signal.SIGINT, lambda: asyncio.create_task(stop_script(signal.SIGINT, None)))  # Add signal handler for Ctrl+C
    loop.create_task(main())  # Create a task for the main function
    loop.run_forever()  # Run the event loop