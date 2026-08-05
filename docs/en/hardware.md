[← Back to index](./)

# Hardware

## Microcontroller

This project is primarily created and developed for the standard ESP32. Many such boards are manufactured, but generally I recommend a board labeled **ESP-WROOM-32**. The **ESP32-C3** (e.g., ESP32-C3 Super Mini) is also supported.

## Sensors

| Sensor          | Description                          | ESP32              | ESP32-C3         | Note      |
| --------------- | ------------------------------------ | ------------------ | ---------------- | --------- |
| **BME280**      | Temperature, humidity, and pressure. | 21 (SDA), 22 (SCL) | 8 (SDA), 9 (SCL) | Required  |
| **BH1750**      | Luxmeter                             | 21 (SDA), 22 (SCL) | 8 (SDA), 9 (SCL) | Optional  |
| **MS-WH-SP-RG** | Rain gauge                           | 27                 | Not supported    | Optional  |

**MS-WH-SP-RG**

Measured values:

- **Rainfall in the last hour**: Rolling rainfall total for the previous 60 minutes.
- **Rainfall in the last 24 hours**: Rolling rainfall total for the previous 24 hours.

Implementation notes:

- Every bucket tip is stored with a timestamp in `LittleFS`, so the last 24 hours of rainfall history survive a reboot.
- The firmware filters invalid pulses using minimum and maximum low-level duration checks plus a short guard interval between accepted tips.
- The default calibration is `0.2794 mm/tip`, but it can be changed in the web configuration.

For the rain gauge to work with the ESP32-C3, the GPIO pin in the code must be changed, as the ESP32-C3 has a different pin configuration than the ESP32. Currently, only the ESP32 with GPIO 27 is supported.

## Other

| Name          | Description                            | GPIO  | Note      |
| ------------- | -------------------------------------- | ----- | --------- |
| **LED diode** | It is used for the heartbeat function. | 2     | Optional  |
