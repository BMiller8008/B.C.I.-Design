#include "WiFiManager.h"

#define BUFFER_SIZE 16 * sizeof(int)  // ✅ Correct size for int array

int audioBuffer[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

WiFiManager* wifiManager;

extern "C" void app_main() {
    wifiManager = new WiFiManager("192.168.2.4", (uint8_t*)audioBuffer, BUFFER_SIZE, 8080);
    wifiManager->connectToOpenNetwork("belkin54g");

    // ✅ Manually add data to buffer before starting sender task
    int testData[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    wifiManager->addDataToBuffer(testData, 16);

    wifiManager->startDataSenderTask();
}
