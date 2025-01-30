#include "WiFiManager.h"
#include <cstring>
#include "nvs_flash.h"

// Constructor: Initialize retry count and create the event group
WiFiManager::WiFiManager(const std::string& ip, uint8_t* buffer, size_t bufferSize, int port) : retryCount(0), targetAddress(ip), 
target_port(port), audioBuffer(buffer), bufferSize(bufferSize) {
    wifiEventGroup = xEventGroupCreate(); // Create event group for Wi-Fi events
    esp_log_level_set("wifi", ESP_LOG_ERROR); // Set Lower Logging Levels

    // Printing
    ESP_LOGI("wifi-startup", "Initializing WifiManager");
    initWiFi(); // Set up Wi-Fi
}

// Destructor: Clean up resources
WiFiManager::~WiFiManager() {
    ESP_LOGI("wifi-startup", "Deinitializing WifiManager");
    esp_wifi_stop();
    esp_wifi_deinit();
    vEventGroupDelete(wifiEventGroup); // Delete the event group
}

void WiFiManager::initWiFi() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_LOGI("wifi-startup", "Initializing NVS");
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();  // Erase and reinitialize NVS
        nvs_flash_init();
    }

    // Initialize network interfaces
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default Wi-Fi station interface
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi with default settings
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers for Wi-Fi and IP events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiManager::eventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFiManager::eventHandler, this, nullptr));


}

// Connect to an open Wi-Fi network
void WiFiManager::connectToOpenNetwork(const std::string& ssid) {
    wifi_config_t wifi_config = {};
    std::strcpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid.c_str());
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    // Configure and start Wi-Fi
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    // Wait for connection or failure
    xEventGroupWaitBits(wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
}

// Connect to a WPA2 network
void WiFiManager::connectToWPA2Network(const std::string& ssid, const std::string& password) {
    wifi_config_t wifi_config = {};
    std::strcpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid.c_str());
    std::strcpy(reinterpret_cast<char*>(wifi_config.sta.password), password.c_str());
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    // Configure and start Wi-Fi
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    // Wait for connection or failure
    xEventGroupWaitBits(wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
}

// Event handler for Wi-Fi and IP events
void WiFiManager::eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    auto* self = static_cast<WiFiManager*>(arg);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // Attempt to connect
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (self->retryCount < self->maxRetries) {
            esp_wifi_connect(); // Retry connection
            self->retryCount++;
        } else {
            xEventGroupSetBits(self->wifiEventGroup, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(self->wifiEventGroup, WIFI_CONNECTED_BIT);
        // Call Connected Function
        self->onWiFiConnected(arg);
    }
}

void WiFiManager::onWiFiConnected(void* arg) {
    // calling print network stats
    ESP_LOGI("wifi-startup", "Wifi Connected");
    auto* self = static_cast<WiFiManager*>(arg);
    self->print_network_info();
}

// Prints IP and Stats
void WiFiManager::print_network_info() {
    // Get network information
    esp_netif_ip_info_t ipInfo;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == nullptr) {
        ESP_LOGE("WiFiManager", "Failed to get network interface handle!");
        return;
    }

    if (esp_netif_get_ip_info(netif, &ipInfo) != ESP_OK) {
        ESP_LOGE("WiFiManager", "Failed to retrieve IP information!");
        return;
    }

    // Convert IP, Gateway, and Netmask to strings
    char ipStr[16], gatewayStr[16], netmaskStr[16];
    esp_ip4addr_ntoa(&ipInfo.ip, ipStr, sizeof(ipStr));
    esp_ip4addr_ntoa(&ipInfo.gw, gatewayStr, sizeof(gatewayStr));
    esp_ip4addr_ntoa(&ipInfo.netmask, netmaskStr, sizeof(netmaskStr));

    // Retrieve DNS information
    esp_netif_dns_info_t dnsInfo;
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dnsInfo);
    char dnsStr[16];
    esp_ip4addr_ntoa(&dnsInfo.ip.u_addr.ip4, dnsStr, sizeof(dnsStr));

    // Retrieve MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Get SSID of the connected network
    wifi_ap_record_t apInfo;
    esp_wifi_sta_get_ap_info(&apInfo);

    // Print network information to the ESP monitor
    ESP_LOGI("WiFiManager", "\n\n==========================================");
    ESP_LOGI("WiFiManager", "Network Information:");
    ESP_LOGI("WiFiManager", "  IP Address: %s", ipStr);
    ESP_LOGI("WiFiManager", "  Gateway: %s", gatewayStr);
    ESP_LOGI("WiFiManager", "  Netmask: %s", netmaskStr);
    ESP_LOGI("WiFiManager", "  DNS Server: %s", dnsStr);
    ESP_LOGI("WiFiManager", "  MAC Address: %s", macStr);
    ESP_LOGI("WiFiManager", "  Connected SSID: %s", apInfo.ssid);
    ESP_LOGI("WiFiManager", "  Signal Strength (RSSI): %d dBm", apInfo.rssi);
    ESP_LOGI("WiFiManager", "\n==========================================\n\n");
}

void WiFiManager::startDataSenderTask() {
    ESP_LOGI("WiFiManager", "Starting DataSender");
    xTaskCreate(dataSenderTask, "DataSenderTask", 4096, this, 5, NULL);
}

void WiFiManager::dataSenderTask(void* arg) {
    WiFiManager* instance = static_cast<WiFiManager*>(arg);

    if (instance->audioBuffer == nullptr) {
        ESP_LOGE("WiFiManager", "Audio buffer is NULL!");
        vTaskDelete(NULL);
    }

    ESP_LOGI("WiFiManager", "DataSender Task Running...");

    int port = instance->target_port;

    while (true) {
        ESP_LOGI("WiFiManager", "DataSender Task LOOP, bufferIndex = %d", instance->bufferIndex);

        // Ensure buffer has data
        if (instance->bufferIndex > 0) {
            int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock < 0) {
                ESP_LOGE("WiFiManager", "Socket creation failed! Error: %d", errno);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            struct sockaddr_in destAddr;
            destAddr.sin_family = AF_INET;
            destAddr.sin_port = htons(port);

            if (inet_pton(AF_INET, instance->targetAddress.c_str(), &destAddr.sin_addr) <= 0) {
                ESP_LOGE("WiFiManager", "Invalid IP address: %s", instance->targetAddress.c_str());
                close(sock);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            // Correctly handle `int` array size
            size_t bytesToSend = instance->bufferIndex * sizeof(int);

            ESP_LOGI("WiFiManager", "Sending %d bytes to %s:%d", bytesToSend, instance->targetAddress.c_str(), port);

            int sentBytes = sendto(sock, instance->audioBuffer, bytesToSend, 0, 
                                   (struct sockaddr*)&destAddr, sizeof(destAddr));

            if (sentBytes < 0) {
                ESP_LOGE("WiFiManager", "Failed to send data! Error: %d", errno);
            } else {
                ESP_LOGI("WiFiManager", "Sent %d bytes", sentBytes);
            }

            close(sock);
            ESP_LOGI("WiFiManager", "Finished sending data.");

            instance->bufferIndex = 0;  // Reset buffer after sending
        }

        vTaskDelay(pdMS_TO_TICKS(500));  // Prevent CPU overuse
    }
}

void WiFiManager::addDataToBuffer(const int* data, size_t length) {
    if (length > bufferSize / sizeof(int)) {
        ESP_LOGE("WiFiManager", "Data too large for buffer!");
        return;
    }

    memcpy(audioBuffer, data, length * sizeof(int));
    bufferIndex = length;  // Set bufferIndex to actual data length
    ESP_LOGI("WiFiManager", "Added %d ints to buffer", bufferIndex);
}
