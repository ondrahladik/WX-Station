[← Zpět na rozcestník](./)

## Formát MQTT dat

Naměřená data jsou publikována do MQTT topicu definovaného ve webové konfiguraci.

Payload je odesílán ve **formátu JSON**, například:

```json
{
  "temperature": 11.13,
  "humidity": 41.63,
  "pressure": 1023.17,
  "light": 320.35,
  "rain_1h": 0.28,
  "rain_24h": 1.12,
  "rssi": -68
}
```

Klíče `light`, `rain_1h` a `rain_24h` se odesílají pouze tehdy, když je ve webové konfiguraci aktivní příslušné čidlo.

---

## Příkazy

Příkazy se odesílají jako **prostý text** do MQTT topicu definovaného ve webové konfiguraci.

Slouží pro základní ovládání stanice.

### Dostupné příkazy

* **`reboot`**
  Restartuje stanici WX.

* **`start-ap`**
  Aktivuje WiFi Manager pro změnu názvu Wi-Fi sítě (SSID) a hesla.
  *(Po dobu běhu WiFi Manageru není dostupné konfigurační webové rozhraní.)*

* **`info`**
  Odešle verzi programu, lokální IP adresu a veřejnou IP adresu do databáze na informačním serveru.

* **`update()`**
  Provede HTTP OTA aktualizaci firmwaru.

  Do závorek vložte URL adresu firmwaru, například:

  ```
  update(http://example.com/firmware.bin)
  ```

* **`get()`**
  Vyžádá konkrétní konfigurační hodnotu nebo celou konfiguraci.

  Odpověď je publikována do **MQTT topicu Pub Sub 2**.

  Pro získání jedné hodnoty vložte do závorek název JSON klíče:

  ```
  get(serverUrl1)
  ```

  Pro získání celé konfigurace použijte:

  ```
  get(config)
  ```

* **`set()`**
  Aktualizuje jednu konfigurační hodnotu nebo nahradí celou konfiguraci JSON najednou.

  Po úspěšné změně je potvrzovací zpráva publikována do **MQTT topicu Pub Sub 2**.

  Pro změnu jedné hodnoty zadejte název JSON klíče a novou hodnotu:

  ```
  set(serverUrl1=http://new-url.com/)
  ```

  Pro aktualizaci celé konfigurace použijte `config=` následované kompletní JSON konfigurací:

  ```text
  set(config={
    "debugMode": false,
    "activeAPRS": true,
    "activeMQTT": true
  })
  ```

  JSON **nemusí být na jednom řádku** – jeho přehledné formátování s jednou hodnotou na řádek je zcela v pořádku.
