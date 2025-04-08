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

extern "C" void app_main() {
    OLED_Display oled;
    oled.init();
    oled.clear();
    int loops = 0;

    while (1) {
        oled.drawText(10, 10, "ECE 477", &Font8, BLACK, WHITE);
        oled.drawText(10, 40, "ECE 477", &Font16, BLACK, WHITE);
        oled.display();

        Driver_Delay_ms(2000);
        oled.clear_buffer();
        
        oled.drawText(0, 0, "ALFREDO GAY", &Font24, BLACK, WHITE);
        oled.display();
        
        Driver_Delay_ms(1000);
        oled.clear_buffer();
        oled.display();

        oled.drawText(0, 0, "BEN TARD", &Font24, BLACK, WHITE);
        oled.display();
        
        Driver_Delay_ms(1000);
        oled.clear_buffer();
        oled.display();

        oled.drawText(0, 0, "ROHIT BLACK", &Font24, BLACK, WHITE);
        oled.display();
        
        Driver_Delay_ms(1000);
        oled.clear_buffer();
        oled.display();
        
        Driver_Delay_ms(2000);
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "This is loop %d!", loops++);
        oled.drawText(0,0, buffer, &Font8, BLACK, WHITE);
        oled.display();
        Driver_Delay_ms(2000);
        oled.clear_buffer();
    }
}