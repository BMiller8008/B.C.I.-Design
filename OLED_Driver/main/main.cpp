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
    // **1. Initialize SPI & OLED**
    System_Init();
    OLED_Display oled;
    oled.init();
    oled.clear();

    // **2. Create framebuffer (128x64)**
    uint8_t imageBuffer[1024] = {0}; // OLED 128x64 / 8 pages
    Paint_NewImage(imageBuffer, 128, 64, ROTATE_0, BLACK);
    
    Paint_SelectImage(imageBuffer);

    while (1)
    {
        Paint_Clear(BLACK);

        // **3. Draw a test string in 8pt font (Foreground: WHITE, Background: BLACK)**
        Paint_DrawString_EN(10, 10, "ECE 477", &Font8, BLACK, WHITE);
        Paint_DrawString_EN(10, 40, "ECE 477", &Font16, BLACK, WHITE);

        // **4. Send framebuffer to OLED**
        oled.display(imageBuffer);

        // **5. Wait 10 sec**
        Driver_Delay_ms(5000);
        Paint_Clear(BLACK);
        Paint_DrawString_EN(10, 40, "ALFREDO GAY", &Font16, BLACK, WHITE);
        oled.display(imageBuffer);
        Driver_Delay_ms(5000);
        Paint_Clear(BLACK);
        oled.display(imageBuffer);
        Driver_Delay_ms(5000);
    }
}
