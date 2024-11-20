**TODO**: 

✓ přidat state machine

wifi
 - připojení
 - ntp
 - jednoduchý web server (bude aktivní pořád?)
 - odpojování od wifi?

logika termostatu
 - rtc alarmy
 - nastavení různé teploty na různé časy
    - struktura pro ukládání
    - hlídání těchto časů přes alarmy
        - jak lze udělat křivky? -> například granuálně po 5 minutách měnit podle grafu křivky(funkce)

menu a ovládání
 - nastavení teplot
 - nastavitelná hysterze temploty 
 - základní nastavení
 - vypnutí/zapnutí webserveru?
 - úplné pozastavení hlídání teploty?

ukládání a statistika teplot

krabička pro termostat
 - tlačítka a otočný úchyt pro potík
 - obrazovka
 - zapnutí/vypnutí termostatu?
 - otvory pro indikační ledky?

kalibrace teploměru (teploměr na pico)

vypnutí potenciometru, když není potřeba

dokumentace <!-- <-this -->

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