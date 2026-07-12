## Mikrokontrolér

Tento projekt je primárně vytvářen a vyvíjen pro standardní **ESP32**. Tyto desky vyrábí mnoho výrobců, obecně však doporučuji desku označenou jako **ESP-WROOM-32**.

## Senzory

| Senzor          | Popis                                                                               | Lze vypnout | GPIO               | Poznámka  |
| --------------- | ----------------------------------------------------------------------------------- | ----------- | ------------------ | --------- |
| **BME280**      | Měří teplotu, vlhkost a atmosférický tlak. Je nezbytný pro provoz stanice.          | Ne          | 21 (SDA), 22 (SCL) | Povinný   |
| **BH1750**      | Měří intenzitu osvětlení v luxech (lx), která je ve firmwaru přepočítávána na W/m². | Ano         | 21 (SDA), 22 (SCL) | Volitelný |
| **MS-WH-SP-RG** | Modul srážkoměru.                                                                   | —           | —                  | Již brzy  |

## Ostatní

| Název         | Popis                                                 | Lze vypnout | GPIO | Poznámka  |
| ------------- | ----------------------------------------------------- | ----------- | ---- | --------- |
| **LED dioda** | Slouží pro funkci Heartbeat (indikaci stavu stanice). | Ano         | 2    | Volitelná |
