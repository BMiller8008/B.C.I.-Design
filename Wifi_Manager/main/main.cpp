#include "WiFiManager.h"

#define BUFFER_SIZE 16  // Example buffer size (small for simplicity)

// Example data buffer
uint8_t audioBuffer[BUFFER_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

WiFiManager wifiManager("192.168.1.100", audioBuffer, BUFFER_SIZE);

extern "C" void app_main() {
    wifiManager.connectToOpenNetwork("Campus Edge Guest");

    // Start transmission task
    // wifiManager.startDataSenderTask();
}
