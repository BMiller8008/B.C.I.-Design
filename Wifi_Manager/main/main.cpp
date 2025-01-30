#include "WiFiManager.h"

#define BUFFER_SIZE 32 * sizeof(int)

int audioBuffer[32] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

WiFiManager* wifiManager;

extern "C" void app_main() {
    wifiManager = new WiFiManager("192.168.2.4", (uint8_t*)audioBuffer, BUFFER_SIZE, 8080, 8081);
    wifiManager->connectToOpenNetwork("belkin54g");

    // Manually add data to buffer before starting sender task
    wifiManager->addDataToBuffer(audioBuffer, 32);

    // Starting Data Tasks
    wifiManager->startDataSenderTask();
    wifiManager->startDataReceiverTask();

    // Polling Receive
    while (true) {
        if (wifiManager->hasNewMessage()) {
            ESP_LOGI("MAIN", "Received: %s", wifiManager->getReceivedMessage());
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}