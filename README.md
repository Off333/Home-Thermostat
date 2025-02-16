

*BUGS*:
 - @bug Tlačítka nemají hlídání na odrazy
 - @bug displej se neuspí správně

**TODO**: 

✓ přidat state machine

✓ logika termostatu
 - X rtc alarmy
 - ✓ nastavení různé teploty na různé časy
    - ✓ struktura pro ukládání
    - X hlídání těchto časů přes alarmy
    - ✓ hlídání časů programů periodicky po X minutách (uspání v hlavní smyčce)
 - ✓ ukládání programů a nastavení do flash paměti 

menu a ovládání
 - ✓ nastavení teplot
 - ✓ nastavitelná hystereze teploty 
 - ✓ základní nastavení
 - X vypnutí/zapnutí webserveru?
 - ✓ úplné pozastavení hlídání teploty?
 - ovládání pomocí potíku

✓ refresh pouze pokud je zmáčknuto tlačítko
 - jediný problém je zobrazení reálného času... tam je asi možno refresh po 1s... jinak by se měl main loop proběhnout jen 1x za dlouhou dobu... třeba 5 min a hlídat teplotu
 - vyřešeno updatem state machine v rámci hlavní smyčky... není dokonalé, ale nemusí být

wifi
 - ✓ připojení
 - ✓ ntp
 - X jednoduchý web server (bude aktivní pořád?)
 - ? nwm jestli deinit odpojí odpojování od wifi?
 - program, který se připojí a zapíše jen několik bytů nemusí být řešen přes webserver
   - komunikační protokol
   - webová aplikace (může být webová stránka s javaskriptem > zařízení z POST požadavku vyjme co potřebuje > zabezpečení?)
  
krabička pro termostat
 - tlačítka a otočný úchyt pro potík
 - obrazovka
 - zapnutí/vypnutí termostatu?
 - X otvory pro indikační ledky?

✓ výpis co se děje při inicializaci zařízení na obrazovku

✓ změna teplotního senzoru z LM35DZ(analog) na DHT22(digital)

výpis vlhkosti (vedle teploty?)

přesunout konstanty do samostatného hlavičkového souboru 

když se něco mění, tak přidat kolem znak, že se ukazuje měněná hodnota (jako u změny časů)

(úplně oddělat potík pryč, pokud ho nebudu používat)
vypnutí/zapnutí potenciometru, když není potřeba
vypnutí/zapnutí teploměru, aby nemusel měřit když nemusí
 - pokud se teplota hlídá co 3 minuty tak není potřeba vědět teplotu každé 2 sekundy

kalibrace teploměru (teploměr na pico?)

ukládání a statistika teplot

změnit všechny více-řádkové komentáře na jednořádkové (něco okolo good practice ve psaní kódu pro hardware) > najít/nainstalovat modul, který hlídá tyto standardy

jak lze udělat křivky? -> například postupně po 5 minutách měnit podle grafu křivky(funkce)

dokumentace 
toto readme

Relevantní odkazy:
 - https://forums.raspberrypi.com/viewtopic.php?t=339289
 - https://github.com/raspberrypi/pico-examples/tree/master/pico_w/wifi/tcp_server
 - https://www.ti.com/lit/ds/symlink/lm35.pdf
 - https://datasheets.raspberrypi.com/picow/pico-w-datasheet.pdf
 - https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
 - https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/ntp_client/picow_ntp_client.c
 - https://github.com/raspberrypi/pico-examples/tree/master/pico_w/wifi/httpd
 - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
 - https://www.instructables.com/Arduino-Thermostat/
 - https://dzone.com/articles/how-to-build-your-own-arduino-thermostat
 - https://moniteurdevices.com/knowledgebase/knowledgebase/what-is-the-difference-between-spst-spdt-and-dpdt/
 - https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/
