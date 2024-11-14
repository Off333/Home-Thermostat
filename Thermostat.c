#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#include "lcd.h"
#include "analog_tmp_sensor.h"
#include "relay_driver.h"
#include "potenciometer.h"
#include "button_driver.h"

/* main loop sleeping after finishing in ms */
#define MAIN_LOOP_REFRESH_TIMER 500


/* temperature limits in °C */
#define TEMPERATURE_SET_MIN 15

#define TEMPERATURE_SET_MAX 30

#define TEMPERATURE_HYSTERSIS 2

#define BTN_1_GPIO 19
#define BTN_2_GPIO 18
#define BTN_3_GPIO 17
#define BTN_4_GPIO 16

#define MENU_LINE LCD_LINE_1
#define STATUS_LINE LCD_LINE_2

typedef enum {
    S_SLEEP,
    S_SHOW_TIME,
    S_SHOW_TEMP,
    S_SHOW_PROGS,
    S_SHOW_SETN,
    S_PROG_SEL,
    S_PROG_TIME_SET,
    S_PROG_WEEKDAY_SET,
    S_PROG_TEMP_SET,
    S_SETN_TIME,
    S_SETN_HYST,
    S_SETN_OFF
} state_t;

typedef enum {
    NS_NONE,
    NS_LEFT,
    NS_RIGHT,
    NS_ENTER,
    NS_ESC
} next_action_t;

#define DEFAULT_STATE S_SHOW_TIME
next_action_t next_action = NS_NONE;

// void lcd_reset() {
//     lcd_display_enable(true);
//     lcd_clear();
//     display_timer = DISPLAY_ON_TIMER+(MAIN_LOOP_REFRESH_TIMER/1000);
// }

void wake_up() {
    //lcd_reset();
    lcd_refresh_on_timer();
}

void show_menu(state_t state) {
    char menu_str[LCD_MAX_CHARS+1] = {0};
    switch(state) {
        case S_SHOW_TIME:
            snprintf(menu_str, LCD_MAX_CHARS, "<nast|cas|prog>");
            break;
        default:
            break;
    }
    lcd_write(menu_str, MENU_LINE);
}

// takhle se používá rtc:
// char datetime_buf[256];
// char *datetime_str = &datetime_buf[0];
// rtc_get_datetime(&t);
// datetime_to_str(datetime_str, sizeof(datetime_buf), &t);

void show_time() {
    static char datetime_buf[LCD_MAX_CHARS+1];
    static char *datetime_str = &datetime_buf[0];
    static datetime_t dt;
    static time_t t;
    
    rtc_get_datetime(&dt);
    datetime_to_time(&dt, &t);
    struct tm* time_struct = localtime(&t);
    strftime(datetime_str, sizeof(datetime_buf), "%H:%M %d.%m.%y", time_struct);
    lcd_write(datetime_str, STATUS_LINE);
    printf("showing time: %s\n", datetime_str);
}

void show_temp_data(uint16_t temp, float temp_treshold) {
    char temp_str[6];
    char write_str[17];

    if(temp >= 0 && temp < 1000) {
        convert_temp_to_str(temp, temp_str);
        sniprintf(write_str, 16, "T. namer: %s", temp_str);
        lcd_write(write_str, LCD_LINE_1);         
    }

    if(temp_treshold >= TEMPERATURE_SET_MIN && temp_treshold <= TEMPERATURE_SET_MAX) {
        sprintf(write_str, "T.  udrz: %2.1f", temp_treshold);
        lcd_write(write_str, LCD_LINE_2);
    }
}

uint16_t get_temp_data() {
    return get_temp();
}

float get_temp_treshold() {
    return TEMPERATURE_SET_MIN + (TEMPERATURE_SET_MAX-TEMPERATURE_SET_MIN)*((100-get_pot_val())/100.0f);
}

/* set temperature will be center of hystersis curve */
void check_relay_state(float current_temp, float set_temp) {    
    if(current_temp > set_temp + TEMPERATURE_HYSTERSIS/2) {
        relay_off();
    } else if(current_temp < set_temp - TEMPERATURE_HYSTERSIS/2) {
        relay_on();
    }
}

// show_temp_data(get_temp_data(), get_temp_treshold());

void main_loop_logic(state_t state) {
    //pokud je zapnutý
    // check_relay_state(convert_temp_to_float(get_temp_data()), get_temp_treshold());
    

    switch(state) {
        case S_SHOW_TIME:
            show_time();
            break;
        default:
            break;
    }
    show_menu(state);
}

state_t main_loop_step() {

    static state_t state = DEFAULT_STATE;

    state_t next_state = state;

    switch(state) {
        case S_SHOW_TIME:
            switch(next_action) {
                case NS_LEFT:
                    break;
                case NS_RIGHT:
                    break;
                case NS_ENTER:
                    break;
                case NS_ESC:
                    break;
                case NS_NONE:
                    break;
            }
            break;
        default:
            switch(next_action) {
                case NS_LEFT:
                    break;
                case NS_RIGHT:
                    break;
                case NS_ENTER:
                    break;
                case NS_ESC:
                    break;
                case NS_NONE:
                    break;
            }
            break;
    }


    if(state == next_state) {
        //timer pro uspání MCU?
    }
    state = next_state;
    return state;
}

int main() {
    
    //busy_wait_ms(2000);
    printf("initialization...\n");

    stdio_init_all();

    if(temp_sensor_init()) {
        printf("temp sensor init failed!");
        return 1;
    }

    if(pot_init()) {
        printf("potenciometer init failed!");
        return 1;
    }

    rtc_init();

    //TODO tohle nahradí načtení času z ntp... možná zde tohle nechám a ntp nastaví čas správně později
    datetime_t t = {
            .year  = 2024,
            .month = 10,
            .day   = 14,
            .dotw  = 1, // 0 is Sunday, so 5 is Friday
            .hour  = 1,
            .min   = 18,
            .sec   = 00
    };

    rtc_set_datetime(&t);
    sleep_us(64); // waiting for rtc to update

    relay_init();

    if(lcd_init()) {
        printf("lcd init failed!");
        return 1;
    }
    lcd_string("Hello, world!");

    btn_set_common_function(&wake_up);
    init_btn_rising_edge(BTN_1_GPIO, NULL);
    init_btn_rising_edge(BTN_2_GPIO, NULL);
    init_btn_rising_edge(BTN_3_GPIO, NULL);
    init_btn_rising_edge(BTN_4_GPIO, NULL);

    //sleep_ms(1000);
    lcd_clear();
    printf("initialization done!");

    state_t state = DEFAULT_STATE;
    while (true) {
        
        state = main_loop_step();

        main_loop_logic(state);

        sleep_ms(MAIN_LOOP_REFRESH_TIMER);

        printf("main loop end, state: %i\n", state);
    }
}


//zobrazování na displeji
// if(display_timer > 0)  {
//     lcd_show_temp_data(temp, temp_treshold);
//     display_timer -= MAIN_LOOP_REFRESH_TIMER/1000;
// }

// if(display_timer == 0) {
//     lcd_display_enable(false);
// }

//printf("main loop end\n");