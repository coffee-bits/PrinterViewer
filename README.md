# PrinterViewer
View printer data on a smal screen on the desktop

# Overview
![PrinterPictureOverlay.drawio.png](PrinterPictureOverlay.drawio.png)

# Server Setup
## Proxmox lxc container

## Python enviroment
~~~
python3 -m venv venv
source venv/bin/activate
pip install prusaLinkPy
pip install requests
pip install gmqtt
~~~

### MQTT client installation test
~~~
import asyncio
from gmqtt import Client as MQTTClient

async def main():
    client = MQTTClient("my_gmqtt_client")
    await client.connect("192.168.xxx.xxx", 1883)
    client.publish("test/topic", b"Testnachricht")
    await client.disconnect()

if __name__ == "__main__":
    asyncio.run(main())
~~~

# ESP32 Setup
## Software
.t.b.d.
## Hardware
.t.b.d.