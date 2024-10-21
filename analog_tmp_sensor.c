
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#include "analog_tmp_sensor.h"
#include "adc_driver.h"

/* 
    GPIO 26 = ADC 0
    GPIO 27 = ADC 1
    GPIO 28 = ADC 2
*/
#define TEMP_ADC_SENSOR_PIN  26
#define TEMP_ADC_INPUT       0
static_assert(TEMP_ADC_SENSOR_PIN - TEMP_ADC_INPUT == 26);



/* TEMPERATURE in 1/10 Celsius */
#define CALIBRATION_OFFSET -15
//#define CALIBRATION_OFFSET 0

adc_data temp_data = {
    .cntr = 0,
    .value = (uint16_t)0,
    .avg = UINT16_MAX,
    .adc_input = TEMP_ADC_INPUT,
    .sample_number = 10
};

void convert_temp_to_str(uint16_t temp, char* str) {
    snprintf(str, 5, "%3.1f", ((float)temp)/10);
}

float convert_temp_to_float(uint16_t temp) {
    return ((float)temp)/10;
}

uint16_t calculate_temp(uint16_t temp_sensor) {
    const float conversion_factor = (3300.0f) / (1 << 12);
    float temp_cels = (temp_sensor*conversion_factor)+(CALIBRATION_OFFSET);
    return (uint16_t)temp_cels;
}

uint16_t get_temp() {
    return calculate_temp(temp_data.avg);
}

int temp_sensor_init() {

    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(TEMP_ADC_SENSOR_PIN);

    // WHEN USING MULTIPLE ADC INPUT THIS MUST BE MOVED TO CALLBACK (and in callback temporarly disabled irq before selecting adc input)!!!
    //adc_select_input(ADC_INPUT);
    return adc_driver_init(&temp_data);
}
