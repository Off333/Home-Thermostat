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
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

//flash
#include "hardware/flash.h"
#include "pico/flash.h"

//wifi
#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

/* DEBUG PRINTING FUNCTION */
#define DEBUG 1
#include "debug.h"

/* MY MODULES */
#include "lcd.h"
#include "relay_driver.h"
#include "digital_tmp_sensor.h"
#include "button_driver.h"

//wifi stuff
#include "ntp.h"
#include "wifi_psswd.h"

/* main loop sleep interval in miliseconds from end of loop to start of next loop */
#define MAIN_LOOP_REFRESH_TIMER 1000 * 5

/* timeout in seconds, after which the device will go to sleep */
//#define SLEEP_TIMEOUT 30

/* wifi constants */
#define USE_WIFI 0
#define USE_NTP 1
#define MAX_CONN_TRIES 10
#define RECON_TIMEOUT 5

/* temperature limits in °C */
#define TEMPERATURE_SET_MIN 150

#define TEMPERATURE_SET_MAX 300

#define DEFAULT_TEMPERATURE_HYSTERESIS 20
#define HYSTERESIS_MIN 04
#define HYSTERESIS_MAX 50

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
    S_MENU_TIME,
    S_MENU_TEMP,
    S_MENU_PROGS,
    S_MENU_SETN,
    S_PROG_SEL,
    S_PROG_TIME_SET,
    S_PROG_WEEKDAY_SET,
    S_PROG_TEMP_SET,
    S_SETN_TIME,
    S_SETN_HYST,
    S_SETN_OFF,
    S_SETN_MIN_TEMP,
    S_SETN_SAVE,
    S_SETN_LOAD,

    S_VAR_CHANGE,
    STATE_T_MAX
} state_t;

/* actions */
typedef enum {
    NA_NONE, NA_LEFT, NA_RIGHT, NA_ENTER, NA_ESC,
    NEXT_ACTION_T_MAX
} next_action_t;

/* defaults */
#define DEFAULT_STATE       S_MENU_TIME
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
/*S_MENU_TIME*/         {S_MENU_TIME,           S_MENU_SETN,        S_MENU_TEMP,        S_MENU_TIME,        S_SLEEP},
/*S_MENU_TEMP*/         {S_MENU_TEMP,           S_MENU_TIME,        S_MENU_PROGS,       S_MENU_TEMP,        S_SLEEP},
/*S_MENU_PROGS*/        {S_MENU_PROGS,          S_MENU_TEMP,        S_MENU_SETN,        S_PROG_SEL,         S_SLEEP},
/*S_MENU_SETN*/         {S_MENU_SETN,           S_MENU_PROGS,       S_MENU_TIME,        S_SETN_TIME,        S_SLEEP},
/*S_PROG_SEL*/          {S_PROG_SEL,            S_PROG_SEL,         S_PROG_SEL,         S_PROG_TIME_SET,    S_MENU_PROGS},
/*S_PROG_TIME_SET*/     {S_PROG_TIME_SET,       S_PROG_TEMP_SET,    S_PROG_WEEKDAY_SET, S_VAR_CHANGE,       S_PROG_SEL},
/*S_PROG_WEEKDAY_SET*/  {S_PROG_WEEKDAY_SET,    S_PROG_TIME_SET,    S_PROG_TEMP_SET,    S_VAR_CHANGE,       S_PROG_SEL},
/*S_PROG_TEMP_SET*/     {S_PROG_TEMP_SET,       S_PROG_WEEKDAY_SET, S_PROG_TIME_SET,    S_VAR_CHANGE,       S_PROG_SEL},
/*S_SETN_TIME*/         {S_SETN_TIME,           S_SETN_LOAD,        S_SETN_HYST,        S_VAR_CHANGE,       S_MENU_SETN},
/*S_SETN_HYST*/         {S_SETN_HYST,           S_SETN_TIME,        S_SETN_OFF,         S_VAR_CHANGE,       S_MENU_SETN},
/*S_SETN_OFF*/          {S_SETN_OFF,            S_SETN_HYST,        S_SETN_MIN_TEMP,    S_VAR_CHANGE,       S_MENU_SETN},
/*S_SETN_MIN_TEMP*/     {S_SETN_MIN_TEMP,       S_SETN_OFF,         S_SETN_SAVE,        S_VAR_CHANGE,       S_MENU_SETN},
/*S_SETN_SAVE*/         {S_SETN_SAVE,           S_SETN_MIN_TEMP,    S_SETN_LOAD,        S_SETN_SAVE,        S_MENU_SETN},
/*S_SETN_LOAD*/         {S_SETN_LOAD,           S_SETN_SAVE,        S_SETN_TIME,        S_SETN_LOAD,        S_MENU_SETN},
/*S_VAR_CHANGE*/        {S_VAR_CHANGE,          S_VAR_CHANGE,       S_VAR_CHANGE,       S_VAR_CHANGE,       DEFAULT_STATE}, //místo default state se změní stav na předchozí, který se měnil ( je uložený v chaning_var_at_state)
};

/* menu text */
static const char menu_texts[][LCD_MAX_CHARS+1] = {

#ifndef LANG_EN
        "",  //S_SLEEP
        "<nast|cas |tepl>",  //S_MENU_TIME
        "<cas |tepl|prog>",  //S_MENU_TEMP
        "<tepl|prog|nast>",  //S_MENU_PROGS
        "<prog|nast| cas>",  //S_MENU_SETN
        "vyber programu  ",  //S_PROG_SEL
        "zacatek-konec   ",  //S_PROG_TIME_SET
        "aktivni dny     ",  //S_PROG_WEEKDAY_SET
        "udrzovat teplotu",  //S_PROG_TEMP_SET
        "zmenit cas      ",  //S_SETN_TIME
        "zmenit hysterezi",  //S_SETN_HYST
        "udrzovani teplot",  //S_SETN_OFF
        "minimalni teplo ",  //S_SETN_MIN_TEMP
        "ulozit nastaveni",  //S_SETN_SAVE
        "nacist nastaveni",  //S_SETN_LOAD
        ""                   //S_VAR_CHANGE - special use for empty string to not replace previous value
#else
        "",  //S_SLEEP
        "<setn|time|temp>",  //S_MENU_TIME
        "<time|temp|prog>",  //S_MENU_TEMP
        "<temp|prog|setn>",  //S_MENU_PROGS
        "<prog|setn|time>",  //S_MENU_SETN
        "select program  ",  //S_PROG_SEL
        "start-end time  ",  //S_PROG_TIME_SET
        "set days        ",  //S_PROG_WEEKDAY_SET
        "set temperature ",  //S_PROG_TEMP_SET
        "set device time ",  //S_SETN_TIME
        "set hysteresis  ",  //S_SETN_HYST
        "regulate temps. ",  //S_SETN_OFF
        "minimal temp    ",  //S_SETN_MIN_TEMP
        "save settings   ",  //S_SETN_SAVE
        "load settings   ",  //S_SETN_LOAD
        ""                   //S_VAR_CHANGE - special use for empty string to not replace previous value
#endif
    };

/* --- THERMOSTAT SETTINGS --- */

typedef enum {
    SETN_TIME_MODE_SHOW,
    SETN_TIME_MODE_SET_HOUR,
    SETN_TIME_MODE_SET_MIN,
    SETN_TIME_MODE_SET_DAY,
    SETN_TIME_MODE_SET_MONTH,
    SETN_TIME_MODE_SET_YEAR,

    SETN_TIME_MODE_MAX
} setn_time_mode;

/**
 * @typedef thermo_settings_t
 * @param temp_regulation_on true for thermostat to try to keep the temperature
 * @param hysteresis is represented in 10x C with wanted temperature in middle and 1/2*val around the temp
 */
typedef struct{
    bool temp_regulation_on;
    uint16_t hysteresis;
    uint16_t min_temp;

    setn_time_mode STM; //special variable for showing which part of time in settings is changing
} thermo_settings_t;

//default settings
thermo_settings_t th_set = {
    .temp_regulation_on = true,
    .hysteresis = DEFAULT_TEMPERATURE_HYSTERESIS,
    .min_temp = TEMPERATURE_SET_MIN,
    .STM = SETN_TIME_MODE_SHOW
};
//*time is stored in RTC

typedef enum {
    PROG_TIME_MODE_SHOW,
    PROG_TIME_MODE_SET_HOUR_START,
    PROG_TIME_MODE_SET_MIN_START,
    PROG_TIME_MODE_SET_HOUR_END,
    PROG_TIME_MODE_SET_MIN_END,

    PROG_TIME_MODE_MAX
} program_time_mode;

#define MONDAY      0b00000001
#define TUESDAY     0b00000010
#define WEDNESDAY   0b00000100
#define THURSDAY    0b00001000
#define FRIDAY      0b00010000
#define SATURDAY    0b00100000
#define SUNDAY      0b01000000
#define WORK_DAY    (MONDAY | TUESDAY | WEDNESDAY | THURSDAY | FRIDAY)
#define WEEKEND     (SATURDAY | SUNDAY)
#define WEEK        (MONDAY | TUESDAY | WEDNESDAY | THURSDAY | FRIDAY | SATURDAY | SUNDAY)
#define NEVER       0b00000000

/* --- PROGRAMS STRUCTURE --- */

/**
 * @typedef program_t
 */
typedef struct{
    datetime_t  start;
    datetime_t  end;
    uint16_t    temp;
    uint8_t     days;
    
    program_time_mode PTM; //special variable for showing which part of time in program is changing
} program_t;

#define NUMBER_OF_PROGRAMS 14

program_t programs[NUMBER_OF_PROGRAMS]; // @todo initialize the structures before main loop
uint16_t selected_program = 0;


datetime_t temp_time = {
        .year  = 2024,
        .month = 12,
        .day   = 01,
        .dotw  = 0, // 0 is Sunday, so 5 is Friday
        .hour  = 00,
        .min   = 00,
        .sec   = 00
};

/* --- HELPER FUNCTIONS --- */

void convert_datetime_to_str(char *str, size_t buf_size, const char *fmt, datetime_t *dt) {
    static time_t t;
    datetime_to_time(dt, &t);
    struct tm* time_struct = localtime(&t);
    strftime(str, buf_size, fmt, time_struct);
}

/**
 * month in range 1 to 12 1 is january
 */
uint16_t get_max_days_in_month(uint8_t month, uint16_t year) {
    const static uint8_t max_days[] = {
        0,  31, 28,//uhmmmmm únor je na 2 věci... -_-
        31, 30, 31,
        30, 31, 31,
        30, 31, 30,
        31
    };

    //leap year... it's not perfect, but only approximation (this code will not be used for more then 400 years)
    if(month == 2 && !(year%4) && (year%100)) return 29;

    return max_days[month];
}

#define is_prog_enabled(prog_num) (programs[prog_num].days != NEVER)
// bool is_prog_enabled(uint16_t prog_num) {
//     return programs[prog_num].days != NEVER;
// }

#define is_prog_setting(state) (state == S_PROG_TEMP_SET || state == S_PROG_TIME_SET || state == S_PROG_WEEKDAY_SET)
#define is_setn_setting(state) (state == S_SETN_HYST || state == S_SETN_OFF || state == S_SETN_TIME)

//will not work correctly for int == 0
#define sign(int) (int > 0 ? 1 : -1)

//convert day of the week from datetime_t to internal dotw format
#define convert_dotw(day) (0b1<<((day+6)%7))

/* --- CALLBACKS --- */
void state_machine();

void wake_up() {
    //lcd_reset();
    lcd_refresh_on_timer();
}

void change_var(state_t var, int16_t dir);

void btn_left() {
    next_action = NA_LEFT;
    if(changing_var_at_state != S_SLEEP) {
        //change value down by ? amount
        //něco by mohlo být změněno potíkem něco čudlíky
        change_var(changing_var_at_state, VALUE_DOWN);
    } else state_machine();
}

void btn_right() {
    next_action = NA_RIGHT;
    if(changing_var_at_state != S_SLEEP) {
        //change value down by ? amount
        //něco by mohlo být změněno potíkem něco čudlíky
        change_var(changing_var_at_state, VALUE_UP);
    } else state_machine();
}

void btn_enter() {
    next_action = NA_ENTER;
    if(changing_var_at_state != S_SLEEP) {
        change_var(changing_var_at_state, VALUE_OK);
    } 
    if(changing_var_at_state != S_VAR_CHANGE)  state_machine();
}

void btn_cancel() {
    next_action = NA_ESC;
    if(changing_var_at_state != S_SLEEP) {
        change_var(S_SLEEP, VALUE_OK);
    }
    if(changing_var_at_state != S_VAR_CHANGE) state_machine(); //is this ok?
}

/* --- PRINTING MENU --- */

void show_menu(state_t state) {
    if(state < 0 || state >= STATE_T_MAX) return;
    lcd_write(menu_texts[state], MENU_LINE);
}

/* --- PRINTING STATUS --- */

// formát: HH:MM DD:MM:RRRR
// podobně jako čas u programů měnit pomocí potíku?
void show_time(setn_time_mode mode) {
    static const char time_fmt[SETN_TIME_MODE_MAX][17] = {
        "%H:%M %d.%m.%Y", ">%H<:%M %d.%m.%y", "%H:>%M< %d.%m.%y", "%H:%M >%d<.%m.%y", "%H:%M %d.>%m<.%y", ":%M %d.%m.>%Y<"
    };
    static char datetime_buf[LCD_MAX_CHARS+1];
    static char *datetime_str = &datetime_buf[0];
    static datetime_t dt;
    if(!mode) rtc_get_datetime(&dt);
    else dt = temp_time;
    convert_datetime_to_str(datetime_str, sizeof(datetime_buf), time_fmt[mode], &dt);
    lcd_write(datetime_str, STATUS_LINE);
    debug("showing time: %s", datetime_str);
}

void show_temperature(uint16_t temp) {
    static char temp_buf[LCD_MAX_CHARS+1];
    static char temp_str[LCD_MAX_CHARS+1];
    convert_temp_to_str(temp, &temp_buf[0]);
    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s C              ", &temp_buf[0]);
    lcd_write(&temp_str[0], STATUS_LINE);
}

void show_prog_selection() {
    static char temp_str[LCD_MAX_CHARS+1];
    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "#%u %s          ", selected_program+1, is_prog_enabled(selected_program) ?
#ifndef LANG_EN 
    "ZAPNUT" : "VYPNUT"
#else
    "ON" : "OFF"
#endif
    );
    lcd_write(&temp_str[0], STATUS_LINE);
}

//@todo jak označit co se mění? kurzor?
//mohlo by být takto: >HH<:MM-HH:MM -> HH:>MM<-HH:MM -> atd.
//měnit pomocí potíku?... potík bez zarážek? hmm
//format HH:MM-HH:MM
void show_prog_time(datetime_t* start, datetime_t* end, program_time_mode mode) {
    const static char start_time_fmt[PROG_TIME_MODE_MAX][8] = {
        "%H:%M", ">%H<:%M", "%H:>%M<", "%H:%M", "%H:%M"
    };
    const static char end_time_fmt[PROG_TIME_MODE_MAX][8] = {
        "%H:%M", "%H:%M", "%H:%M", ">%H<:%M", "%H:>%M<"
    };
    static char temp_str[LCD_MAX_CHARS+1];
    convert_datetime_to_str(&temp_str[0], 8, start_time_fmt[mode], start);
    //this should not write over the whole string, but only change first 5 characters
    static char temp_2[LCD_MAX_CHARS+1];
    static char temp_3[LCD_MAX_CHARS+1];
    convert_datetime_to_str(&temp_2[0], 8, end_time_fmt[mode], end);
    snprintf(&temp_3[0], sizeof(temp_3), "%s-%s         ", &temp_str[0], &temp_2[0]);
    debug("|%s|%s|%s|", start_time_fmt[mode], end_time_fmt[mode], temp_3);
    lcd_write(&temp_3[0], STATUS_LINE);

}

//formát ->
/*
PONDELI
UTERY
STREDA
CTVRTEK
PATEK
SOBOTA
NEDELE
    tyto tři možnosti by měli být jako první
PRACOVNI DEN    myšleno pondělí až pátek... mohlo by být taky takto: PONDELI-PATEK
VIKEND          mohlo by být takto: SOBOTA+NEDELE
CELY TYDEN
*/
void show_prog_weekday(uint8_t days) {
    static char temp_str[LCD_MAX_CHARS+1];
    switch(days) {
                case WEEK:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "CELY TYDEN"
#else
            "EVERY DAY"
#endif              
                    );
                    break;
                case WORK_DAY:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "PRACOVNI DNY"
#else
            "WORK DAYS"
#endif              
                    );
                    break;
                case WEEKEND:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "VIKEND"
#else
            "WEEKEND"
#endif              
                    );
                    break;
                case MONDAY:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "PONDELI"
#else
            "MONDAY"
#endif              
                    );
                    break;
                case TUESDAY:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "UTERY"
#else
            "TUESDAY"
#endif              
                    );
                    break;
                case WEDNESDAY:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "STREDA"
#else
            "WEDNESDAY"
#endif              
                    );
                    break;
                case THURSDAY:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "CTVRTEK"
#else
            "THURSDAY"
#endif              
                    );
                    break;
                case FRIDAY:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "PATEK"
#else
            "FRIDAY"
#endif              
                    );
                    break;
                case SATURDAY:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "SOBOTA"
#else
            "SATURDAY"
#endif              
                    );
                    break;
                case SUNDAY:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "NEDELE"
#else
            "SUNDAY"
#endif              
                    );
                    break;
                default:
                    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s             ", 
#ifndef LANG_EN
            "NIKDY"
#else
            "NEVER"
#endif              
                    );
                break;
            }
            lcd_write(&temp_str[0], STATUS_LINE);
}

// formát: TT.T C
// void show_prog_temp(uint16_t temp) {
//     static char temp_str[LCD_MAX_CHARS+1];
    
// }

void show_setn_reg_state(bool state) {
    static char temp_str[LCD_MAX_CHARS+1];
    snprintf(&temp_str[0], LCD_MAX_CHARS+1, "%s          ", state ?
#ifndef LANG_EN 
    "ZAPNUTO" : "VYPNUTO"
#else
    "ON" : "OFF"
#endif
    );
    lcd_write(&temp_str[0], STATUS_LINE);

}

/**
 * 
 */
void update_status(state_t state, program_t* program, thermo_settings_t* settings) {
    debug("updating status", "");
    switch(state) {
        case S_SLEEP:
            //uspat dokud se nevzbudí kvůli libovolného interputu?
            //v tom případě po jak dlouhé době se má kontrolovat udržování teploty?

            //change_var(S_SLEEP, VALUE_OK); // <-- TOHLE NESMÍ BÝT VOLANÉ V RÁMCI UPDATE STATUS!!! JINAK JE ZDE NEKONEČNÁ SMYČKA! ... zatím dokud něco nezměním v rámci change_var
            return;
        case S_MENU_TIME:
            show_time(SETN_TIME_MODE_SHOW);
            break;
        case S_MENU_TEMP:
            show_temperature(get_temp());
            break;
        case S_MENU_PROGS:
#ifndef LANG_EN
            lcd_write("zmenit programy ", STATUS_LINE);
#else
            lcd_write("change programs ", STATUS_LINE);
#endif
            break;
        case S_MENU_SETN:
#ifndef LANG_EN
            lcd_write("zmenit nastaveni", STATUS_LINE);
#else
            lcd_write("change settings ", STATUS_LINE);
#endif
            break;
        
        case S_PROG_SEL:
            show_prog_selection();
            break;
        
        case S_PROG_TIME_SET:
            show_prog_time(&(program->start), &(program->end), program->PTM);
            break;

        case S_PROG_WEEKDAY_SET:
            show_prog_weekday(program->days);
            break;
            
        case S_PROG_TEMP_SET:
            show_temperature(program->temp);
            break;

        case S_SETN_TIME:
            show_time(settings->STM);
            break;

        case S_SETN_HYST:
            show_temperature(settings->hysteresis);
            break;

        case S_SETN_OFF:
            show_setn_reg_state(settings->temp_regulation_on);
            break;
        
        case S_SETN_MIN_TEMP:
            show_temperature(settings->min_temp);
            break;
        // case S_VAR_CHANGE:
        //     break;
        default:
            //lcd_write("                ", STATUS_LINE);
            break;
    }
}

/* --- WRITE FUNCTIONS --- */

/**
 * This function keeps note of changed variables, changes them and can save(apply) the current value
 * @param var variable that will be changed represented by state. When function is called consecutively and state is different, then internal value is reset to that of saved value of variable of new state.
 * If S_SLEEP is called as variable, then internal values are simply reset.
 * @param dir direction. Which way or how the variable should change. 1 usually means up -1 means down and 0 to save.
 */
void change_var(state_t var, int16_t dir) {
    // @todo když se uspí protože se nic nedělalo tak zavolat tuto funkci s S_SLEEP
    // @todo pamatování zda se proměnná mění a které aby se nejdříve načetla do dočasné hodnoty ta uložená
    // @todo !!! změna hodnoty by se neměla propsat dokud není zmáčknuto ok

    static state_t previous_var = S_SLEEP;
    if(var == S_SLEEP) {
        previous_var = S_SLEEP;
        return;
    }


    bool prog_changing = false;
    static program_t temp_program;

    bool setn_changing = false;
    static thermo_settings_t temp_setn = { //this may not be needed? This just in case, when the data from saved wasn't fetched...
        .temp_regulation_on = true,
        .hysteresis = DEFAULT_TEMPERATURE_HYSTERESIS,
        .STM = SETN_TIME_MODE_SHOW
    };

    //change temporary variables to current values 
    if(var != previous_var) {
        temp_program = programs[selected_program];
        temp_setn = th_set;
        previous_var = var;
        temp_program.PTM = PROG_TIME_MODE_SET_HOUR_START;
        temp_setn.STM = SETN_TIME_MODE_SET_HOUR;
        if(var == S_SETN_TIME) rtc_get_datetime(&temp_time); //get time from rtc only when it's needed
    } else {
        //choose whatever device settings or program settings are are being changed
        if(is_prog_setting(var)) prog_changing = true;
        else if(is_setn_setting(var)) setn_changing = true;
    }

    //change the variable according to direction
    switch(var) {
        case S_PROG_TIME_SET:
            if(dir) {
                switch (temp_program.PTM) {
                    case PROG_TIME_MODE_SET_HOUR_START:
                        temp_program.start.hour = (uint8_t)((24+(int16_t)temp_program.start.hour + dir)%24);
                        break;
                    case PROG_TIME_MODE_SET_MIN_START:
                        temp_program.start.min = (uint8_t)((60+(int16_t)temp_program.start.min + dir)%60);
                        break;
                    case PROG_TIME_MODE_SET_HOUR_END:
                        temp_program.end.hour = (uint8_t)((24+(int16_t)temp_program.end.hour + dir)%24);
                        break;
                    case PROG_TIME_MODE_SET_MIN_END:
                        temp_program.end.min = (uint8_t)((60+(int16_t)temp_program.end.min + dir)%60);
                        break;
                }
            } else if(prog_changing) { //Save
                //next time value
                if(temp_program.PTM < PROG_TIME_MODE_MAX-1) {
                    temp_program.PTM += 1;
                    changing_var_at_state = previous_var = var; // keep in the changing state even after enter
                } else { //actual save
                    // @todo validation
                    programs[selected_program].start.hour  =  temp_program.start.hour; 
                    programs[selected_program].start.min   =  temp_program.start.min;
                    programs[selected_program].end.hour    =  temp_program.end.hour;
                    programs[selected_program].end.min     =  temp_program.end.min;
                    previous_var = S_SLEEP; // this must be at the save of each case, because of how time values are changed
                    next_action = NA_ESC;
                } 
            }
            break;
        case S_PROG_WEEKDAY_SET:
            if(dir) { // @bug čudlík se bouncne a tohle přepne 2x :(
                // NEVER -> WORK_DAY -> WEEKEND -> WEEK -> MONDAY -> TUESDAY -> WEDNESDAY -> THURSDAY -> FRIDAY -> SATURDAY -> SUNDAY
                switch (temp_program.days) { //this should be done absolutely different way
                    case NEVER:
                        temp_program.days = dir < 0 ? SUNDAY : WORK_DAY;
                        break;
                    case WORK_DAY:
                        temp_program.days = dir < 0 ? NEVER : WEEKEND;
                        break;
                    case WEEKEND:
                        temp_program.days = dir < 0 ? WORK_DAY : WEEK;
                        break;
                    case WEEK:
                        temp_program.days = dir < 0 ? WEEKEND : MONDAY;
                        break;
                    case MONDAY:
                        temp_program.days = dir < 0 ? WEEK : TUESDAY;
                        break;
                    case TUESDAY:
                        temp_program.days = dir < 0 ? MONDAY : WEDNESDAY;
                        break;
                    case WEDNESDAY:
                        temp_program.days = dir < 0 ? TUESDAY : THURSDAY;
                        break;
                    case THURSDAY:
                        temp_program.days = dir < 0 ? WEDNESDAY : FRIDAY;
                        break;
                    case FRIDAY:
                        temp_program.days = dir < 0 ? THURSDAY : SATURDAY;
                        break;
                    case SATURDAY:
                        temp_program.days = dir < 0 ? FRIDAY : SUNDAY;
                        break;
                    case SUNDAY:
                        temp_program.days = dir < 0 ? SATURDAY : NEVER;
                        break;
                }
                debug("switching days to: %d", temp_program.days);
        
            } else { //Save
                // @todo validation
                programs[selected_program].days = temp_program.days; 
                previous_var = S_SLEEP;
                next_action = NA_ESC;
            }
            break;
        case S_PROG_TEMP_SET:
            if(dir) {
                //I can use max and min pro standard math library...
                temp_program.temp = temp_program.temp + dir;
                if(temp_program.temp < TEMPERATURE_SET_MIN) temp_program.temp = TEMPERATURE_SET_MIN;
                else if(temp_program.temp > TEMPERATURE_SET_MAX) temp_program.temp = TEMPERATURE_SET_MAX;
            } else { //Save
                //I hope no validation si needed
                programs[selected_program].temp = temp_program.temp;
                previous_var = S_SLEEP;
                next_action = NA_ESC;
            }
            break;
        case S_SETN_TIME:
            if(dir) {
                switch (temp_setn.STM) {
                    case SETN_TIME_MODE_SET_HOUR:
                        temp_time.hour = (uint8_t)((24+(int16_t)temp_time.hour + dir)%24);
                        break;
                    case SETN_TIME_MODE_SET_MIN:
                        temp_time.min = (uint8_t)((60+(int16_t)temp_time.min + dir)%60);
                        break;
                    case SETN_TIME_MODE_SET_DAY:
                        uint8_t max_days = get_max_days_in_month(temp_time.month, temp_time.year);
                        temp_time.day = (int8_t)((max_days+(int16_t)temp_time.day + dir)%max_days);
                        break;
                    case SETN_TIME_MODE_SET_MONTH:
                        temp_time.month = (uint8_t)((12+(int16_t)temp_time.month + dir)%12);
                        break;
                    case SETN_TIME_MODE_SET_YEAR:
                        temp_time.year = temp_time.year + dir;
                        break;
                }
            } else if(setn_changing) { //Save
                //next time value
                if(temp_setn.STM < SETN_TIME_MODE_MAX-1) {
                    temp_setn.STM += 1;
                    changing_var_at_state = previous_var = var; // keep in the changing state even after enter
                } else { //actual save
                    // @todo validation
                    rtc_set_datetime(&temp_time);
                    previous_var = S_SLEEP; // this must be at the save of each case, because of how time values are changed
                    next_action = NA_ESC;
                } 
            }
            break;
        case S_SETN_HYST:
            if(dir) {
                //I can use max and min pro standard math library...
                temp_setn.hysteresis = temp_setn.hysteresis + 2*dir;
                if(temp_setn.hysteresis < HYSTERESIS_MIN) temp_setn.hysteresis = HYSTERESIS_MIN;
                else if(temp_setn.hysteresis > HYSTERESIS_MAX) temp_setn.hysteresis = HYSTERESIS_MAX;
            } else { //Save
                th_set.hysteresis = temp_setn.hysteresis;
                previous_var = S_SLEEP;
                next_action = NA_ESC;
            }
            break;
        case S_SETN_OFF:
            if(dir) {
                temp_setn.temp_regulation_on = !temp_setn.temp_regulation_on;
            } else { //Save
                th_set.temp_regulation_on = temp_setn.temp_regulation_on;
                previous_var = S_SLEEP;
                next_action = NA_ESC;
            }
            break;
        case S_SETN_MIN_TEMP:
            if(dir) {
                //I can use max and min pro standard math library...
                temp_setn.min_temp = temp_setn.min_temp + dir;
                if(temp_setn.min_temp < TEMPERATURE_SET_MIN) temp_setn.min_temp = TEMPERATURE_SET_MIN;
                else if(temp_setn.min_temp > TEMPERATURE_SET_MAX) temp_setn.min_temp = TEMPERATURE_SET_MAX;
            } else { //Save
                th_set.min_temp = temp_setn.min_temp;
                previous_var = S_SLEEP;
                next_action = NA_ESC;
            }
            break;
    }

    //update the value on display 
    update_status(var, &temp_program, &temp_setn);
}

/* set temperature will be center of HYSTERESIS curve */
void check_relay_state(float current_temp, float set_temp) {
    debug("checking relay state %f %f", current_temp, set_temp);
    if(current_temp > set_temp + th_set.hysteresis/20) {
        debug("relay off", "");
        relay_off();
    } else if(current_temp < set_temp - th_set.hysteresis/20) {
        debug("relay on", "");
        relay_on();
    }
}

/* --- READ FUNCTIONS --- */


// float get_temp_threshold() {
//     return TEMPERATURE_SET_MIN/10 + (TEMPERATURE_SET_MAX-TEMPERATURE_SET_MIN)*((100-get_pot_val())/1000.0f);
// }

/* --- FLASH WRITE/READ FUNCTIONS --- */
//https://github.com/raspberrypi/pico-examples/blob/master/flash/program/flash_program.c
//tohle půjde stejně do vlastního souboru tohle je jen pro @test
typedef struct{
    uint16_t min_temp;
    uint16_t hysteresis;
    uint8_t  flags; // -|-|-|-|-|-|-|ON/OFF
} thermo_settings_flash_t;

typedef struct{
    uint16_t start;
    uint16_t end;
    uint16_t temp;
    uint8_t  days;
} program_flash_t;

static_assert(sizeof(thermo_settings_flash_t)+NUMBER_OF_PROGRAMS*sizeof(program_flash_t) <= FLASH_PAGE_SIZE);
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
const uint8_t* flash_target = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
bool FLASH_CLEARED = false;

/**
 * 
 * NOT SAFE!
 */
static void flash_erase(void* target) {
    uint32_t offset = (uint32_t)target;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

/**
 * 
 * NOT SAFE!
 */
static void flash_program(void* target) {
    uint32_t offset  = ((uintptr_t*)target)[0];
    const uint8_t* data = (const uint8_t*)((uintptr_t*)target)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}

int flash_erase_safe() {
    //místo hard assertu vrátit chybu 
    hard_assert(flash_safe_execute(flash_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX) == PICO_OK);
    FLASH_CLEARED = true;
    return 0;
}

int flash_program_safe(uint8_t* buf, size_t buf_len) {
    uint8_t data[FLASH_PAGE_SIZE] = {0};
    memcpy(&data, buf, buf_len);
    uintptr_t params[] = {FLASH_TARGET_OFFSET, (uintptr_t)data};
    //místo hard assertu vrátit chybu 
    hard_assert(flash_safe_execute(flash_program, params, UINT32_MAX) == PICO_OK);
    FLASH_CLEARED = false;
    bool mismatch = false;
    for(uint i = 0; i < buf_len; ++i) {
        if(buf[i] != flash_target[i]) return 1;
    }
    return 0;
}

#define datetime_to_uint8_t(datetime) ((((uint16_t)datetime.hour) << 8) | ((uint16_t)datetime.min))

void save_settings() {
    if(flash_erase_safe()) return; // zde by mělo být chybové hlášení @todo
    uint8_t data[sizeof(thermo_settings_flash_t)+NUMBER_OF_PROGRAMS*sizeof(program_flash_t)];
    uint8_t *(data_pntr) = data;
    thermo_settings_flash_t temp_settings = {
        .min_temp = th_set.min_temp,
        .hysteresis = th_set.hysteresis,
        .flags = th_set.temp_regulation_on
    };
    memcpy(data_pntr, &temp_settings, sizeof(thermo_settings_flash_t));
    data_pntr += sizeof(thermo_settings_flash_t);
    program_flash_t temp_prog;
    for(uint i = 0; i < NUMBER_OF_PROGRAMS; ++i) {
        temp_prog.start = datetime_to_uint8_t(programs[i].start);
        temp_prog.end   = datetime_to_uint8_t(programs[i].end);
        temp_prog.temp  = programs[i].temp;
        temp_prog.days  = programs[i].days;
        memcpy(data_pntr, &temp_prog, sizeof(program_flash_t));
        data_pntr += sizeof(program_flash_t);
    }
    if(flash_program_safe(data, sizeof(data))) debug("ERROR - FLASH PROGRAMMING FAILED", "");
    else lcd_write("OK", STATUS_LINE);
}

void load_settings() {
    if(FLASH_CLEARED) return; // zde by mělo být chybové hlášení @todo
    uint8_t* data_pntr = (uint8_t*)flash_target;
    thermo_settings_flash_t temp_settings;
    memcpy(&temp_settings, data_pntr, sizeof(thermo_settings_flash_t));
    th_set.min_temp           = temp_settings.min_temp;
    th_set.hysteresis         = temp_settings.hysteresis;
    th_set.temp_regulation_on = temp_settings.flags << 7;
    data_pntr += sizeof(thermo_settings_flash_t);
    program_flash_t temp_prog;
    for(uint i = 0; i < NUMBER_OF_PROGRAMS; ++i) {
        memcpy(&temp_prog, data_pntr, sizeof(program_flash_t));
        programs[i].start.hour = (temp_prog.start >> 8);
        programs[i].start.min = (temp_prog.start << 8) >> 8;
        programs[i].end.hour = (temp_prog.end >> 8);
        programs[i].end.min = (temp_prog.end << 8) >> 8;
        programs[i].temp = temp_prog.temp;
        programs[i].days = temp_prog.days;
        data_pntr += sizeof(program_flash_t);
    }
    lcd_write("OK", STATUS_LINE);
}


/* --- MAIN LOOP FUNCTIONS --- */


void main_loop_logic(state_t state) {

    //je důležité pořadí?
    update_status(state, &(programs[selected_program]), &th_set);
    show_menu(state);
}

state_t main_loop_step(state_t state) {
    state_t next_state = state_table[state][next_action];
    debug("state: %d next_state: %d action: %d s_var_change: %d", state, next_state, next_action, changing_var_at_state);

    //@TODO tohle asi předělat na switch... začíná to být nečitelné

    //changing variable
    if(next_state == S_VAR_CHANGE && state != S_VAR_CHANGE) {
        changing_var_at_state = state;
        change_var(changing_var_at_state, VALUE_OK);
    //stopped changing variable
    } else if(next_state != S_VAR_CHANGE && state == S_VAR_CHANGE) {
        debug("%d %d", state, next_state);
        next_state = changing_var_at_state;
        changing_var_at_state = S_SLEEP;

    //selecting program
    } else if(state == S_PROG_SEL && next_action == NA_LEFT) {
        selected_program = (NUMBER_OF_PROGRAMS+selected_program-1) % NUMBER_OF_PROGRAMS;
    } else if(state == S_PROG_SEL && next_action == NA_RIGHT) {
        selected_program = (NUMBER_OF_PROGRAMS+selected_program+1) % NUMBER_OF_PROGRAMS;
    } else if(state != S_SLEEP && next_state == S_SLEEP) {
        lcd_sleep();
    } else if(state == S_SETN_SAVE && next_action == NA_ENTER) {
        save_settings();
    } else if(state == S_SETN_LOAD && next_action == NA_ENTER) {
        load_settings();
    } else if (next_state == S_SETN_SAVE || next_state == S_SETN_LOAD) {
        lcd_write("                ", STATUS_LINE); //tohle by tu nemělo být... ale nwm jak to udělat lépe, aby se zobrazovalo to ok? ... hmm @TODO
    }
    next_action = NA_NONE; // možná dát až po main loop logic?



    // static uint32_t timeout = SLEEP_TIMEOUT*1000;
    // if(state == next_state && state != S_VAR_CHANGE) {
    //     //dekrement sleep timer
    //     //if sleep timer <= 0 goto sleep
    //     if(timeout <= 0) {
    //         //start sleep
    //         change_var(S_SLEEP, VALUE_OK);
    //         lcd_sleep();
    //         next_state = S_SLEEP;
    //         //nastavit stav na sleep?
    //         /* @todo sleep */
    //     } else {
    //         timeout -= MAIN_LOOP_REFRESH_TIMER;
    //     }
    // } else {
    //     timeout = SLEEP_TIMEOUT*1000;
    // }

    state = next_state;
    return state;
}

void state_machine() {
    static state_t state = DEFAULT_STATE;
    state = main_loop_step(state);
    main_loop_logic(state);
}

void regulate_temps() {

    debug("regulating temperature", "");

    datetime_t current_time;
    rtc_get_datetime(&current_time);

    uint8_t dotw = convert_dotw(current_time.dotw);

    uint16_t temp = th_set.min_temp;

    for(uint8_t i = 0; i < NUMBER_OF_PROGRAMS; i++) {
        program_t p = programs[i];
        /*debug("prog %d dotw %d, start h %d min %d end h %d min %d",
            i,
            p.days & dotw, 
            current_time.hour >= p.start.hour, 
            current_time.min >= p.start.min, 
            current_time.hour <= p.end.hour, 
            current_time.min <= p.end.min);*/

        if(
            p.days & dotw &&
            current_time.hour >= p.start.hour &&
            current_time.min >= p.start.min &&
            current_time.hour <= p.end.hour &&
            current_time.min <= p.end.min
        ) {
            temp = p.temp;
            //debug("program %d on!", i);
            break;
        }
    }


    uint16_t measured_temp = get_temp();
    if(measured_temp && temp) check_relay_state(convert_temp_to_float(measured_temp), convert_temp_to_float(temp));
}

int main() {

    /* --- INITIALIZATION --- */
    for(int i = 0; i < 1; ++i) {
        printf("START\n");
        busy_wait_ms(1000); 
    }
    debug("initialization...", "");

    stdio_init_all();

    if(temp_sensor_init()) {
        debug("temp sensor init failed!", "");
        return 1;
    }
    /*
    if(pot_init()) {
        debug("potenciometr init failed!", "");
        return 1;
    }*/

    rtc_init();

    /* @todo tohle nahradí načtení času z ntp... možná zde tohle nechám a ntp nastaví čas správně později */
    datetime_t t = {
            .year  = 2024,
            .month = 12,
            .day   = 01,
            .dotw  = 0, // 0 is Sunday, so 5 is Friday
            .hour  = 00,
            .min   = 00,
            .sec   = 00
    };

    rtc_set_datetime(&t);
    sleep_us(64); // waiting for rtc to update

    relay_init();

    if(lcd_init()) {
        debug("lcd init failed!", "");
        return 1;
    }
    lcd_write("lcd ok          ", MENU_LINE);
    debug("lcd initialized!", "");

    t.year = -1;
    t.month = -1;
    t.day = -1;
    t.dotw = 00;
    t.hour = 00;
    t.min = 00;
    t.sec = 00;

    datetime_t t2 = { // @test
        .year  = -1,
        .month = -1,
        .day   = -1,
        .dotw  = 0, // 0 is Sunday, so 5 is Friday
        .hour  = 23,
        .min   = 59,
        .sec   = 00
    };
    
    for(int i = 0; i < NUMBER_OF_PROGRAMS; i++) {
        programs[i].start   = t; 
        programs[i].end     = t2;
        programs[i].days    = NEVER;
        programs[i].temp    = TEMPERATURE_SET_MIN;
        programs[i].PTM     = PROG_TIME_MODE_SHOW;
    }
    lcd_write("programs ok     ", MENU_LINE);
    debug("programs initialized!", "");

#if USE_WIFI

    if (cyw43_arch_init()) {
        debug("failed to initialise wifi driver", "");
        return 1;
    }
    

    cyw43_arch_enable_sta_mode();

    lcd_write("connecting wifi ", MENU_LINE);
    debug("connecting to wifi!", "");

    int err_code = -1;
    char status_str[MAX_CONN_TRIES+1] = {0};

    for(int i = 0; i < MAX_CONN_TRIES; i++) {
        err_code = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000);
        if(!err_code) break;
        debug("failed to connect to wifi - code %d", err_code);
        sleep_ms(1000*RECON_TIMEOUT+ rand()%1000);
        status_str[i] = '.';
        lcd_write(&status_str[0], STATUS_LINE);
    }
    lcd_write("                ", STATUS_LINE);

    if(err_code) {
        debug("max connection attempts reached", "");
        cyw43_arch_deinit();
        return err_code;
    }
    lcd_write("wifi ok         ", MENU_LINE);
    debug("connected to wifi!", "");

    busy_wait_ms(600);

#if USE_NTP 

    lcd_write("waiting for ntp ", MENU_LINE);
    debug("probing ntp!", "");

    struct tm ntp_time;

    run_ntp(&ntp_time);
    debug("got time from ntp!", "");

    time_t mkt = mktime(&ntp_time);

    if(!time_to_datetime(mkt, &t)) {
        debug("time conversion error (time_t to datetime_t)", "");
        cyw43_arch_deinit();
        return 1;
    }

    rtc_set_datetime(&t);

    lcd_write("time ok", MENU_LINE);
    debug("set time to rtc!", "");

#endif

    busy_wait_ms(200);

    cyw43_arch_deinit();

#endif


    changing_var_at_state = S_VAR_CHANGE; //state machine will not change
    btn_set_common_function(&wake_up);
    init_btn_rising_edge(BTN_1_GPIO, &btn_left);
    init_btn_rising_edge(BTN_2_GPIO, &btn_right);
    init_btn_rising_edge(BTN_3_GPIO, &btn_enter);
    init_btn_rising_edge(BTN_4_GPIO, &btn_cancel);

    debug("button initialization ok!", "");
    lcd_write("buttons ok", MENU_LINE);

    busy_wait_ms(300);
    lcd_clear();
    debug("initialization done!", "");

    
    next_action = DEFAULT_NEXT_ACTION;
    changing_var_at_state = S_SLEEP;
    state_machine(); //first step of state machine

    /* --- MAIN LOOP --- */

    while (true) {
        //if temperature regulation is on
        if(th_set.temp_regulation_on) {
            regulate_temps();
        }
        state_machine();
        sleep_ms(MAIN_LOOP_REFRESH_TIMER);
    }
}
