#include "microphone.h"

#define TAG "MAX4466_MIC"
#define WDT_TIMEOUT 5  
#define FIR_TAP_NUM 64  // Number of FIR filter coefficients

// Copy the coefficients from your Python output
static float fir_coeff[FIR_TAP_NUM] = {
    -0.00057226f, -0.00060791f, 0.00068073f, 0.00079408f, -0.00095147f, -0.00115656f,
    0.00141321f, 0.00172556f, -0.00209806f, -0.00253565f, 0.00304381f, 0.00362882f,
    -0.00429793f, -0.00505971f, 0.00592450f, 0.00690496f, -0.00801689f, -0.00928042f,
    0.01072165f, 0.01237514f, -0.01428766f, -0.01652413f, 0.01917721f, 0.02238380f,
    -0.02635460f, -0.03143101f, 0.03820408f, 0.04779207f, -0.06259374f, -0.08884954f,
    0.14944755f, 0.45040037f, 0.45040037f, 0.14944755f, -0.08884954f, -0.06259374f,
    0.04779207f, 0.03820408f, -0.03143101f, -0.02635460f, 0.02238380f, 0.01917721f,
    -0.01652413f, -0.01428766f, 0.01237514f, 0.01072165f, -0.00928042f, -0.00801689f,
    0.00690496f, 0.00592450f, -0.00505971f, -0.00429793f, 0.00362882f, 0.00304381f,
    -0.00253565f, -0.00209806f, 0.00172556f, 0.00141321f, -0.00115656f, -0.00095147f,
    0.00079408f, 0.00068073f, -0.00060791f, -0.00057226f
};


#define FIR_BUFFER_SIZE 64  // Must match FIR_TAP_NUM

// Circular buffer for FIR input samples
static float fir_input[FIR_BUFFER_SIZE] = {0};
static float fir_output = 0;  // Single output sample
static int fir_index = 0;  // Circular buffer index

int Microphone::applyFilter(int raw_value) {
    // Shift samples in the buffer
    fir_input[fir_index] = (float) raw_value;
    
    fir_output = 0;
    // Apply FIR filter
    for (int i = 0; i < FIR_TAP_NUM; i++) {
        int sample_index = (fir_index - i + FIR_TAP_NUM) % FIR_TAP_NUM;
        fir_output += fir_input[sample_index] * fir_coeff[i];
    }

    // Update circular buffer index
    fir_index = (fir_index + 1) % FIR_TAP_NUM;

    return (int) fir_output;  // Return filtered value as an integer
}


Microphone::Microphone(adc_channel_t pin, int sampleRate)
    : micPin(pin), sampleRateMs(sampleRate) {
    
    // Initialize ADC oneshot mode
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,  
    };
    adc_oneshot_new_unit(&init_config, &adcHandle);

    // Configure ADC channel
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_11,   // Full-scale voltage: 3.3V
        .bitwidth = ADC_BITWIDTH_12 // 12-bit resolution
    };
    adc_oneshot_config_channel(adcHandle, micPin, &channel_config);
}

void Microphone::start() {
    // esp_task_wdt_config_t wdt_config = {
    //     .timeout_ms = WDT_TIMEOUT * 1000,  // Convert timeout to milliseconds
    //     .idle_core_mask = (1 << 0),  // Monitor core 0
    //     .trigger_panic = false  // Do not trigger panic on timeout
    // };

    // esp_task_wdt_init(&wdt_config);
    // esp_task_wdt_add(xTaskGetCurrentTaskHandle());

    xTaskCreate(&Microphone::taskEntry, "MicrophoneTask", 4096, this, 5, nullptr);
}

void Microphone::taskEntry(void *param) {
    static_cast<Microphone *>(param)->readADC();
}

void Microphone::readADC() {

    // esp_task_wdt_add(xTaskGetCurrentTaskHandle()); 

    while (true) {
        int raw_value = 0;
        adc_oneshot_read(adcHandle, micPin, &raw_value); // Read ADC value

        int filtered_value = applyFilter(raw_value); // Apply filtering
        int pcm_value = convertToPCM(filtered_value); // Convert to PCM

        printf("%d\n", pcm_value); // Output PCM data over serial

        // esp_task_wdt_reset(); 

        vTaskDelay(pdMS_TO_TICKS(10)); // Control sampling rate
    }
}


// Convert ADC raw data (12-bit) to 16-bit PCM
int Microphone::convertToPCM(int raw_value) {
    return ((raw_value - 2048) * 16); // Normalize 12-bit ADC to signed 16-bit PCM
}