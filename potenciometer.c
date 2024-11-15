/**
 * potenciometer.c
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "potenciometer.h"
#include "adc_driver.h"

/* 
    GPIO 26 = ADC 0
    GPIO 27 = ADC 1
    GPIO 28 = ADC 2
*/
#define POT_ADC_SENSOR_PIN  27
#define POT_ADC_INPUT       1
static_assert(POT_ADC_SENSOR_PIN - POT_ADC_INPUT == 26);

#define SAMPLE_NUMBERS 5

adc_data pot_data = {
    .cntr = 0,
    .value = (uint16_t)0,
    .avg = UINT16_MAX,
    .adc_input = POT_ADC_INPUT,
    .sample_number = 5
};

uint16_t get_pot_val() {
    const float conversion_factor = (3300.0f) / (1 << 12);
    return (uint16_t)((pot_data.avg*conversion_factor)*100/3300.0f);
}

int pot_init() {
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(POT_ADC_SENSOR_PIN);

    // WHEN USING MULTIPLE ADC INPUT THIS MUST BE MOVED TO CALLBACK (and in callback temporarly disabled irq before selecting adc input)!!!
    //adc_select_input(ADC_INPUT);
    return adc_driver_init(&pot_data);
}