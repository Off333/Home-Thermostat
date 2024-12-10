/**
 * relay_driver.c
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#include "hardware/gpio.h"

#include "relay_driver.h"

void relay_init() {
    gpio_init(RELAY_PIN);

    gpio_set_dir(RELAY_PIN, GPIO_OUT);

    gpio_put(RELAY_PIN, 1);
}

void relay_set(bool state) {
    if(state)   relay_on();
    else        relay_off();
}

void relay_on() {
    gpio_put(RELAY_PIN, 0);
}

void relay_off() {
    gpio_put(RELAY_PIN, 1);
}

bool relay_get_state() {
    return gpio_get(RELAY_PIN);
}

void realy_toggle() {
    gpio_put(RELAY_PIN, !gpio_get(RELAY_PIN));
}