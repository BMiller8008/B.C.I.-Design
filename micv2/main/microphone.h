#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

// FIR filter configuration
#define FIR_TAP_NUM 64

class Microphone {
public:
    Microphone(adc_channel_t pin, int sampleRate);
    void start();

private:
    adc_oneshot_unit_handle_t adcHandle;
    adc_channel_t micPin;
    int sampleRateMs;

    static void taskEntry(void *param);
    void readADC();
    int applyFilter(int raw_value);
    int convertToPCM(int raw_value);
};

#endif // MICROPHONE_H
