/**
 * button_driver.c
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#include "button_driver.h"

#include "pico/stdlib.h"   
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include <stdio.h>
#include "pico/stdlib.h"


btn_callback_t common_callback = NULL;

btn_callback_t gpio_callback_map[30] = {NULL};


void gpio_callback(uint gpio, uint32_t events) {
    gpio_set_irq_enabled(gpio, events, false);
    static bool triggered = false;

    if(!triggered) {
        triggered = true;
        //this can take a while
        //calling common function for all buttons
        if(common_callback != NULL) (*common_callback)();
        //calling specific function for button
        if(gpio >= 0 && gpio < 30 && gpio_callback_map[gpio] != NULL) (*(gpio_callback_map[gpio]))();

        triggered = false;
    }
    
    gpio_set_irq_enabled(gpio, events, true);
}

void btn_set_common_function(btn_callback_t callback) {
    common_callback = callback;
}

void init_btn(uint gpio, uint32_t event_mask, btn_callback_t callback) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);

    if(gpio >= 0 && gpio < 30) gpio_callback_map[gpio] = callback;
    gpio_set_irq_enabled_with_callback(gpio, event_mask, true, &gpio_callback);
}

void init_btn_rising_edge(uint gpio, btn_callback_t btn_callback) {
    init_btn(gpio, GPIO_IRQ_EDGE_RISE, btn_callback);
    gpio_pull_up(gpio);
}

// busy_wait_ms(50);
// Put the GPIO event(s) that just happened into event_str
// so we can print it
// static unsigned long last_irq_t = 0;
// unsigned long irq_t = to_ms_since_boot(get_absolute_time());
// if(irq_t - last_irq_t > 80) {
// last_irq_t = irq_t; // <---
// lcd_reset();
// busy_wait_us(80); //možná není potřeba protože předhoczí funkce jsou pomalé?
// lcd_show_data(get_temp_data(), get_temp_treshold());
// gpio_acknowledge_irq(gpio, events & (GPIO_IRQ_EDGE_RISE));
// }
