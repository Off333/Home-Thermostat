/**
 * lcd.h
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#ifndef LCD_H
#define LCD_H

/* how long display should stay on in miliseconds */
#define LCD_REFRESH_RATE 200

/* after how long should display turn off in seconds*/
#define LCD_ON_TIMER 20

#define LCD_LINE_1 0
#define LCD_LINE_2 1

#define LCD_MAX_LINES      2
#define LCD_MAX_CHARS      16

void lcd_clear();

void lcd_sleep();

// go to location on LCD
void lcd_set_cursor(int line, int position);

void lcd_string(const char *s);

int lcd_init();

void lcd_display_enable(bool enable);

void lcd_write(const char *s, uint line);

void lcd_refresh();

int lcd_refresh_on_timer();

#endif /* LCD_H */