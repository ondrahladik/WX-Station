[← Zpět na rozcestník](./)

# Hardware

## Mikrokontrolér

Tento projekt je primárně vytvářen a vyvíjen pro standardní **ESP32**. Tyto desky vyrábí mnoho výrobců, obecně však doporučuji desku označenou jako **ESP-WROOM-32**.

## Senzory

| Senzor          | Popis                                                                               | Lze vypnout | GPIO               | Poznámka  |
| --------------- | ----------------------------------------------------------------------------------- | ----------- | ------------------ | --------- |
| **BME280**      | Měří teplotu, vlhkost a atmosférický tlak. Je nezbytný pro provoz stanice.          | Ne          | 21 (SDA), 22 (SCL) | Povinný   |
| **BH1750**      | Měří intenzitu osvětlení v luxech (lx), která je ve firmwaru přepočítávána na W/m². | Ano         | 21 (SDA), 22 (SCL) | Volitelný |
| **MS-WH-SP-RG** | Překlápěcí srážkoměr pro měření úhrnu srážek.                                       | Ano         | 27                 | Volitelný |

**MS-WH-SP-RG**

Měřené hodnoty:

- **Srážky za poslední hodinu**: Klouzavý úhrn srážek za posledních 60 minut.
- **Srážky za posledních 24 hodin**: Klouzavý úhrn srážek za posledních 24 hodin.

Poznámky k implementaci:

- Každý překlop se ukládá s časovou značkou do `LittleFS`, takže se po restartu obnoví posledních 24 hodin historie srážek.
- Firmware filtruje chybné pulzy pomocí minimální a maximální délky sepnutí a krátkého ochranného intervalu mezi dvěma platnými pulzy.
- Výchozí kalibrace je `0.2794 mm/tip`, ale lze ji změnit ve webové konfiguraci.

## Ostatní

| Název         | Popis                                                 | Lze vypnout | GPIO | Poznámka  |
| ------------- | ----------------------------------------------------- | ----------- | ---- | --------- |
| **LED dioda** | Slouží pro funkci Heartbeat (indikaci stavu stanice). | Ano         | 2    | Volitelná |
