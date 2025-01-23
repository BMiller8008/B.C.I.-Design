#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "TimerApp";

class TimerApp {
public:
    void start() {
        ESP_LOGI(TAG, "ESP32 Timer Program Starting...");
        xTaskCreate(&TimerApp::timerTaskA, "timer_task_A", 4096, this, 5, nullptr);
        xTaskCreate(&TimerApp::timerTaskB, "timer_task_B", 4096, this, 5, nullptr);
    }

private:
    static void timerTaskA(void *pvParameters) {
        uint seconds = 0;
        while (true) {
            ESP_LOGI("TASK A", "Seconds passed: %u", seconds);
            seconds++;
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
        }
    }

    static void timerTaskB(void *pvParameters) {
        uint seconds = 0;
        while (true) {
            ESP_LOGI("TASK B", "Pair of Seconds passed: %u", seconds);
            seconds++;
            vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 1 second
        }
    }
    
};

extern "C" void app_main() {
    TimerApp app;
    app.start();
}
