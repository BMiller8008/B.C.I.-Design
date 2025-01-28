#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <string>

extern "C" {
    // ESP-IDF headers required for Wi-Fi and system functionality
    #include <freertos/FreeRTOS.h>
    #include <freertos/event_groups.h>
    #include <esp_wifi.h>
    #include <esp_event.h>
    #include <esp_log.h>
}

/**
 * @brief A class to manage Wi-Fi connections and provide extensibility for additional features.
 * 
 * This class handles connecting to open and WPA2 networks and includes virtual methods for
 * further customization, such as handling API calls and received data.
 */
class WiFiManager {
public:
    // Constructor: Sets up the Wi-Fi manager and initializes components
    WiFiManager();

    // Destructor: Cleans up resources
    virtual ~WiFiManager();

    /**
     * @brief Connect to an open Wi-Fi network.
     * @param ssid The SSID (name) of the Wi-Fi network.
     */
    void connectToOpenNetwork(const std::string& ssid);

    /**
     * @brief Connect to a WPA2-protected Wi-Fi network.
     * @param ssid The SSID (name) of the Wi-Fi network.
     * @param password The password for the Wi-Fi network.
     */
    void connectToWPA2Network(const std::string& ssid, const std::string& password);

    /**
     * @brief method called when Wi-Fi is successfully connected.
     * @param ipAddress The IP address assigned to the device.
     */
    void onWiFiConnected(void* arg);

    /**
     * @brief Prints the network info
     * @param ipAddress The IP address assigned to the device.
     */
    void print_network_info();

protected:
    /**
     * @brief Initialize Wi-Fi settings and register event handlers.
     */
    void initWiFi();

    // Event group for Wi-Fi events (e.g., connection success or failure)
    EventGroupHandle_t wifiEventGroup;

    // Flags for connection status
    static const int WIFI_CONNECTED_BIT = BIT0;
    static const int WIFI_FAIL_BIT = BIT1;

    // Wi-Fi event handler (static so it can be used with ESP-IDF)
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    // Retry counter and maximum retries
    int retryCount;
    static const int maxRetries = 5;

    // IP address of the connected device
    std::string ipAddress;
};

#endif // WIFI_MANAGER_H
