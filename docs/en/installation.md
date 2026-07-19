[← Back to index](./)

# Firmware upload

WX-Station can be uploaded to your ESP32 in two ways:

- **Arduino IDE:** the standard method, which requires installing the Arduino IDE itself as well as all required libraries.
- **Web flasher:** a simpler option where the latest firmware version is always available.

## Arduino IDE

To successfully build and upload the firmware, the following libraries are required:

| Library                 | Link                                                |
|-------------------------|-----------------------------------------------------|
| Adafruit Unified Sensor | https://github.com/adafruit/Adafruit_Sensor         |
| Adafruit BME280         | https://github.com/adafruit/Adafruit_BME280_Library |
| WiFiManager             | https://github.com/tzapu/WiFiManager                |
| PubSubClient            | https://github.com/knolleary/pubsubclient           |
| ArduinoJson             | https://github.com/bblanchon/ArduinoJson            |
| BH1750                  | https://github.com/claws/BH1750                     |

## Web flasher

A web-based flasher is publicly available at https://wxmet.cz/flasher.  
Please follow the steps below:

1. Connect your ESP32 to the computer using a data USB cable.
2. Click the **CONNECT** button and select the correct serial port.
3. Press and hold the **BOOT / FLASH** button on the board.
4. Click the **INSTALL** button.
5. Briefly press the **RESET** button.

If the installation does not start automatically, try reconnecting the device and repeat the process.