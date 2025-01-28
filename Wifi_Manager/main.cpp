#include "WiFiManager.h"
#include <iostream>

/**
 * @brief Custom class inheriting from WiFiManager.
 * 
 * Demonstrates overriding virtual methods for handling Wi-Fi and API functionality.
 */


// Entry point for the application
extern "C" void app_main() {
    WiFiManager app;
    app.connectToOpenNetwork("Campus Edge Guest");
    // For WPA2: app.connectToWPA2Network("MySSID", "MyPassword");
}
