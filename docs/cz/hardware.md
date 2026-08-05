[← Zpět na rozcestník](./)

# Hardware

## Mikrokontrolér

Tento projekt je primárně vytvářen a vyvíjen pro standardní **ESP32**. Tyto desky vyrábí mnoho výrobců, obecně však doporučuji desku označenou jako **ESP-WROOM-32**. Nově je podporován i **ESP32-C3** (např. ESP32-C3 Super Mini).

## Senzory

| Senzor          | Popis                                 | ESP32              | ESP32-C3         | Poznámka  |
| --------------- | ------------------------------------- | ------------------ | ---------------- | --------- |
| **BME280**      | Teplota, vlhkost a atmosférický tlak  | 21 (SDA), 22 (SCL) | 8 (SDA), 9 (SCL) | Povinný   |
| **BH1750**      | Luxmetr                               | 21 (SDA), 22 (SCL) | 8 (SDA), 9 (SCL) | Volitelný |
| **MS-WH-SP-RG** | Srážkoměr                             | 27                 | Není podporován  | Volitelný |

**MS-WH-SP-RG**

Měřené hodnoty:

- **Srážky za poslední hodinu**: Klouzavý úhrn srážek za posledních 60 minut.
- **Srážky za posledních 24 hodin**: Klouzavý úhrn srážek za posledních 24 hodin.

Poznámky k implementaci:

- Každý překlop se ukládá s časovou značkou do `LittleFS`, takže se po restartu obnoví posledních 24 hodin historie srážek.
- Firmware filtruje chybné pulzy pomocí minimální a maximální délky sepnutí a krátkého ochranného intervalu mezi dvěma platnými pulzy.
- Výchozí kalibrace je `0.2794 mm/tip`, ale lze ji změnit ve webové konfiguraci.

Pro funkčnost srážkoměru na ESP32-C3 je nutné kódu změnit GPIO pin, protože ESP32-C3 má jinou konfiguraci pinů než ESP32. V současné době je podporován pouze ESP32 s GPIO 27.

## Ostatní

| Název         | Popis                       | GPIO  | Poznámka  |
| ------------- | --------------------------- | ----- | --------- |
| **LED dioda** | Slouží pro funkci Heartbeat | 2     | Volitelná |
