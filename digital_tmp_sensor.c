/**
 * digital_tmp_sensor.c
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/

#include "digital_tmp_sensor.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "pico/stdlib.h"
#include "hardware/sync.h"

#define DEBUG 1
#include "debug.h"

#define MY_DHT_PIN 22
#define MY_DHT_MAX_TIMINGS 85
#define MY_DHT_READING_DELAY_MS 2500
static_assert(MY_DHT_READING_DELAY_MS >= 2000, "Min reading delay = 2s");

bool dht_sensor_busy = true;

typedef struct {
    float humidity;
    float temp_celsius;
} dht_reading;

int read_from_dht(dht_reading *result);

int64_t dht_busy_callback(alarm_id_t id, __unused void *user_data) {
    dht_sensor_busy = false;
    return 0;
}

int temp_sensor_init() {
    
    gpio_init(MY_DHT_PIN);
    add_alarm_in_ms(MY_DHT_READING_DELAY_MS, dht_busy_callback, NULL, false);
    
    return 0;
}

uint16_t get_temp() {
    static uint16_t saved_temp = 10;
    
    //THIS SETS THE VALUE OF DHT_SENSOR_BUSY ON PURPOSE TO PREVENT RACE CONDITIONS TO NOT READ FROM SENSOR TOO FAST
    if(dht_sensor_busy && (dht_sensor_busy = true)) return saved_temp;
    
    debug("reading temps", "");
    dht_reading reading;
    uint32_t interrupts_state = save_and_disable_interrupts();
    if(!read_from_dht(&reading)) saved_temp = (uint16_t)(reading.temp_celsius*10);
    //restore_interrupts(interrupts_state);
    restore_interrupts_from_disabled(interrupts_state);
    debug("got temps: %f", reading.temp_celsius);
    
    add_alarm_in_ms(MY_DHT_READING_DELAY_MS, dht_busy_callback, NULL, false);
    return saved_temp;
}

int read_from_dht(dht_reading *result) {
    uint timeout = MY_DHT_MAX_TIMINGS;
    uint cnt = 0;
    //unsigned long long real_time = to_ms_since_boot(get_absolute_time());
    uint8_t bits_delay[40] = {0};
    uint8_t bytes[5] = {0};

    // zahájení komunikace
    gpio_set_dir(MY_DHT_PIN, GPIO_OUT);
    gpio_put(MY_DHT_PIN, 0);
    busy_wait_ms(18);
    gpio_put(MY_DHT_PIN, 1);
    busy_wait_us_32(30);
    gpio_set_dir(MY_DHT_PIN, GPIO_IN);

    //kontrola zda zařízení odpovídá
    // 80 us LOW -> 80 us HIGH
    busy_wait_us_32(40);
    if(gpio_get(MY_DHT_PIN)) {
        debug("Not responding! (LOW)", "");
        return 1;
    }
    busy_wait_us_32(80);
    if(!gpio_get(MY_DHT_PIN)) {
        debug("Not responding! (HIGH)", "");
        return 1;
    }
    busy_wait_us_32(40);

    //čtení zprávy
    int bit = 0;
    for(; bit < 40; bit++) {
        timeout = MY_DHT_MAX_TIMINGS;
        cnt = 0;
        //čekání, mělo by to být 50us od konce předchozího bitu
        //tohle čekání lze vypustit a přidat 50 us ke kontrola času na konci
        while(timeout > 0 && !gpio_get(MY_DHT_PIN)) {
            timeout--;
            busy_wait_us_32(1);
        }
        if(timeout <= 0) {
            debug("Device timeout on rise edge! bit %d", bit);
            return 1;
        }
        timeout = MY_DHT_MAX_TIMINGS;
        //timeout lze vyměnit za kontrolu cnt pokud je menší < MY_DHT_MAX_TIMINGS
        while(timeout > 0 && gpio_get(MY_DHT_PIN)) {
            cnt++;
            timeout--;
            busy_wait_us_32(1);
        }
        if(timeout <= 0) {
            debug("Device timeout on fall edge! bit %d", bit);
            return 1;
        }
        //printf("%llu", to_us_since_boot(get_absolute_time())-real_time);
        //printf(": counted %u us\n", cnt);
        //real_time = to_ms_since_boot(get_absolute_time());
        bits_delay[bit] = cnt;
    }

    //kontrola délky
    if(bit < 40) {
        debug("Message too short!", "");
        return 1;
    }

    //MSB first
    //BYTE 1 REL HUMIDITY INT
    //BYTE 2 REL HUMIDITY DEC
    //BYTE 3 TMP          INT
    //BYTE 4 TMP          DEC
    //BYTE 5 CHECKSUM == (BYTE1+BYTE2+BYTE3+BYTE4) & 0xFF

    //převedení seznamu bitů do seznamu bytů
    //lze změnit na zápis bytů přímo ve smyčce čtení ze senzoru
    for(int i = 0; i < 40; i++) {
        // prinktf("%u", (bits_delay[i] > 30));
        // if(i && !((i+1)%8)) printf(" ");
        bytes[i/8] <<= 1;
        if(bits_delay[i] > 30) bytes[i/8] |= 1;
        // if(i && !((i+1)%8)) printf("0x%02x %u | ", bytes[i/8], bytes[i/8]);
    }
    // printf("\n");

    //kontrola checksum
    if(bytes[4] != ((bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 0xFF)) {
        debug("Wrong checksum!", "");
        return 1;
    }

    result->humidity = (float) ((bytes[0] << 8) + bytes[1]) / 10;
    // printf("%u %u %f\n", bytes[0] << 8, (bytes[0] << 8) + bytes[1], result->humidity);
    if (result->humidity > 100) {
        result->humidity = bytes[0];
    }
    result->temp_celsius = (float) (((bytes[2] & 0x7F) << 8) + bytes[3]) / 10;
    // printf("%f\n", result->temp_celsius);
    if (result->temp_celsius > 125) {
        result->temp_celsius = bytes[2];
    }
    if (bytes[2] & 0x80) {
        result->temp_celsius = -result->temp_celsius;
    }
    return 0;
}
