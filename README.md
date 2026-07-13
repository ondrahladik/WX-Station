# WX-Station

## 🇨🇿

Tento projekt vychází z mého staršího projektu [APRS-WX-Station](https://github.com/ondrahladik/APRS-WX-Station), který byl postaven na platformě ESP8266. Nyní využívá výkonnější mikrokontrolér ESP32, což umožňuje implementaci dalších funkcí a zajišťuje lepší konektivitu. Cílem projektu bylo vytvořit jediný univerzální firmware, který lze snadno přizpůsobit různým hardwarovým konfiguracím.

Veškerá konfigurace probíhá prostřednictvím webového rozhraní, které umožňuje nastavit až 46 různých parametrů. Firmware podporuje více způsobů odesílání dat, včetně HTTP GET, MQTT a APRS. V případě HTTP GET lze data odesílat až na tři různé servery. K dispozici je rovněž podpora logování na syslog server nebo do sériového rozhraní. Jednotlivé funkce lze nezávisle zapínat či vypínat podle potřeb uživatele.

Pro detailní popis všech funkcí a dalších podrobností navštivte [dokumentaci](./docs/cz) projektu.

## 🇪🇳

This project is based on my older [APRS-WX-Station](https://github.com/ondrahladik/APRS-WX-Station) project, which was built on the ESP8266 platform. It is now built on the more powerful ESP32 microcontroller, which allows for more features and better connectivity. The goal of the project was to create a single universal firmware that could be easily adapted to different hardware configurations.

All configuration is done via a web interface, which allows setting up to 46 different parameters. The firmware supports multiple ways of sending data, including HTTP GET, MQTT, and APRS. For HTTP GET, it is possible to send data to up to three different servers. There is also support for logging to a syslog server or to the serial interface. Individual features can be enabled or disabled independently according to the user’s needs.

For a detailed description of all features and other details, please visit the [documentation](./docs/en) of the project.