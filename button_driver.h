/**
 * button_driver.h
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include "pico/stdlib.h"   
#include "hardware/gpio.h"
#include "hardware/irq.h"

typedef void(*btn_callback_t)(void);

/**
 * 
 */
void btn_set_common_function(btn_callback_t callback);

/**
 * 
 */
void init_btn_rising_edge(uint gpio, btn_callback_t btn_callback);

#endif /* BUTTON_DRIVER_H */