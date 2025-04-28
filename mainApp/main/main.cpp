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

// ----- For State Tracking -----
enum AppState {
    STATE_MAIN,
    STATE_FONT,
    STATE_LANG,
    STATE_OFF
};

// ==================== Globals ====================

// ----- Ring buffer (shared between ISR and main code) -----
static int* ringBuffer = nullptr;              // Pointer to dynamically allocated ring buffer
static volatile int writeIndex = 0;            // Index for writing into the ring buffer
static volatile int readIndex  = 0;            // Index for reading from the ring buffer

// ----- Button state tracking -----
volatile bool button1Pressed = false;          // True if Button 1 is currently pressed
volatile bool button2Pressed = false;          // True if Button 2 is currently pressed

// ----- App state and synchronization -----
AppState state = STATE_OFF;                    // Current application state
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; // Mutex for critical sections (used in ISRs)

// ----- Core components -----
static WiFiManager* wifiManager = nullptr;     // Pointer to WiFi manager instance
static OLED_Display* oled = nullptr;           // Pointer to OLED display instance
static MainApp* app = nullptr;                 // Pointer to main app logic instance
static esp_timer_handle_t samplingTimer = nullptr; // Handle for periodic sampling timer

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

// ------ Button Callback -----
static void IRAM_ATTR button_isr_handler(void* arg) {
    int pin = (int)(intptr_t)arg;

    portENTER_CRITICAL_ISR(&mux);

    if (pin == BUTTON1_GPIO) {
        button1Pressed = true;
    }

    if (pin == BUTTON2_GPIO) {
        button2Pressed = true;
    }

    portEXIT_CRITICAL_ISR(&mux);
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
            wifiManager->sendRawData(localBuffer, CHUNK_SIZE);
            vTaskDelay(pdMS_TO_TICKS(100));
            // ESP_LOGI(TAG, "Sent %d samples", CHUNK_SIZE);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// ----- OLED Display Message Task -----
static void display_task(void* pvParameters) {
    while (true) {
        // checking if in menu or other state
        if (state == STATE_OFF)
        {
            if (wifiManager != NULL && wifiManager->hasNewMessage() && app->getState() == MainApp::State::ON) {
                const char* msg = wifiManager->getReceivedMessage();
                ESP_LOGI(TAG, "Receive message!!");
                oled->clear_buffer();
                // getting font sizes
                std::string curr_font = app->getFontSize();
                if (curr_font == "8")
                {
                    oled->drawText(17, 30, msg, &Font8, BLACK, WHITE);
                }
                else if (curr_font == "12")
                {
                    oled->drawText(17, 25, msg, &Font12, BLACK, WHITE);
                }
                else {
                    oled->drawText(17, 15, msg, &Font16, BLACK, WHITE);
                }
                oled->display();
                vTaskDelay(pdMS_TO_TICKS(1200));
            }
            else {
                vTaskDelay(pdMS_TO_TICKS(100));
                oled->clear_buffer();
                oled->display();
            }
        }
        else if (state == STATE_MAIN)
        {
            app->updateBattery();
            app->displayMainMenu(oled);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else if (state == STATE_LANG)
        {
            app->displayLangChoice(oled);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else if (state == STATE_FONT) {
            app->displayFontChoice(oled);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// ----- Setup Helpers -----
static void initWiFi() {
    ESP_LOGI(TAG, "Initializing Wi-Fi Manager...");
    wifiManager = new WiFiManager("192.168.1.254", 8080, 8081, 8088);
    wifiManager->connectToOpenNetwork("NETGEAR41");
}

// ----- Setup Helpers -----
static void initButtons() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; //GPIO_PULLUP_ENABLE
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pin_bit_mask = (1ULL << BUTTON1_GPIO) | (1ULL << BUTTON2_GPIO);
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON1_GPIO, button_isr_handler, (void*)BUTTON1_GPIO);
    gpio_isr_handler_add(BUTTON2_GPIO, button_isr_handler, (void*)BUTTON2_GPIO);
}

static void configADC() {
    // configuring audio ADC
    ESP_LOGI(TAG, "Configuring ADC 1...");
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL, ADC_ATTEN_DB_11);

    // configuring battery ADC
    ESP_LOGI(TAG, "Configuring ADC 2...");
    adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_11);
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

// ----- Task to handle app state -----
void button_task(void* pvParameters) {
    AppState lastState = state;

    while (true) {
        // ---- Clear and read button flags (atomic section) ----
        bool b1 = false, b2 = false;
        portENTER_CRITICAL(&mux);
        b1 = button1Pressed;
        b2 = button2Pressed;
        button1Pressed = false;
        button2Pressed = false;
        portEXIT_CRITICAL(&mux);

        // debounce
        vTaskDelay(pdMS_TO_TICKS(150));

        // ---- Enter menu from OFF using Button2 ----
        if ((b2 && !b1) && state == STATE_OFF && app->getState() == MainApp::State::ON) {
            state = STATE_MAIN;
            ESP_LOGI(TAG, "Entered menu mode");
        }

        // ---- Scroll menu with Button1 ----
        else if ((b1 && !b2) && state != STATE_OFF && app->getState() == MainApp::State::ON) {
            app->setMenuIdx(app->getMenuIdx() + 1);
            ESP_LOGI("MENU", "Menu index increased");
        }

        // ---- Select item / change setting with Button2 ----
        else if ((!b1 && b2) && state != STATE_OFF && app->getState() == MainApp::State::ON) {
            switch (state) {
                case STATE_MAIN: {
                    int idx = app->getMenuIdx();
                    state = (idx % 2 == 0) ? STATE_LANG : STATE_FONT;
                    break;
                }

                case STATE_LANG: {
                    const auto& langs = app->getAvailableLanguages();
                    const auto& choice = langs[app->getMenuIdx() % langs.size()];
                    app->setLanguage(choice);
                    state = STATE_OFF;
                    break;
                }

                case STATE_FONT: {
                    const auto& fonts = app->getAvailableFontSizes();
                    const auto& choice = fonts[app->getMenuIdx() % fonts.size()];
                    app->setFontSize(choice);
                    state = STATE_OFF;
                    break;
                }

                default:
                    break;
            }

            ESP_LOGI(TAG, "Menu action completed. New state = %d", state);
        }

        // ---- Handle state transition (e.g., ON ↔ OFF) ----
        if (state != lastState) {
            app->setMenuIdx(0);

            std::string command = "state:";
            command += (app->getState() == MainApp::State::ON) ? "on," : "off,";
            command += "lang:" + app->getLanguage();
            command += ",font:" + app->getFontSize();
            wifiManager->sendCommandToServer(command);  // Uncomment if needed

            if (state == STATE_OFF) {
                startAudioSamplingTimer();  // resume audio
            } else {
                esp_timer_stop(samplingTimer);  // pause audio
            }

            lastState = state;
        }
    }
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
    initButtons();

    // Creating Objects
    oled = new OLED_Display();
    oled->init();
    oled->clear();
    oled->drawText(10, 40, "ECE 477", &Font16, BLACK, WHITE); // default text for debugging
    oled->display();
    vTaskDelay(pdMS_TO_TICKS(1000));
    app = new MainApp();

    // 3. Start tasks
    xTaskCreate(audio_stream_task, "AudioStreamTask", 8192, nullptr, 5, nullptr);
    xTaskCreate(display_task, "DisplayTask", 4096, nullptr, 4, nullptr);
    xTaskCreate(button_task, "ButtonTask", 4096, nullptr, 6, nullptr);
    startAudioSamplingTimer();
}
