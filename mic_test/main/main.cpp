#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/adc.h"
#include "esp_log.h"

#include "WiFiManager.h"

static const char* TAG = "MAIN";

// ---------- Basic Config ----------
#define SAMPLE_RATE        8000
#define CHUNK_SIZE         1024
#define RING_BUFFER_SIZE   4096
#define ADC1_CHANNEL       ADC1_CHANNEL_6 // GPIO34

// ---------- The Rest of Your Code ----------

// Globals
static int* ringBuffer = nullptr; 
static volatile int writeIndex = 0;
static volatile int readIndex  = 0;
static WiFiManager* wifiManager = nullptr;
static esp_timer_handle_t samplingTimer = nullptr;

// Timer callback
static void IRAM_ATTR audioSamplingCallback(void* arg)
{
    int nextWrite = (writeIndex + 1) % RING_BUFFER_SIZE;
    if (nextWrite == readIndex) {
        // Overrun => discard oldest
        readIndex = (readIndex + 1) % RING_BUFFER_SIZE;
    }
    ringBuffer[writeIndex] = adc1_get_raw(ADC1_CHANNEL);
    writeIndex = nextWrite;
}

// Audio streaming task
static void audio_stream_task(void* pvParameters)
{
    ESP_LOGI(TAG, "Audio streaming task started");

    int localBuffer[CHUNK_SIZE];

    while (true) {
        int available = (writeIndex - readIndex + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
        if (available >= CHUNK_SIZE) {
            // Collect chunk
            for (int i = 0; i < CHUNK_SIZE; i++) {
                localBuffer[i] = ringBuffer[readIndex];
                readIndex = (readIndex + 1) % RING_BUFFER_SIZE;
            }

            // **** APPLY IIR FILTERS HERE ****
            // applyAudioFilters(localBuffer, CHUNK_SIZE);

            // Send filtered audio
            wifiManager->sendRawData(localBuffer, CHUNK_SIZE);
            ESP_LOGI(TAG, "Sent %d filtered samples", CHUNK_SIZE);
        } else {
            // Not enough data, yield
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

// Helpers
static void initWiFi()
{
    ESP_LOGI(TAG, "Initializing Wi-Fi Manager...");
    wifiManager = new WiFiManager("192.168.2.2", nullptr, 0, 8080, 8081);
    wifiManager->connectToOpenNetwork("belkin54g");
}

static void configADC()
{
    ESP_LOGI(TAG, "Configuring ADC...");
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL, ADC_ATTEN_DB_11);
}

static void startAudioSamplingTimer()
{
    ESP_LOGI(TAG, "Creating ADC sampling timer...");

    esp_timer_create_args_t timerArgs = {
        .callback        = &audioSamplingCallback,
        .arg             = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "ADC Sampling Timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &samplingTimer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(samplingTimer, 1000000 / SAMPLE_RATE));

    ESP_LOGI(TAG, "ADC sampling timer started at %d Hz", SAMPLE_RATE);
}

static void startAudioStreamingTask()
{
    ESP_LOGI(TAG, "Starting audio streaming task...");
    xTaskCreate(audio_stream_task, "AudioStreamTask", 8192, nullptr, 5, nullptr);
}

// ---------- Main Entry ----------
extern "C" void app_main()
{
    // Allocate ring buffer
    ringBuffer = (int*)heap_caps_malloc(RING_BUFFER_SIZE * sizeof(int), MALLOC_CAP_DEFAULT);
    if (!ringBuffer) {
        ESP_LOGE(TAG, "Failed to allocate ring buffer!");
        return;
    }

    // 1) Wi-Fi
    initWiFi();

    // 2) ADC
    configADC();

    // 3) Wait a moment for Wi-Fi
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 5) Start sampling & streaming
    startAudioSamplingTimer();
    startAudioStreamingTask();
}
