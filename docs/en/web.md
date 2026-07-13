[← Back to index](./)

# Web Configuration

The web configuration is divided into several sections for configuring individual features. Each feature can be fully configured and, if desired, disabled. The individual configuration options are described below.

## MENU

* **Backup**

  * **Download:** Downloads the JSON configuration file for backup purposes.
  * **Restore:** Uploads a JSON file and restores the configuration.
* **Device**

  * **WiFi Manager:** Activates WiFi Manager to change the Wi-Fi SSID and password. *(The web configuration interface is unavailable while WiFi Manager is running.)*
  * **Factory Reset:** Restores the factory default settings.
  * **Reboot:** Restarts the WX station.
* **config.json:** Opens the `config.json` file in a new browser tab.
* **Save:** Saves the entire configuration. A restart is normally not required after saving.

## STATION

* **Name:** The station name used only for identification in Syslog and the MQTT PUB/SUB client. This is especially useful if you operate multiple stations.
* **ASL:** The station altitude above sea level in meters. It is used to calculate sea-level pressure correctly.

## DATA

* **Temp, Humi, Press:** The names used for temperature, humidity, and pressure when sending HTTP GET parameters and as JSON keys in MQTT messages.
* **Offset:** Used to calibrate temperature, humidity, and pressure. Both positive and negative values are supported.
* **Light 🔹:** The name used for the light sensor when sending HTTP GET parameters and as the JSON key in MQTT messages.
* **RSSI:** Like the other values, this defines the name used for Wi-Fi signal strength when sending data to the database and MQTT.

🔹 This item can be disabled using the switch next to its name.

## SERVER

The firmware can send data via HTTP GET to up to three different servers and one information server.

* **Server i:** Sends information to the server when the station starts or when requested using the MQTT `info` command. The station name (from the second field), firmware version, local IP address, and public IP address are transmitted.
* **Server 1, Server 2, Server 3:** Server addresses for sending measurement data. The second field is optional and is used as the `station` parameter to identify the station, which is useful when operating multiple stations.

All server addresses must begin with `http://` or `https://`. If no specific file is required, the address must end with a trailing slash `/`.

Examples:

* `http://example.com/wx.php`
* `http://example.com/`

## APRS

Sends weather data to the APRS-IS network. Only temperature, humidity, and pressure are transmitted.

* **Host:** Regional APRS server. You can find the appropriate server for your region at **aprs2.net**.
* **Port:** APRS server port, typically **14580**.
* **Call:** Your callsign including the SSID. For weather stations, **-13** is recommended, for example **OK1KKY-13**.
* **Pass:** The passcode for your APRS callsign. It can be generated at **ok1kky.cz/aprs-generator**.
* **Lat:** Latitude in APRS format, for example `5005.79N` (equivalent to `50°05.79'N`).
* **Lon:** Longitude in APRS format, for example `01551.39E` (equivalent to `15°51.39'E`).
* **Comment:** Any comment you want to include with your APRS station.

## MQTT

Sends all measurement data to an MQTT server in JSON format. This is ideal for real-time processing, such as displaying data on external screens or other hardware.

* **Server:** MQTT server address.
* **Port:** MQTT server port, typically **1883**.
* **Pub Topic 1:** Topic used to publish measurement data in JSON format.
* **Pub Topic 2:** Topic where the response to the MQTT `get()` command is published.
* **Sub Topic:** Topic on which the station listens for plain-text commands. Two topics can be configured—for example, one dedicated to a single station and another for controlling multiple stations simultaneously.

## SYSLOG

All station activity is logged to a Syslog server. It provides functionality similar to Debug mode but does not require the station to be physically connected to a computer.

* **Server:** Syslog server address.
* **Port:** Syslog server port, typically **514**.

## INTERVAL

* **Reboot:** Automatic restart interval. It can be set to **6, 12, 24, or 48 hours**. This helps prevent lockups and ensures long-term reliable operation.
* **Server, APRS, MQTT:** Transmission intervals for the HTTP servers, APRS, and MQTT.

## HEARTBEAT

The heartbeat LED indicates the current operating status of the station. It is an optional but useful feature. More information is available on the **Heartbeat** page.

## DEBUG

Debug mode prints diagnostic messages to the serial interface, making troubleshooting and debugging easier.

Simply connect the station to a computer using a USB cable and open a serial monitor, such as the one in the Arduino IDE, or use the online monitor at **serial.ok1kky.cz**.

The serial baud rate must be set to **115200**.
