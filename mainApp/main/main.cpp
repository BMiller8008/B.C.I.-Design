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

// ----- Globals -----
static int* ringBuffer = nullptr;
static volatile int writeIndex = 0;
static volatile int readIndex  = 0;
volatile bool button1Pressed = false;
volatile bool button2Pressed = false;
AppState state = STATE_OFF;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define SIMULTANEOUS_PRESS_WINDOW_US 20000  // 20 ms

volatile uint32_t lastButton1Time = 0;
volatile uint32_t lastButton2Time = 0;
volatile bool bothButtonsPressedSimultaneous = false;

static WiFiManager* wifiManager = nullptr;
static OLED_Display* oled = nullptr;
static MainApp* app = nullptr;
static esp_timer_handle_t samplingTimer = nullptr;

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
    uint32_t now = (uint32_t)esp_timer_get_time(); // microseconds

    portENTER_CRITICAL_ISR(&mux);
    if (pin == BUTTON1_GPIO) {
        lastButton1Time = now;
        button1Pressed = true;
        if ((now - lastButton2Time) < SIMULTANEOUS_PRESS_WINDOW_US) {
            bothButtonsPressedSimultaneous = true;
        }
    } else if (pin == BUTTON2_GPIO) {
        lastButton2Time = now;
        button2Pressed = true;
        if ((now - lastButton1Time) < SIMULTANEOUS_PRESS_WINDOW_US) {
            bothButtonsPressedSimultaneous = true;
        }
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

            // TODO: Apply FIR or IIR filter here if needed

            wifiManager->sendRawData(localBuffer, CHUNK_SIZE);
            // ESP_LOGI(TAG, "Sent %d samples", CHUNK_SIZE);
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

// ----- OLED Display Message Task -----
static void display_task(void* pvParameters) {
    while (true) {
        // checking if in menu or other state
        if (state == STATE_OFF)
        {
            if (wifiManager->hasNewMessage()) {
                const char* msg = wifiManager->getReceivedMessage();
                ESP_LOGI(TAG, "Receive message!!");
                oled->clear_buffer();
                oled->drawText(10, 10, msg, &Font16, BLACK, WHITE);
                oled->display();
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        else if (state == STATE_MAIN)
        {
            /* code */
            // TODO get adc and display main
        }
        
    }
}

// ----- Setup Helpers -----
static void initWiFi() {
    ESP_LOGI(TAG, "Initializing Wi-Fi Manager...");
    wifiManager = new WiFiManager("192.168.1.5", 8080, 8081, 8088);
    wifiManager->connectToOpenNetwork("NETGEAR41");
}

// ----- Setup Helpers -----
static void initButtons() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
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
        bool b1 = false, b2 = false;
        bool simultaneous = false;
        // updating button states
        portENTER_CRITICAL(&mux);
        b1 = button1Pressed;
        b2 = button2Pressed;
        simultaneous = bothButtonsPressedSimultaneous;
        button1Pressed = false;
        button2Pressed = false;
        bothButtonsPressedSimultaneous = false;
        portEXIT_CRITICAL(&mux);

        if (simultaneous && state == STATE_OFF) {
            state = STATE_MAIN;  // Enter menu mode
            ESP_LOGI(TAG, "Entered menu mode");

        } 
        else if ((b1 && !b2) && state != STATE_OFF) {
            // Scrolling up through menu
            app->setMenuIdx(app->getMenuIdx() + 1);
            ESP_LOGI("MENU", "IDX INCREASED");
        }
        else if ((!b1 && b2) && state != STATE_OFF) {
            // Scrolling up through menu
            app->setMenuIdx(app->getMenuIdx() - 1);
            ESP_LOGI("MENU", "IDX DECREASED0");
        }
        else if (simultaneous && state != STATE_OFF) { //TODO find logic
            // changing menus
            app->setMenuIdx(app->getMenuIdx() - 1);
            ESP_LOGI(TAG, "Menu navigation: new state = %d", state);
        }

        // Handle state change logic (like stopping timer)
        if (state != lastState) {
            // reseting idx
            app->setMenuIdx(0);
            // TODO send command
            if (state == STATE_OFF) {
                startAudioSamplingTimer();  // resume streaming
            } else {
                esp_timer_stop(samplingTimer);  // pause streaming
            }
            lastState = state;
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // debounce
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
    xTaskCreate(button_task, "ButtonTask", 2048, nullptr, 6, nullptr);
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
