#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
// #include <lwip/sockets.h>
#include <esp_heap_caps.h>
#include <cstring>

// For audio chunks 
#define CHUNK_SIZE 1
#define RECEIVE_BUFFER 128

/**
 * @brief A class to manage Wi-Fi connections and provide extensibility for additional features.
 * 
 * This class handles connecting to open and WPA2 networks and includes virtual methods for
 * further customization, such as handling API calls and received data.
 */
class WiFiManager {
public:
    /**
     * @brief Contructor
     * @param server_ip the ip of the server
     */
    WiFiManager(const std::string& server_ip, int serv_port, int recvPort, int cmdPort);

    /**
     * @brief Destructor
     */
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
     */
    void onWiFiConnected(void* arg);

    /**
     * @brief Prints the network info
     */
    void print_network_info();


    /**
     * @brief getter function to check if the receive buffer has new data
     */
    bool hasNewMessage();

    /**
     * @brief getter function to pull recieve buffer
     */
    const char* getReceivedMessage();

    /**
     * @brief sends an int array to the server
     */
    void sendRawData(const int* data, size_t length);

    /**
     * @brief starts receiving software
     */
    void startDataReceiverTask();

    /**
     * @brief sends command packet via tcp
     */
    void sendCommandToServer(const std::string& message);



protected:
    /**
     * @brief Initialize Wi-Fi settings and register event handlers.
     */
    void initWiFi();

    /**
     * @brief Starts TCP Receiving task
     */
    static void dataReceiverTask(void* arg);

    /**
     * @brief Task to handle connecting to wifi
     */
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    // Event group for Wi-Fi events (e.g., connection success or failure)
    EventGroupHandle_t wifiEventGroup;

    // Flags for connection status
    static const int WIFI_CONNECTED_BIT = BIT0;
    static const int WIFI_FAIL_BIT = BIT1;

    // Retry counter and maximum retries
    int retryCount;
    static const int maxRetries = 5;

    // IP address of the connected device
    std::string ipAddress;

    // Target IP Address of server
    std::string targetAddress;

    // Target Port for server (audio)
    int target_port;

    // Target Port for ESP32
    int recvPort;

    // Target port for server commands
    int cmdPort;

    // For receive Buffer
    char receivedMessage[RECEIVE_BUFFER];  // Stores the latest message
    bool messageAvailable = false;  // Flag to indicate new message
    SemaphoreHandle_t messageLock;  // Mutex to protect access
};

#endif // WIFI_MANAGER_H
