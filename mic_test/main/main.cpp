#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/adc.h"
#include "esp_log.h"

#include "WiFiManager.h"

// ---------- Configuration Constants ----------
static const char* TAG = "MAIN";
#define SAMPLE_RATE        8000
#define CHUNK_SIZE         1024
#define RING_BUFFER_SIZE   4096   // Reduced from 8192 or more to conserve memory
#define ADC1_CHANNEL       ADC1_CHANNEL_6 // GPIO34

// ---------- Global Variables ----------

// Dynamically allocated ring buffer (instead of large static array)
static int* ringBuffer = nullptr;
static volatile int writeIndex = 0;
static volatile int readIndex  = 0;

static WiFiManager* wifiManager = nullptr;

// Keep the timer handle in global scope so it doesn't go out of scope
static esp_timer_handle_t samplingTimer = nullptr;

// ---------- Timer Callback (ESP_TIMER_TASK) ----------
// Called at SAMPLE_RATE to read ADC and store in ringBuffer.
static void IRAM_ATTR audioSamplingCallback(void* arg)
{
    // Calculate nextWrite index circularly
    int nextWrite = (writeIndex + 1) % RING_BUFFER_SIZE;

    // If the buffer is full, advance readIndex to make room
    if (nextWrite == readIndex) {
        readIndex = (readIndex + 1) % RING_BUFFER_SIZE;
    }

    // Write ADC sample
    ringBuffer[writeIndex] = adc1_get_raw(ADC1_CHANNEL);

    // Move writeIndex forward
    writeIndex = nextWrite;
}

// ---------- Audio Streaming Task ----------
// This task periodically checks if enough samples have accumulated
// and sends them over Wi-Fi in chunks of CHUNK_SIZE.
void audio_stream_task(void* pvParameters)
{
    ESP_LOGI(TAG, "Audio streaming task started");

    // Local buffer for sending CHUNK_SIZE samples
    int localBuffer[CHUNK_SIZE];

    while (true) {
        // Number of samples in the ring buffer
        int available = (writeIndex - readIndex + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;

        if (available >= CHUNK_SIZE) {
            // Read out CHUNK_SIZE samples
            for (int i = 0; i < CHUNK_SIZE; i++) {
                localBuffer[i] = ringBuffer[readIndex];
                readIndex = (readIndex + 1) % RING_BUFFER_SIZE;
            }

            // Send the chunk via Wi-Fi
            wifiManager->sendRawData(localBuffer, CHUNK_SIZE);
            ESP_LOGI(TAG, "Sent %d samples", CHUNK_SIZE);
        } 
        else {
            // Not enough data yet; yield
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

// ---------- Helper Functions ----------

// Wi-Fi initialization
static void initWiFi()
{
    ESP_LOGI(TAG, "Initializing Wi-Fi Manager...");
    // Create WiFiManager object (adjust IP/ports to your needs)
    wifiManager = new WiFiManager("192.168.2.2",
                                  nullptr, // We'll no longer pass the ring buffer pointer here
                                  0,
                                  8080,
                                  8081);

    wifiManager->connectToOpenNetwork("belkin54g");
}

// ADC configuration
static void configADC()
{
    ESP_LOGI(TAG, "Configuring ADC...");
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL, ADC_ATTEN_DB_11);
}

// Create and start the sampling timer (fires at SAMPLE_RATE)
static void startAudioSamplingTimer()
{
    ESP_LOGI(TAG, "Creating ADC sampling timer...");

    esp_timer_create_args_t timerArgs = {
        .callback        = &audioSamplingCallback,
        .arg             = NULL,
        .dispatch_method = ESP_TIMER_TASK, // callback runs in timer service task
        .name            = "ADC Sampling Timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &samplingTimer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(samplingTimer, 1000000 / SAMPLE_RATE));

    ESP_LOGI(TAG, "ADC sampling timer started at %d Hz", SAMPLE_RATE);
}

// Start the streaming task with a bigger stack size
static void startAudioStreamingTask()
{
    ESP_LOGI(TAG, "Starting audio streaming task...");
    // Increase stack to 8192 to guard against stack overflow
    xTaskCreate(audio_stream_task, "AudioStreamTask", 8192, NULL, 5, NULL);
}

// ---------- Main App Entry ----------
extern "C" void app_main()
{
    // 1) Allocate ring buffer from heap
    ringBuffer = (int*)heap_caps_malloc(RING_BUFFER_SIZE * sizeof(int), MALLOC_CAP_DEFAULT);
    if (!ringBuffer) {
        ESP_LOGE(TAG, "Failed to allocate ring buffer!");
        return; // Can't continue
    }

    // (Optional) Fill with a known pattern to detect overwrites
    for (int i = 0; i < RING_BUFFER_SIZE; i++) {
        ringBuffer[i] = 0xDEADDEAD;
    }

    // 2) Initialize Wi-Fi
    initWiFi();

    // 3) Configure ADC
    configADC();

    // 4) Wait for Wi-Fi to connect (adjust as needed)
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 5) Create and start the ADC sampling timer
    startAudioSamplingTimer();

    // 6) Create the audio streaming task with a larger stack
    startAudioStreamingTask();

    // app_main completes, but the FreeRTOS tasks remain running
}
