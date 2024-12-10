/**
 * relay_driver.h
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#ifndef RELAY_DRIVER_H
#define RELAY_DRIVER_H

#define RELAY_PIN 0

void relay_init();

void relay_set(bool state);

void relay_on();

void relay_off();

bool relay_get_state();

void realy_toggle();

#endif