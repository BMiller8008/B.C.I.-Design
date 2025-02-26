/*
-----------------------------------------------------------------------------------
| OLED Pin Name | ESP32 Pin (New Suggestion) | Description                         |
|--------------|---------------------------|-------------------------------------|
| VCC         | 3.3V                        | Power supply (3.3V recommended)     |
| GND         | GND                          | Ground                              |
| CS          | GPIO21                       | Chip Select (SPI)                   |
| RST         | GPIO22                       | Reset                               |
| DC          | GPIO19                       | Data/Command                        |
| CLK (SCK)   | GPIO18                       | SPI Clock                           |
| MOSI (DIN)  | GPIO23                       | SPI Data                            |
| MISO        | Not used                     | (Not needed for OLED)               |
-----------------------------------------------------------------------------------
*/

#include "OLED_Driver.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include "fonts.h"
#include "esp_log.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"

// ADC channel and width configuration
#define ADC_CHANNEL ADC1_CHANNEL_0
#define ADC_WIDTH ADC_WIDTH_BIT_12 
#define ADC_ATTEN ADC_ATTEN_DB_11  

class BatteryMonitor {
public:
    char * battery_status;

    BatteryMonitor(adc1_channel_t channel) : channel_(channel) {
        adc1_config_width(ADC_WIDTH);
        adc1_config_channel_atten(channel_, ADC_ATTEN);
    }

    float readVoltage() {
        int raw_value = adc1_get_raw(channel_);
        return (raw_value / 4095.0) * 3.3;
    }

    void printBatteryStatus() {
        float voltage = readVoltage();
        if (voltage > 2.78) {
            // printf("100%%\n");
            this->battery_status = "100%%\n";
        } else if (voltage > 2.26) {
            // printf("75%%\n");
            this->battery_status = "75%%\n";
        } else if (voltage > 1.73) {
            this->battery_status = "50%%\n";
        } else if (voltage > 1.21) {
            this->battery_status = "Low Battery!\n25%%\n";
        } else {
            this->battery_status = "Shutting Down\n0%%\n";
        }
    }

private:
    adc1_channel_t channel_;
};

void adc_task(void *arg) {
    BatteryMonitor *monitor = static_cast<BatteryMonitor*>(arg);
    
    while (true) {
        monitor->printBatteryStatus();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

extern "C" void app_main() {
    static BatteryMonitor monitor(ADC_CHANNEL);

    // const char * battery_status;

    xTaskCreate(adc_task, "ADC Reader", 4096, &monitor, 1, NULL);

    OLED_Display oled;
    oled.init();
    oled.clear();
    int loops = 0;

    while (1) {
        oled.drawText(10, 10, monitor.battery_status, &Font8, BLACK, WHITE);
        // oled.drawText(10, 10, "ECE 477", &Font8, BLACK, WHITE);
        oled.drawText(10, 40, "ECE 477", &Font16, BLACK, WHITE);
        oled.display();
        Driver_Delay_ms(2000);
        oled.clear_buffer();
        
        // oled.drawText(0, 0, "HELLO", &Font24, BLACK, WHITE);
        // oled.display();
        
        // Driver_Delay_ms(1000);
        // oled.clear_buffer();
        // oled.display();
        
        // Driver_Delay_ms(2000);
        // char buffer[50];
        // snprintf(buffer, sizeof(buffer), "This is loop %d!", loops++);
        // oled.drawText(0,0, buffer, &Font8, BLACK, WHITE);
        // oled.display();
        // Driver_Delay_ms(2000);
        // oled.clear_buffer();
    }
}


// #include <cstdio>
// #include "driver/gpio.h"
// #include "esp_err.h"

// class Button {
// public:
//     using CallbackFunction = void(*)();

//     //needs to work with external pull-up resistors as well
//     Button(gpio_num_t pin, CallbackFunction callback) : pin_(pin), callback_(callback) {
//         // configure GPIO
//         gpio_config_t io_conf = {};
//         io_conf.pin_bit_mask = (1ULL << pin_);
//         io_conf.mode = GPIO_MODE_INPUT;
//         io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
//         io_conf.intr_type = GPIO_INTR_NEGEDGE;  // negedge for press
//         gpio_config(&io_conf);

//         gpio_install_isr_service(0);
//         gpio_isr_handler_add(pin_, isrHandler, this);
//     }

// private:
//     gpio_num_t pin_;
//     CallbackFunction callback_;

//     // interrupt service routine
//     static void IRAM_ATTR isrHandler(void* arg) {
//         Button* btn = static_cast<Button*>(arg);
//         btn->callback_();
//     }
// };

// void onLanguagePress() {
//     printf("Language Button Pressed!\n");
// }

// void onTextPress() {
//     printf("Text Button Pressed!\n");
// }

// extern "C" void app_main() {
//     static Button language(GPIO_NUM_4, onLanguagePress);
//     static Button text(GPIO_NUM_5, onTextPress);
// }