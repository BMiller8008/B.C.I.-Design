#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GPIO_Example";

// Define GPIO pins
#define LED_GPIO GPIO_NUM_25    // GPIO for LED (adjust based on your board)

// Function to initialize GPIO
void initialize_gpio() {
    // Configure LED GPIO as an output
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO), // Bit mask for GPIO
        .mode = GPIO_MODE_OUTPUT,          // Set as output
        .pull_up_en = GPIO_PULLUP_DISABLE, // No pull-up resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // No pull-down resistor
        .intr_type = GPIO_INTR_DISABLE,    // No interrupt
    };
    ESP_ERROR_CHECK(gpio_config(&led_config));
}

void gpio_task(void *pvParameters) {
    // changing the led state
    bool led_state = false;

    while (true)
    {
        gpio_set_level(LED_GPIO, led_state); // Update LED
        ESP_LOGI(TAG, "LED UPDATED");
        led_state = !led_state;
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
    
}

extern "C" void app_main() {
    // Initialize GPIO
    initialize_gpio();

    // Create GPIO task
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 5, NULL);
}