/**
 * analog_tmp_sensor.h
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#ifndef ANALOG_TMP_SENSOR_H
#define ANALOG_TMP_SENSOR_H


#include "pico/stdlib.h"

/* return x10 temperature reading in C as integer*/
uint16_t get_temp();

/* converts integer temperature to string */
void convert_temp_to_str(uint16_t temp, char* str);

/* converts integer temperature to float */
float convert_temp_to_float(uint16_t temp);

/* inicializes analog temperature sensor */
int temp_sensor_init();

#endif /* ANALOG_TMP_SENSOR_H */