*BUGS*:
 - @bug Tlačítka nemají hlídání na odrazy
 - @bug displej se neuspí správně

**TODO**: 

✓ přidat state machine

logika termostatu
 - rtc alarmy
 - ✓ nastavení různé teploty na různé časy
    - ✓ struktura pro ukládání
    - hlídání těchto časů přes alarmy
        - jak lze udělat křivky? -> například postupně po 5 minutách měnit podle grafu křivky(funkce)

menu a ovládání
 - ✓ nastavení teplot
 - ✓ nastavitelná hystereze teploty 
 - ✓ základní nastavení
 - vypnutí/zapnutí webserveru?
 - ✓ úplné pozastavení hlídání teploty?
 - ovládání pomocí potíku

refresh pouze pokud je zmáčknuto tlačítko
 - jediný problém je zobrazení reálného času... tam je asi možno refresh po 1s... jinak by se měl main loop proběhnout jen 1x za dlouhou dobu... třeba 5 min a hlídat teplotu

wifi
 - ✓ připojení
 - ✓ ntp
 - jednoduchý web server (bude aktivní pořád?)
 - ? nwm jestli deinit odpojí odpojování od wifi?
  
krabička pro termostat
 - tlačítka a otočný úchyt pro potík
 - obrazovka
 - zapnutí/vypnutí termostatu?
 - otvory pro indikační ledky?

✓ výpis co se děje při inicializaci zařízení na obrazovku

když se něco mění, tak přidat kolem znak, že se ukazuje měněná hodnota (jako u změny časů)

vypnutí/zapnutí potenciometru, když není potřeba
vypnutí/zapnutí teploměru, aby nemusel měřit když nemusí
 - pokud se teplota hlídá co 3 minuty tak není potřeba vědět teplotu každé 2 sekundy

kalibrace teploměru (teploměr na pico?)

ukládání a statistika teplot

dokumentace a toto readme <!-- <-this -->

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