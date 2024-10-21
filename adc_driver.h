#ifndef ADC_H
#define ADC_H

/* reading interval in ms*/
#define READING_INTERVAL 50

//typedef adc_driver_data adc_data; 

struct adc_driver_data{
    uint cntr;
    uint adc_input;
    uint sample_number;
    uint16_t value;
    uint16_t avg;
};

typedef struct adc_driver_data adc_data;

/**
 * 
 */
uint16_t my_adc_read(uint input);

/**
 * 
 */
int adc_driver_init(adc_data *data);

#endif /* ADC_H */