# Nahrání firmware

Firmware WX-Station lze do ESP32 nahrát dvěma způsoby:

* **Arduino IDE:** standardní metoda, která vyžaduje instalaci Arduino IDE a všech potřebných knihoven.
* **Webový flasher:** jednodušší možnost, kde je vždy dostupná nejnovější verze firmwaru.

## Arduino IDE

Pro úspěšné sestavení a nahrání firmwaru jsou vyžadovány následující knihovny:

| Knihovna                | Odkaz                                               |
| ----------------------- | --------------------------------------------------- |
| Adafruit Unified Sensor | https://github.com/adafruit/Adafruit_Sensor         |
| Adafruit BME280         | https://github.com/adafruit/Adafruit_BME280_Library |
| WiFiManager             | https://github.com/tzapu/WiFiManager                |
| PubSubClient            | https://github.com/knolleary/pubsubclient           |
| ArduinoJson             | https://github.com/bblanchon/ArduinoJson            |
| BH1750                  | https://github.com/claws/BH1750                     |

## Webový flasher

Veřejně dostupný webový flasher najdete na adrese **https://wxmet.cz/flasher**.

Postup instalace:

1. Připojte ESP32 k počítači pomocí **datového USB kabelu**.
2. Klikněte na tlačítko **CONNECT** a vyberte správný sériový port.
3. Stiskněte a podržte tlačítko **BOOT / FLASH** na desce.
4. Klikněte na tlačítko **INSTALL**.
5. Krátce stiskněte tlačítko **RESET**.

Pokud se instalace nespustí automaticky, zkuste zařízení odpojit a znovu připojit, poté celý postup opakujte.
