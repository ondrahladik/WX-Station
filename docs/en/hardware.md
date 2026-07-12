# Hardware

## Microcontroller

This project is primarily created and developed for the standard ESP32. Many such boards are manufactured, but generally I recommend a board labeled **ESP-WROOM-32**.

## Sensors

| Sensor          | Description                                                                    | Disable | GPIO               | Notes          |
| --------------- | ------------------------------------------------------------------------------ | ------- | ------------------ | -------------- |
| **BME280**      | Measures temperature, humidity, and pressure. Essential for station operation. | No      | 21 (SDA), 22 (SCL) | Required       |
| **BH1750**      | Measures illuminance in lux (lx), converted to W/m² in the firmware.           | Yes     | 21 (SDA), 22 (SCL) | Optional       |
| **MS-WH-SP-RG** | Rain sensor module.                                                            | —       | —                  | Coming soon    |

## Other

| Name            | Description                                                                    | Disable | GPIO      | Notes          |
| --------------- | ------------------------------------------------------------------------------ | ------- | --------- | -------------- |
| **LED diode**   | It is used for the heartbeat function.                                         | Yes     | 2         | Optional       |
