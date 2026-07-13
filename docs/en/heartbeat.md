[← Back to index](./)

# Heartbeat

Heartbeat serves as a simple visual indication of the station's status via an LED connected to an ESP32 GPIO output.

## LED Statuses

- **Off**: The station is not running, or the function is disabled.
- **On**: The access point (AP) is active.
- **Double flash**: When the program starts up normally and everything works correctly.
- **Fast flashing**: Critical error or loss of communication with the sensor.

## Sensor error behavior

- If a sensor failure occurs, the program enters an error state.
- Standard data transmission stops, and the program attempts to re-establish the sensor connection.
- Once the sensor becomes available again, the station automatically returns to normal operation.

## Important

- Disabling the heartbeat only turns off the LED indication.
- Program protection in the event of a sensor fault remains functional at all times.