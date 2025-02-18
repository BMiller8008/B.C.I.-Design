#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"

// ADC channel and width configuration
#define ADC_CHANNEL ADC1_CHANNEL_0
#define ADC_WIDTH ADC_WIDTH_BIT_12 
#define ADC_ATTEN ADC_ATTEN_DB_11  

void adc_task(void *arg) {
    while (true) {
        int raw_value = adc1_get_raw(ADC_CHANNEL); 
        float voltage = (raw_value / 4095.0) * 3.3; // Convert to voltage
        // printf("Raw ADC Value: %d, Voltage: %.2fV\n", raw_value, voltage);

        //upload to an object so that it can be getted by oled
        if (voltage > 2.78) {
            printf("100%%\n");
        }
        else if (voltage > 2.26) {
            printf("75%%\n");
        }
        else if (voltage > 1.73) {
            printf("50%%\n");
        }
        else if (voltage > 1.21) {
            printf("Low Battery!\n");
            printf("25%%\n");
        }
        else {
            printf("Shutting Down\n");
            printf("0%%\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));  // Read every 500ms
    }
}

extern "C" void app_main() {
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    xTaskCreate(adc_task, "ADC Reader", 4096, NULL, 1, NULL);
}