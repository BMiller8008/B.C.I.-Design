#include <stdio.h>
#include "mainApp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "WiFiManager.h"
#include "esp_timer.h" 
#include "driver/adc.h"
#include "OLED_Driver.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include "fonts.h"

/* Audio Contants */
#define BUFFER_SIZE 1024  // More efficient to send large packets
#define SAMPLE_RATE 8000
#define ADC1_CHANNEL ADC1_CHANNEL_6  // GPIO34
static const char *TAG = "MAIN";

int audioBuffer[BUFFER_SIZE] = {0};

/* CLASS OBJECTS */
WiFiManager* wifiManager;
MainApp* app;
OLED_Display* oled;

// Task to stream audio
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

// Polls the receive buffer
void wifi_display_task(void* pvParameters) {
    while (true) {
        if (wifiManager->hasNewMessage() && app->getState() == MainApp::State::ON) {
            const char* msg = wifiManager->getReceivedMessage();
            ESP_LOGI(TAG, "Received: %s", msg);

            oled->clear_buffer();
            oled->drawText(0, 0, msg, &Font24, BLACK, WHITE);
            oled->display();
        }

        vTaskDelay(pdMS_TO_TICKS(500));  // Check every 500ms
    }
}


extern "C" void app_main(void) {
    // Inititializing WIFI
    wifiManager = new WiFiManager("192.168.1.4", 8080, 8081, 8088);
    wifiManager->connectToOpenNetwork("NETGEAR41");

    // Initializing Mic ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL, ADC_ATTEN_DB_11);

    // Initializing OLED
    oled = new OLED_Display();
    oled->init();
    oled->clear();

    // Starting Audio Transmission Task
    ESP_LOGI(TAG, "Starting Audio Streaming Task...");
    xTaskCreate(audio_stream_task, "AudioStreamTask", 4096, NULL, 5, NULL);

    // Creating main app
    app = new MainApp();

    // Starting WiFi Message Display Task
    ESP_LOGI(TAG, "Starting Wi-Fi Display Task...");
    xTaskCreate(wifi_display_task, "WiFiDisplayTask", 4096, NULL, 4, NULL);

    printf("Initial state: %s\n", app->getState() == MainApp::State::ON ? "ON" : "OFF");
    printf("Current language: %s\n", app->getLanguage().c_str());
    printf("Current font size: %s\n", app->getFontSize().c_str());

    app->printAvailableLanguages();
    app->printAvailableFontSizes();

    // Change language and font size
    app->setLanguage("French");
    app->setFontSize("12");
    printf("\nUpdated language: %s\n", app->getLanguage().c_str());
    printf("Updated font size: %s\n", app->getFontSize().c_str());

    // Toggle LEDs
    printf("Turning on LED1 and LED2...\n");
    app->set_led1(1);
    app->set_led2(1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Turning off LED1 and LED2...\n");
    app->set_led1(0);
    app->set_led2(0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Done.\n");

    std::vector<std::string> langSlice = app->getWrappedSlice(app->getAvailableLanguages(), 5, 3);
    printf("Wrapped language slice:\n");
    for (const auto& lang : langSlice) {
        printf(" - %s\n", lang.c_str());
    }

}
