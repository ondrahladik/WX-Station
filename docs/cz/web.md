[← Zpět na rozcestník](./)

# Webová konfigurace

Webová konfigurace je rozdělena do několika sekcí pro nastavení jednotlivých funkcí. Každou funkci lze plně nakonfigurovat a případně také deaktivovat. Popis jednotlivých nastavení naleznete níže.

## MENU

* **Záloha**

  * **Stáhnout:** Stáhne konfigurační soubor JSON pro zálohu nastavení.
  * **Obnovit:** Slouží k nahrání souboru JSON a obnovení konfigurace.
* **Zařízení**

  * **WiFi Manager:** Aktivuje WiFi Manager pro změnu názvu Wi-Fi sítě (SSID) a hesla. *(Po dobu běhu WiFi Manageru není konfigurační webové rozhraní dostupné.)*
  * **Obnovit tovární nastavení:** Obnoví výchozí nastavení zařízení.
  * **Restart:** Restartuje stanici WX.
* **config.json:** Otevře soubor `config.json` v novém okně.
* **Uložit:** Uloží veškerou konfiguraci. Po uložení není nutné stanici restartovat.

## STANICE

* **Název:** Název stanice používaný pouze pro identifikaci v Syslogu a MQTT PUB/SUB klientovi. Je užitečný zejména při provozu více stanic.
* **Nadmořská výška (ASL):** Nadmořská výška stanice v metrech. Používá se pro správný přepočet atmosférického tlaku na tlak přepočtený na hladinu moře.

## DATA

* **Temp, Humi, Press:** Název teploty, vlhkosti a tlaku používaný při odesílání HTTP GET parametrů i jako JSON klíč při odesílání dat přes MQTT.
* **Offset:** Slouží ke korekci teploty, vlhkosti a tlaku. Lze zadat kladné i záporné hodnoty.
* **Light 🔹:** Název světelného senzoru používaný při odesílání HTTP GET parametrů i jako JSON klíč v MQTT.
* **RSSI:** Stejně jako u ostatních hodnot slouží k nastavení názvu síly Wi-Fi signálu při odesílání dat do databáze a MQTT.

🔹 Tuto položku lze deaktivovat pomocí přepínače vedle názvu.

## SERVER

Program může odesílat data pomocí HTTP GET až na tři různé servery a jeden informační server.

* **Server i:** Odesílání informačních údajů na server při spuštění stanice nebo na vyžádání pomocí MQTT příkazu `info`. Odesílá se název stanice z druhého pole, verze programu, lokální IP adresa a veřejná IP adresa.
* **Server 1, Server 2, Server 3:** Adresa serveru pro odesílání měřených dat. Druhé pole je nepovinné a slouží k identifikaci stanice (parametr `station`), což je užitečné při provozu více stanic.

Všechny adresy serverů musí začínat na `http://` nebo `https://`. Pokud není potřeba zadávat konkrétní soubor, musí adresa končit lomítkem `/`.

Příklad:

* `http://example.com/wx.php`
* `http://example.com/`

## APRS

Odesílání dat do sítě APRS-IS. Do APRS se odesílá pouze teplota, vlhkost a tlak.

* **Host:** Regionální APRS server. Adresu pro svůj region naleznete na **aprs2.net**.
* **Port:** Port APRS serveru, obvykle **14580**.
* **Call:** Vaše volací značka včetně SSID. Pro meteorologické stanice se doporučuje SSID **-13**, například **OK1KKY-13**.
* **Pass:** Passcode k vaší APRS volací značce. Lze jej vygenerovat na **ok1kky.cz/aprs-generator**.
* **Lat:** Zeměpisná šířka ve formátu `5005.79N` (odpovídá `50°05.79'N`).
* **Lon:** Zeměpisná délka ve formátu `01551.39E` (odpovídá `15°51.39'E`).
* **Comment:** Libovolný komentář k APRS stanici.

## MQTT

Odesílání všech dat na MQTT server ve formátu JSON. Ideální pro zpracování dat v reálném čase, například pro externí displeje nebo další hardware.

* **Server:** Adresa MQTT serveru.
* **Port:** Port MQTT serveru, obvykle **1883**.
* **Pub topic 1:** Topic pro odesílání naměřených dat ve formátu JSON.
* **Pub topic 2:** Topic, do kterého bude odeslána hodnota konfigurace vyžádaná pomocí MQTT příkazu `get()`.
* **Sub topic:** Topic, na kterém stanice přijímá textové příkazy. Lze nastavit dva topicy – jeden například pro ovládání konkrétní stanice a druhý pro hromadné ovládání více stanic.

## SYSLOG

Veškerá činnost stanice je zaznamenávána na Syslog server. Funguje podobně jako debug režim, ale není nutné mít stanici připojenou k počítači.

* **Server:** Adresa Syslog serveru.
* **Port:** Port Syslog serveru, obvykle **514**.

## INTERVAL

* **Restart:** Interval automatického restartu stanice. Lze nastavit **6, 12, 24 nebo 48 hodin**. Funkce pomáhá předcházet zamrznutí programu a zajišťuje dlouhodobě spolehlivý provoz.
* **Server, APRS, MQTT:** Intervaly odesílání dat na databázové servery, APRS a MQTT server.

## HEARTBEAT

Heartbeat využívá LED diodu pro indikaci aktuálního stavu stanice. Jedná se o volitelnou, ale užitečnou funkci. Podrobnější informace naleznete na stránce **Heartbeat**.

## DEBUG

Debug režim vypisuje diagnostické zprávy na sériové rozhraní, což usnadňuje ladění programu a odhalování chyb.

Pro zobrazení výpisu stačí připojit stanici pomocí USB kabelu a otevřít sériový monitor například v Arduino IDE nebo využít online monitor na **serial.ok1kky.cz**.

Komunikační rychlost (baud rate) musí být nastavena na **115200**.
