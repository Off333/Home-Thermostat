/**
 * digital_tmp_sensor.h
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#ifndef DIGITAL_TMP_SENSOR_H
#define DIGITAL_TMP_SENSOR_H

#include "pico/stdlib.h"

/* return x10 temperature reading in C as integer*/
uint16_t get_temp();

/* converts integer temperature to string */
//void convert_temp_to_str(uint16_t temp, char* str);
/* converts 10x temperature in C to string 
format: %3.1f */
//@todo předělat na inline funkce
#define convert_temp_to_str(temp, str) temp ? snprintf(str, 5, "%3.1f", ((float)temp)/10) : snprintf(str, 5, "%3.1f", 0)

/* converts integer temperature in x10 C to float in C*/
#define convert_temp_to_float(temp) (((float)temp)/10)

/* initializes analog temperature sensor */
int temp_sensor_init();

#endif /* DIGITAL_TMP_SENSOR_H */