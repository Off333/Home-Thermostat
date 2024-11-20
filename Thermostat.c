/**
 * Thermostat.c
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 *
*/
/* STANDARD C LIBRARIES */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* PICO LIBRARIES */
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

/* DEBUG PRINTING FUNCTION */
#define DEBUG 1
#include "debug.h"

/* MY MODULES */
#include "lcd.h"
#include "analog_tmp_sensor.h"
#include "relay_driver.h"
#include "potenciometer.h"
#include "button_driver.h"

/* main loop sleep interval in miliseconds from end of loop to start of next loop */
#define MAIN_LOOP_REFRESH_TIMER 200

/* timeout in seconds, after which the device will go to sleep */
#define SLEEP_TIMEOUT 30

/* temperature limits in °C */
#define TEMPERATURE_SET_MIN 15

#define TEMPERATURE_SET_MAX 30

#define DEFAULT_TEMPERATURE_HYSTERSIS 20
#define TEMP_HYSTERSIS_STEP

/* button pins */
#define BTN_1_GPIO 19
#define BTN_2_GPIO 18
#define BTN_3_GPIO 17
#define BTN_4_GPIO 16

/* lcd line functions */
#define MENU_LINE LCD_LINE_1
#define STATUS_LINE LCD_LINE_2

/* --- STATE MACHINE --- */
/* states */
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
    S_SETN_OFF,
    S_VAR_CHANGE,

    STATE_T_MAX
} state_t;

/* actions */
typedef enum {
    NA_NONE, NA_LEFT, NA_RIGHT, NA_ENTER, NA_ESC,
    NEXT_ACTION_T_MAX
} next_action_t;

/* deafults */
#define DEFAULT_STATE       S_SHOW_TIME
#define DEFAULT_NEXT_ACTION NA_NONE

#define VALUE_DOWN -1
#define VALUE_UP 1
#define VALUE_OK 0

/* static variables - accessed from callbacks */
next_action_t   next_action             = DEFAULT_NEXT_ACTION;
state_t         changing_var_at_state   = S_SLEEP;

/* state transition table */
static const state_t state_table[STATE_T_MAX][NEXT_ACTION_T_MAX] = {
/*                      {NA_NONE,               NA_LEFT,            NA_RIGHT,           NA_ENTER,           NA_ESC}*/
/*S_SLEEP*/             {S_SLEEP,               DEFAULT_STATE,      DEFAULT_STATE,      DEFAULT_STATE,      DEFAULT_STATE},
/*S_SHOW_TIME*/         {S_SHOW_TIME,           S_SHOW_SETN,        S_SHOW_TEMP,        S_SHOW_TIME,        S_SLEEP},
/*S_SHOW_TEMP*/         {S_SHOW_TEMP,           S_SHOW_TIME,        S_SHOW_PROGS,       S_SHOW_TEMP,        S_SLEEP},
/*S_SHOW_PROGS*/        {S_SHOW_PROGS,          S_SHOW_TEMP,        S_SHOW_SETN,        S_PROG_SEL,         S_SLEEP},
/*S_SHOW_SETN*/         {S_SHOW_SETN,           S_SHOW_PROGS,       S_SHOW_TIME,        S_SETN_TIME,        S_SLEEP},
/*S_PROG_SEL*/          {S_PROG_SEL,            S_PROG_SEL,         S_PROG_SEL,         S_PROG_TIME_SET,    S_SHOW_PROGS},
/*S_PROG_TIME_SET*/     {S_PROG_TIME_SET,       S_PROG_TEMP_SET,    S_PROG_WEEKDAY_SET, S_VAR_CHANGE,       S_PROG_SEL},
/*S_PROG_WEEKDAY_SET*/  {S_PROG_WEEKDAY_SET,    S_PROG_TIME_SET,    S_PROG_TEMP_SET,    S_VAR_CHANGE,       S_PROG_SEL},
/*S_PROG_TEMP_SET*/     {S_PROG_TEMP_SET,       S_PROG_WEEKDAY_SET, S_PROG_TIME_SET,    S_VAR_CHANGE,       S_PROG_SEL},
/*S_SETN_TIME*/         {S_SETN_TIME,           S_SETN_OFF,         S_SETN_HYST,        S_VAR_CHANGE,       S_SHOW_SETN},
/*S_SETN_HYST*/         {S_SETN_HYST,           S_SETN_TIME,        S_SETN_OFF,         S_VAR_CHANGE,       S_SHOW_SETN},
/*S_SETN_OFF*/          {S_SETN_OFF,            S_SETN_HYST,        S_SETN_TIME,        S_VAR_CHANGE,       S_SHOW_SETN},
/*S_VAR_CHANGE*/        {S_VAR_CHANGE,          S_VAR_CHANGE,       S_VAR_CHANGE,       S_VAR_CHANGE,       DEFAULT_STATE}, //místo default state se změní stav na předchozí, který se měnil ( je uložený v chaning_var_at_state)
};

/* menu text */
static const char menu_texts[][LCD_MAX_CHARS+1] = {
        "               ",  //S_SLEEP
        "<nast|cas |tepl>",  //S_SHOW_TIME
        "<cas |tepl|prog>",  //S_SHOW_TEMP
        "<tepl|prog|nast>",  //S_SHOW_PROGS
        "<prog|nast| cas>",  //S_SHOW_SETN
        "<#1 >#2 ok#3 x#4",  //S_PROG_SEL
        "zmenit cas      ",  //S_PROG_TIME_SET
        "zmenit dny      ",  //S_PROG_WEEKDAY_SET
        "zmenit teplotu  ",  //S_PROG_TEMP_SET
        "zmenit cas      ",  //S_SETN_TIME
        "zmenit hysterzi ",  //S_SETN_HYST
        "udrzovat teplo? ",  //S_SETN_OFF
        ""                  //S_VAR_CHANGE - special use for empty string to not replace previus value
    };

/* --- THERMOSTAT SETTINGS --- */

/**
 * @typedef thermo_settings_t
 * @param temp_regulation_on true for thermostat to try to keep the temperature
 * @param hystersia is represented in 10x C with wanted temperature in middle and 1/2*val around the temp
 */
typedef struct{
    bool temp_regulation_on;
    uint16_t hystersia;
} thermo_settings_t;

//default settings
thermo_settings_t th_set = {
    .temp_regulation_on = true,
    .hystersia = DEFAULT_TEMPERATURE_HYSTERSIS
};
//*time is stored in RTC

/* --- CALLBACKS --- */
void wake_up() {
    //lcd_reset();
    lcd_refresh_on_timer();
}

void change_var(state_t var, uint16_t dir);

void btn_left() {
    next_action = NA_LEFT;
    if(changing_var_at_state != S_SLEEP) {
        //change value down by ? amount
        //něco by mohlo být změněno potíkem něco čudlíky
        change_var(changing_var_at_state, VALUE_DOWN);
    }
}

void btn_right() {
    next_action = NA_RIGHT;
    if(changing_var_at_state != S_SLEEP) {
        //change value down by ? amount
        //něco by mohlo být změněno potíkem něco čudlíky
        change_var(changing_var_at_state, VALUE_UP);
    }
}

void btn_enter() {
    next_action = NA_ENTER;
    if(changing_var_at_state != S_SLEEP) {
        change_var(changing_var_at_state, VALUE_OK);
    }
}

void btn_cancel() {
    next_action = NA_ESC;
    if(changing_var_at_state != S_SLEEP) {
        change_var(S_SLEEP, VALUE_OK);
    }
}

/* --- PRINTING MENU --- */

void show_menu(state_t state) {
    if(state < 0 || state >= STATE_T_MAX) return;
    lcd_write(menu_texts[state], MENU_LINE);
}

/* --- PRINTING STATUS --- */
void show_time() {
    static char datetime_buf[LCD_MAX_CHARS+1];
    static char *datetime_str = &datetime_buf[0];
    static datetime_t dt;
    static time_t t;

    rtc_get_datetime(&dt);
    datetime_to_time(&dt, &t);
    struct tm* time_struct = localtime(&t);
    strftime(datetime_str, sizeof(datetime_buf), "%H:%M   %d.%m.%y", time_struct);
    lcd_write(datetime_str, STATUS_LINE);
    debug("showing time: %s\n", datetime_str);
}

void show_temperature() {
    static char temp_buf[LCD_MAX_CHARS+1];
    static char temp_str[LCD_MAX_CHARS+1];
    uint16_t temp = get_temp();
    convert_temp_to_str(temp, &temp_buf[0]);
    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s C              ", &temp_buf[0]);
    lcd_write(&temp_str[0], STATUS_LINE);
}

/* @deprecated old test function */
// void show_temp_data(uint16_t temp, float temp_treshold) {
//     char temp_str[6];
//     char write_str[17];

//     if(temp >= 0 && temp < 1000) {
//         convert_temp_to_str(temp, temp_str);
//         sniprintf(write_str, 16, "T. namer: %s", temp_str);
//         lcd_write(write_str, LCD_LINE_1);
//     }

//     if(temp_treshold >= TEMPERATURE_SET_MIN && temp_treshold <= TEMPERATURE_SET_MAX) {
//         sprintf(write_str, "T.  udrz: %2.1f", temp_treshold);
//         lcd_write(write_str, LCD_LINE_2);
//     }
// }

void update_status(state_t state) {
    switch(state) {
        case S_SLEEP:
            lcd_sleep();
            //uspat dokud se nevzbudí kvůli libovolného interuptu?
            // v tom případě po jak dlouhé době se má kontrolovat udržování teploty?
            change_var(S_SLEEP, VALUE_OK);
            return;
        case S_SHOW_TIME:
            show_time();
            break;
        case S_SHOW_TEMP:
            show_temperature();
            break;
        case S_SHOW_PROGS:
            lcd_write("zmenit programy ", STATUS_LINE);
            break;
        case S_SHOW_SETN:
            lcd_write("zmenit nastaveni", STATUS_LINE);
            break;

        /* @todo ostatní stavy */

        default:
            lcd_write("                ", STATUS_LINE);
            break;
    }
}

/* --- WRITE FUNCTIONS --- */

/**
 * This function keeps note of changed variables, changes them and can save(apply) the current value
 * @param var variable that will be changed represented by state. When function is called consectutivly and state is diffrent, then internal value is reset to that of saved value of variable of newe state.
 * If S_SLEEP is called as variable, then internal values are simply reset.
 * @param dir direction. Which way or how the variable should change. 1 usually means up -1 means down and 0 to save.
 */
void change_var(state_t var, uint16_t dir) {
    // @todo když se uspí protože se nic nedělalo tak zavolat tuto funkci s S_SLEEP
    // @todo pomatování zda se proměnná mění a které aby se nejdříve načetla do dočasné hodnoty ta uložená
    // @todo !!! změna hodnoty by se neměla propsat dokud není zmáčknuto ok 






    //update the value on display 
    /* @todo how to show the changing variable? */
    update_status(var);
}

/* set temperature will be center of hystersis curve */
void check_relay_state(float current_temp, float set_temp) {
    if(current_temp > set_temp + th_set.hystersia/20) {
        relay_off();
    } else if(current_temp < set_temp - th_set.hystersia/20) {
        relay_on();
    }
}

/* --- READ FUNCTIONS --- */

float get_temp_treshold() {
    return TEMPERATURE_SET_MIN + (TEMPERATURE_SET_MAX-TEMPERATURE_SET_MIN)*((100-get_pot_val())/100.0f);
}

/* --- MAIN LOOP FUNCTIONS --- */


void main_loop_logic(state_t state) {

    //if temperature regulation is on
    if(th_set.temp_regulation_on) {
        /* @todo get_temp_treshold byl měl brát teplotu ze struktury programů */
        check_relay_state(convert_temp_to_float(get_temp_data()), get_temp_treshold());
    }

    //je důležité pořadí?
    update_status(state);
    show_menu(state);
}

state_t main_loop_step(state_t state) {
    state_t next_state = state_table[state][next_action];
    debug("stavy: %d %d %d\n", state, next_state, next_action);

    //changing variable
    if(next_state == S_VAR_CHANGE && state != S_VAR_CHANGE) {
        changing_var_at_state = state;

    //stopped changing variable
    } else if(next_state != S_VAR_CHANGE && state == S_VAR_CHANGE) {
        next_state = changing_var_at_state;
        changing_var_at_state = S_SLEEP;
    }
    next_action = NA_NONE; // možná dát až po main loop logic?

    static uint16_t timeout = SLEEP_TIMEOUT*1000;

    if(state == next_state) {
        //dekrement sleep timer
        //if sleep timer <= 0 goto sleep
        if(timeout <= 0) {
            //start sleep
            /* @todo sleep */
        } else {
            timeout -= MAIN_LOOP_REFRESH_TIMER;
        }
    } else {
        timeout = SLEEP_TIMEOUT*1000;
    }
    state = next_state;
    return state;
}

int main() {

    /* --- INICIALIZATION --- */
    //busy_wait_ms(2000);
    debug("initialization...\n", "");

    stdio_init_all();

    if(temp_sensor_init()) {
        debug("temp sensor init failed!\n", "");
        return 1;
    }

    if(pot_init()) {
        debug("potenciometer init failed!\n", "");
        return 1;
    }

    rtc_init();

    /* @todo tohle nahradí načtení času z ntp... možná zde tohle nechám a ntp nastaví čas správně později */
    datetime_t t = {
            .year  = 2024,
            .month = 11,
            .day   = 17,
            .dotw  = 0, // 0 is Sunday, so 5 is Friday
            .hour  = 00,
            .min   = 00,
            .sec   = 00
    };

    rtc_set_datetime(&t);
    sleep_us(64); // waiting for rtc to update

    relay_init();

    if(lcd_init()) {
        debug("lcd init failed!\n", "");
        return 1;
    }
    lcd_string("Hello, world!");

    btn_set_common_function(&wake_up);
    init_btn_rising_edge(BTN_1_GPIO, &btn_left);
    init_btn_rising_edge(BTN_2_GPIO, &btn_right);
    init_btn_rising_edge(BTN_3_GPIO, &btn_enter);
    init_btn_rising_edge(BTN_4_GPIO, &btn_cancel);

    sleep_ms(1000);
    lcd_clear();
    debug("initialization done!\n", "");

    /* --- MAIN LOOP --- */

    state_t state = DEFAULT_STATE;
    next_action = DEFAULT_NEXT_ACTION;
    while (true) {

        state = main_loop_step(state);

        main_loop_logic(state);

        sleep_ms(MAIN_LOOP_REFRESH_TIMER);

        debug("main loop end, state: %i\n", state);
    }
}
