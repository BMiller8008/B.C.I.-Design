// #include "driver/adc.h"
// #include "esp_system.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// #define MIC_PIN         ADC1_CHANNEL_6   // GPIO 34 (Analog input)
// #define TAG             "MAX4466_MIC"
// #define SAMPLE_RATE_MS  10               // Sampling delay (adjustable)

// void app_main() {
//     esp_log_level_set("*", ESP_LOG_INFO);  // Enable logging

//     // ADC Configuration
//     adc1_config_width(ADC_WIDTH_BIT_12);             // 12-bit resolution (0-4095)
//     adc1_config_channel_atten(MIC_PIN, ADC_ATTEN_DB_11); // Full-scale voltage: 3.3V

//     ESP_LOGI(TAG, "MAX4466 Microphone Initialized!");

//     while (1) {
//         int raw_value = adc1_get_raw(MIC_PIN);       // Read analog value
//         // ESP_LOGI(TAG, "Audio Sample: %d", raw_value); // Log the ADC value
//         printf("%d\n", raw_value);

//         vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE_MS));   // Control the sampling rate
//     }
// }

#include "microphone.h"

// Define Microphone globally so it persists throughout the execution
Microphone mic(ADC_CHANNEL_6, 10);  // GPIO 34, 10ms sample rate (dont use sample rate anymore)

extern "C" void app_main() {
    mic.start();  // Start the ADC sampling task
}


// #include "microphone.h"
// extern "C" void app_main() {
//     static Microphone mic(ADC_CHANNEL_6);  // GPIO 34
//     mic.start();
// }
