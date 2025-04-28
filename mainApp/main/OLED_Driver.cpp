/*****************************************************************************
* | File        :   OLED_Driver.cpp
* | Author      :   BCI Team                        
* | Function    :   1.3inch OLED Drive function
* | Info        :
*----------------
* | This version:   V2.0
* | Date        :   2025-02-04
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "OLED_Driver.h"
#include <stdio.h>
#include "esp_log.h"

#define TAG "OLED"

OLED_Display::OLED_Display() {
    // filling and setting fram buffer
    Paint_NewImage(framebuffer, OLED_1IN51_HEIGHT, OLED_1IN51_WIDTH, ROTATE_180, BLACK); //ROTATE_0
    // Paint_SelectImage(framebuffer);
    Paint_Clear(BLACK);
}

OLED_Display::~OLED_Display() {
    // Destructor: Cleanup if needed.
}

void OLED_Display::reset() {
    ESP_LOGI(TAG, "Resetting OLED...");
    OLED_RST_1;
    Driver_Delay_ms(100);
    OLED_RST_0;
    Driver_Delay_ms(100);
    OLED_RST_1;
    Driver_Delay_ms(100);
}

void OLED_Display::writeCommand(uint8_t cmd) {
    OLED_DC_0;
    OLED_CS_0;
    SPI4W_Write_Byte(cmd);
    OLED_CS_1;
}

void OLED_Display::writeData(uint8_t data) {
    OLED_DC_1;
    OLED_CS_0;
    SPI4W_Write_Byte(data);
    OLED_CS_1;
}

void OLED_Display::init() {
    // initializing SPI
    System_Init();

    ESP_LOGI(TAG, "Initializing OLED...");

    // **Configure GPIOs**
    gpio_reset_pin(OLED_CS);
    gpio_set_direction(OLED_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(OLED_CS, 1);

    gpio_reset_pin(OLED_RST);
    gpio_set_direction(OLED_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(OLED_RST, 1);

    gpio_reset_pin(OLED_DC);
    gpio_set_direction(OLED_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(OLED_DC, 1);

    reset();  // Perform hardware reset

    // Initialization sequence for OLED
    writeCommand(0xAE); // Display OFF
    writeCommand(0x20); // Set Memory Addressing Mode
    writeCommand(0x00); // Horizontal Addressing Mode
    writeCommand(0xA6); // Set Normal Display

    // More initialization commands...
    writeCommand(0xAF); // Display ON

    ESP_LOGI(TAG, "OLED Initialized!");
}


void OLED_Display::clear() {
    ESP_LOGI(TAG, "Clearing OLED screen...");
    for (uint8_t i = 0; i < 8; i++) {
        writeCommand(0xB0 + i); // Set page address
        writeCommand(0x00);     // Set low column address
        writeCommand(0x10);     // Set high column address
        for (uint8_t j = 0; j < 128; j++) {
            writeData(0x00); // Clear display
        }
    }
}

void OLED_Display::display() {
    for (uint8_t page = 0; page < 8; page++) {
        writeCommand(0xB0 + page); 
        writeCommand(0x00);
        writeCommand(0x10);

        for (uint8_t col = 0; col < OLED_1IN51_HEIGHT; col++) {
            writeData(framebuffer[page * OLED_1IN51_HEIGHT + col]);
        }
    }
}

void OLED_Display::drawText(int x, int y, const char* text, sFONT* font, uint16_t fgColor, uint16_t bgColor) {
    Paint_DrawString_EN(x,y,text,font,fgColor,bgColor);
}

void OLED_Display::clear_buffer(){
    Paint_Clear(BLACK);
}

void OLED_Display::draw_rect(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill) {
    Paint_DrawRectangle(Xstart, Ystart, Xend, Yend, Color, Line_width, Draw_Fill);
}

void OLED_Display::draw_num(UWORD Xpoint, UWORD Ypoint,const char * Number,
        sFONT* Font, UWORD Digit,UWORD Color_Foreground, UWORD Color_Background) {
    Paint_DrawNum(Xpoint, Ypoint, Number, Font, Digit, Color_Foreground, Color_Background);
}

void OLED_Display::DrawBitMap(const unsigned char* image) {
    Paint_DrawBitMap(image);
}