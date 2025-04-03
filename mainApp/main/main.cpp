#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "WiFiManager.h"
#include "OLED_Driver.h"
#include "mainApp.h"
#include "fonts.h"

static const char* TAG = "MAIN";

// ----- Audio Config -----
#define SAMPLE_RATE        8000
#define CHUNK_SIZE         1024
#define RING_BUFFER_SIZE   4096
#define ADC1_CHANNEL       ADC1_CHANNEL_6 // GPIO34

// ----- Globals -----
static int* ringBuffer = nullptr;
static volatile int writeIndex = 0;
static volatile int readIndex  = 0;

static WiFiManager* wifiManager = nullptr;
static OLED_Display* oled = nullptr;
static MainApp* app = nullptr;
static esp_timer_handle_t samplingTimer = nullptr;

// ----- For State Tracking -----
enum AppState {
    STATE_MAIN,
    STATE_FONT,
    STATE_LANG,
    STATE_OFF
};


// ----- Timer Callback -----
static void IRAM_ATTR audioSamplingCallback(void* arg)
{
    int nextWrite = (writeIndex + 1) % RING_BUFFER_SIZE;
    if (nextWrite == readIndex) {
        readIndex = (readIndex + 1) % RING_BUFFER_SIZE;
    }
    ringBuffer[writeIndex] = adc1_get_raw(ADC1_CHANNEL);
    writeIndex = nextWrite;
}

// ----- Audio Stream Task -----
static void audio_stream_task(void* pvParameters)
{
    ESP_LOGI(TAG, "Audio streaming task started");
    int localBuffer[CHUNK_SIZE];

    while (true) {
        int available = (writeIndex - readIndex + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
        if (available >= CHUNK_SIZE) {
            for (int i = 0; i < CHUNK_SIZE; i++) {
                localBuffer[i] = ringBuffer[readIndex];
                readIndex = (readIndex + 1) % RING_BUFFER_SIZE;
            }

            // TODO: Apply FIR or IIR filter here if needed

            wifiManager->sendRawData(localBuffer, CHUNK_SIZE);
            // ESP_LOGI(TAG, "Sent %d samples", CHUNK_SIZE);
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

// ----- OLED Display Message Task -----
static void wifi_display_task(void* pvParameters) {
    while (true) {
        if (wifiManager->hasNewMessage()) {
            const char* msg = wifiManager->getReceivedMessage();
            ESP_LOGI(TAG, "Receive message!!");
            oled->clear_buffer();
            oled->drawText(10, 10, msg, &Font16, BLACK, WHITE);
            oled->display();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ----- Setup Helpers -----
static void initWiFi() {
    ESP_LOGI(TAG, "Initializing Wi-Fi Manager...");
    wifiManager = new WiFiManager("192.168.1.5", 8080, 8081, 8088);
    wifiManager->connectToOpenNetwork("NETGEAR41");
}

static void configADC() {
    ESP_LOGI(TAG, "Configuring ADC...");
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL, ADC_ATTEN_DB_11);
}

static void startAudioSamplingTimer() {
    ESP_LOGI(TAG, "Creating ADC sampling timer...");
    esp_timer_create_args_t timerArgs = {
        .callback = &audioSamplingCallback,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ADC Sampling Timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &samplingTimer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(samplingTimer, 1000000 / SAMPLE_RATE));
    ESP_LOGI(TAG, "ADC sampling timer started at %d Hz", SAMPLE_RATE);
}

// ----- Main Entry -----
extern "C" void app_main() {
    // 1. Allocate ring buffer
    ringBuffer = (int*)heap_caps_malloc(RING_BUFFER_SIZE * sizeof(int), MALLOC_CAP_INTERNAL);
    if (!ringBuffer) {
        ESP_LOGE(TAG, "Failed to allocate ring buffer!");
        return;
    }

    // 2. Setup components
    initWiFi();
    configADC();

    oled = new OLED_Display();
    oled->init();
    oled->clear();
    oled->drawText(10, 40, "ECE 477", &Font16, BLACK, WHITE);
    oled->display();
    vTaskDelay(pdMS_TO_TICKS(1000));

    app = new MainApp();
    AppState state = STATE_OFF; // setting state of menus

    // 3. Start tasks
    xTaskCreate(audio_stream_task, "AudioStreamTask", 8192, nullptr, 5, nullptr);
    xTaskCreate(wifi_display_task, "WiFiDisplayTask", 4096, nullptr, 4, nullptr);
    startAudioSamplingTimer();

    // 4. MainApp info and LED blink
    printf("Initial state: %s\n", app->getState() == MainApp::State::ON ? "ON" : "OFF");
    app->printAvailableLanguages();
    app->printAvailableFontSizes();

    app->setLanguage("French");
    app->setFontSize("12");

    // while (true)
    // {
    //     app->displayMainMenu(oled);
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    //     for (size_t i = 0; i < 7; i++)
    //     {
    //         vTaskDelay(pdMS_TO_TICKS(1000));
    //         app->setMenuIdx(i);
    //         app->displayLangChoice(oled);
    //     }
    // }
}
