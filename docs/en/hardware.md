[← Back to index](./)

# Hardware

## Microcontroller

This project is primarily created and developed for the standard ESP32. Many such boards are manufactured, but generally I recommend a board labeled **ESP-WROOM-32**.

## Sensors

| Sensor          | Description                                                                    | Disable | GPIO               | Notes          |
| --------------- | ------------------------------------------------------------------------------ | ------- | ------------------ | -------------- |
| **BME280**      | Measures temperature, humidity, and pressure. Essential for station operation. | No      | 21 (SDA), 22 (SCL) | Required       |
| **BH1750**      | Measures illuminance in lux (lx), converted to W/m² in the firmware.           | Yes     | 21 (SDA), 22 (SCL) | Optional       |
| **MS-WH-SP-RG** | Tipping-bucket rain gauge for rainfall totals.                                 | Yes     | 27                 | Optional       |

**MS-WH-SP-RG**

Measured values:

- **Rainfall in the last hour**: Rolling rainfall total for the previous 60 minutes.
- **Rainfall in the last 24 hours**: Rolling rainfall total for the previous 24 hours.

Implementation notes:

- Every bucket tip is stored with a timestamp in `LittleFS`, so the last 24 hours of rainfall history survive a reboot.
- The firmware filters invalid pulses using minimum and maximum low-level duration checks plus a short guard interval between accepted tips.
- The default calibration is `0.2794 mm/tip`, but it can be changed in the web configuration.

## Other

| Name            | Description                                                                    | Disable | GPIO      | Notes          |
| --------------- | ------------------------------------------------------------------------------ | ------- | --------- | -------------- |
| **LED diode**   | It is used for the heartbeat function.                                         | Yes     | 2         | Optional       |
