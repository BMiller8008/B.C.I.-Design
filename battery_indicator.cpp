#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"

// ADC channel and width configuration
#define ADC_CHANNEL ADC1_CHANNEL_0
#define ADC_WIDTH ADC_WIDTH_BIT_12 
#define ADC_ATTEN ADC_ATTEN_DB_11  

class BatteryMonitor {
public:
    BatteryMonitor(adc1_channel_t channel) : channel_(channel) {
        adc1_config_width(ADC_WIDTH);
        adc1_config_channel_atten(channel_, ADC_ATTEN);
    }

    float readVoltage() {
        int raw_value = adc1_get_raw(channel_);
        return (raw_value / 4095.0) * 3.3;
    }

    void printBatteryStatus() {
        float voltage = readVoltage();
        if (voltage > 2.78) {
            printf("100%%\n");
        } else if (voltage > 2.26) {
            printf("75%%\n");
        } else if (voltage > 1.73) {
            printf("50%%\n");
        } else if (voltage > 1.21) {
            printf("Low Battery!\n25%%\n");
        } else {
            printf("Shutting Down\n0%%\n");
        }
    }

private:
    adc1_channel_t channel_;
};

void adc_task(void *arg) {
    BatteryMonitor *monitor = static_cast<BatteryMonitor*>(arg);
    
    while (true) {
        monitor->printBatteryStatus();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

extern "C" void app_main() {
    static BatteryMonitor monitor(ADC_CHANNEL);

    xTaskCreate(adc_task, "ADC Reader", 4096, &monitor, 1, NULL);
}