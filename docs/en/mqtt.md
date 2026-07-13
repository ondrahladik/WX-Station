## MQTT Data Format

Measured data are published to the MQTT topic defined in the web configuration.  

The payload is sent in **JSON format**, for example:

```json
{
  "temperature": 11.13,
  "humidity": 41.63,
  "pressure": 1023.17,
  "light": 320.35,
  "rssi": -68
}
```

---

## Commands

Commands are sent as **plain text** to the MQTT topic defined in the web configuration.  

They are used for basic station control.

### Available Commands

- **`reboot`** 
  Restarts the WX station.

- **`start-ap`** 
  Activates WiFi Manager to change the Wi-Fi SSID and password.  
  *(The configuration web interface is not available while WiFi Manager is running.)*

- **`info`** 
  Sends the program version, local IP address, and public IP address to the database on the info server.

- **`update()`** 
  Performs an HTTP OTA firmware update.  

  Insert the firmware URL inside the parentheses, for example:

  ```
  update(http://example.com/firmware.bin)
  ```

- **`get()`** 
  Requests a specific configuration value or the entire configuration.  

  The response is published to the **Pub Sub 2 topic**.

  To request a single value, put the JSON key name inside the parentheses:

  ```
  get(serverUrl1)
  ```

  To request the entire configuration, use:

  ```
  get(config)
  ```

- **`set()`** 
  Updates a specific configuration value or replaces the entire JSON configuration at once.  

  After a successful update, a confirmation message is published to the **Pub Sub 2 topic**.

  To set a single value, specify the JSON key and the new value:

  ```
  set(serverUrl1=http://new-url.com/)
  ```

  To update the entire configuration, use `config=` followed by the full JSON configuration:

  ```
  set(config={
    "debugMode": false,
    "activeAPRS": true,
    "activeMQTT": true
  })
  ```

  The JSON does **not** need to be on a single line, formatting it with one value per line is perfectly acceptable.
