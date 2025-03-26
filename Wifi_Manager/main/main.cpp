#include "WiFiManager.h"

#define BUFFER_SIZE 32 * sizeof(int)

int audioBuffer[32] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

WiFiManager* wifiManager;

extern "C" void app_main() {
    wifiManager = new WiFiManager("192.168.1.3", 8080, 8081, 8088);
    wifiManager->connectToOpenNetwork("NETGEAR41");
    // wifiManager->connectToWPA2Network("NETGEAR41", "bravemango923");

    // Starting Data Tasks


    // Polling Receive and sending periodically
    while (true) {
        if (wifiManager->hasNewMessage()) {
            ESP_LOGI("MAIN", "Received: %s", wifiManager->getReceivedMessage());
        }
        vTaskDelay(pdMS_TO_TICKS(1500));
        wifiManager->sendRawData(audioBuffer, 32);
        wifiManager->sendCommandToServer("BOOBIES_RULE");
    }
}