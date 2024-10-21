#ifndef RELAY_DRIVER_H
#define RELAY_DRIVER_H

void relay_init();

void relay_set(bool state);

void relay_on();

void relay_off();

bool relay_get_state();

void realy_toggle();

#endif