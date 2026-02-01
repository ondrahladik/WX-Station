# WX-Station

This project is based on my older [APRS-WX-Station](https://github.com/ondrahladik/APRS-WX-Station) project, which was built on the ESP8266 platform. It is now built on the more powerful ESP32 microcontroller, which allows for more features and better connectivity. The goal of the project was to create a single universal firmware that could be easily adapted to different hardware configurations.

All configuration is done via a web interface, which allows setting up to 44 different parameters. The firmware supports multiple ways of sending data, including HTTP GET, MQTT, and APRS. For HTTP GET, it is possible to send data to up to three different servers. There is also support for logging to a syslog server or to the serial interface. Individual features can be enabled or disabled independently according to the user’s needs.

Not only all features, but also additional details are described in detail on the [wiki](https://github.com/ondrahladik/WX-Station/wiki) page.