
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#include "adc_driver.h"



bool adc_initialized = false;

uint16_t my_adc_read(uint input) {
    uint32_t state = save_and_disable_interrupts();
    adc_select_input(input);
    uint16_t result = adc_read();
    restore_interrupts(state);
    return result;
}

bool reading_callback(repeating_timer_t *rt) {
    adc_data* data = (adc_data*)rt->user_data;
    if(data->cntr < data->sample_number) {
        uint16_t val = my_adc_read(data->adc_input);
        data->value += val;
        data->cntr++;
    } else {
        data->avg = (data->value)/(data->sample_number);
        data->value = 0;
        data->cntr = 0;
    }
    return true;
}

int adc_driver_init(adc_data *data) {
    if(!adc_initialized) adc_init();
    adc_initialized = true;

    //setup sensor constant sensor reading
    repeating_timer_t *timer = malloc(sizeof(repeating_timer_t));
    if(timer == NULL) return 1;

    if(!add_repeating_timer_ms(-READING_INTERVAL, *reading_callback, data, timer)) {
        printf("failed to add repeating timer - adc\n");
        return 1;
    }
    return 0;
}