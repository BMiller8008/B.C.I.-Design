#include "WiFiManager.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUFFER_SIZE 1024  // More efficient to send large packets
#define SAMPLE_RATE 8000
#define ADC1_CHANNEL ADC1_CHANNEL_6  // GPIO34

WiFiManager* wifiManager;
int audioBuffer[BUFFER_SIZE] = {0};

static const char *TAG = "MAIN";

// Task to collect ADC samples and send over UDP
#include "esp_timer.h"  // ✅ Add this

void audio_stream_task(void *pvParameters) {
    ESP_LOGI(TAG, "Audio streaming task started!");

    while (true) {
        int64_t start_time = esp_timer_get_time();  // Get start timestamp

        for (int i = 0; i < BUFFER_SIZE; i++) {
            audioBuffer[i] = adc1_get_raw(ADC1_CHANNEL);
        }

        wifiManager->sendRawData(audioBuffer, BUFFER_SIZE);
        ESP_LOGI(TAG, "Sent %d samples", BUFFER_SIZE);

        // Adjust delay to maintain an 8kHz sampling rate
        int64_t elapsed_time = esp_timer_get_time() - start_time;
        int64_t target_delay = (1000000 / SAMPLE_RATE) * BUFFER_SIZE;  // Microseconds
        int64_t sleep_time = target_delay - elapsed_time;

        if (sleep_time > 0) {
            vTaskDelay(pdMS_TO_TICKS(sleep_time / 1000));  // Convert to ms
        }
    }
}

// void audio_stream_task(void *pvParameters) {
//     ESP_LOGI(TAG, "Audio streaming task started!");

//     while (true) {
//         for (int i = 0; i < BUFFER_SIZE; i++) {
//             audioBuffer[i] = adc1_get_raw(ADC1_CHANNEL);
//         }

//         wifiManager->sendRawData(audioBuffer, BUFFER_SIZE);
//         ESP_LOGI(TAG, "Sent %d samples", BUFFER_SIZE);

//         vTaskDelay(pdMS_TO_TICKS(100));  // Prevent spamming
//     }
// }

extern "C" void app_main() {
    ESP_LOGI(TAG, "Initializing Wi-Fi Manager...");
    wifiManager = new WiFiManager("192.168.1.3", (uint8_t*)audioBuffer, BUFFER_SIZE * sizeof(int), 8080, 8081);
    wifiManager->connectToOpenNetwork("NETGEAR41");

    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL, ADC_ATTEN_DB_11);

    vTaskDelay(pdMS_TO_TICKS(3000));  // Ensure Wi-Fi is connected

    ESP_LOGI(TAG, "Starting Audio Streaming Task...");
    xTaskCreate(audio_stream_task, "AudioStreamTask", 4096, NULL, 5, NULL);
}
