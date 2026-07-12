# Heartbeat

Heartbeat slouží jako jednoduchá vizuální indikace stavu stanice pomocí LED připojené na GPIO výstup ESP32.

## Stavy LED

* **LED zhasnutá:** Heartbeat je vypnutý.
* **LED svítí:** Je aktivní režim přístupového bodu (AP).
* **LED pomalu bliká:** Stanice běží normálně a vše funguje správně.
* **LED rychle bliká:** Došlo ke kritické chybě nebo ke ztrátě komunikace se senzorem.

## Chování při chybě senzoru

* Pokud dojde k chybě senzoru, program přejde do chybového stavu.
* Běžné odesílání dat se pozastaví a program se pokouší obnovit komunikaci se senzorem.
* Jakmile je komunikace se senzorem opět navázána, stanice se automaticky vrátí do běžného provozu.

## Důležité

* Vypnutí heartbeatu pouze deaktivuje LED indikaci stavu.
* Ochrana programu při chybě senzoru zůstává vždy aktivní.
