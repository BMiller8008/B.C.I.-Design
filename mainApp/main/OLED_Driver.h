/*****************************************************************************
* | File      	:   OLED_Driver.h
* | Author      :   BCI team
* | Function    :   1.51inch OLED Drive function
* | Info        :
*----------------
* |	This version:   V2.0
* | Date        :   2025-02-03
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

/*
-----------------------------------------------------------------------------------
| OLED Pin Name | ESP32 Pin                | Description                         |
|-------------|---------------------------|-------------------------------------|
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
#ifndef __OLED_DRIVER_H
#define __OLED_DRIVER_H		

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "fonts.h"

/********************************************************************************
function:	
		Define the full screen height length of the display
********************************************************************************/
#define OLED_1IN51_WIDTH  64 //OLED width
#define OLED_1IN51_HEIGHT 128 //OLED height

class OLED_Display {
public:
    OLED_Display();
    ~OLED_Display();

    void init();
    void clear();
    void display(); //const uint8_t *image
    void reset();
    void drawText(int x, int y, const char* text, sFONT* font, uint16_t fgColor, uint16_t bgColor);
    void clear_buffer();
    void draw_rect(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill);
    void draw_num(UWORD Xpoint, UWORD Ypoint,const char * Number,
                   sFONT* Font, UWORD Digit,UWORD Color_Foreground, UWORD Color_Background);                  
    void DrawBitMap(const unsigned char* image);
private:
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    uint8_t framebuffer[OLED_1IN51_WIDTH * OLED_1IN51_HEIGHT / 8]; // 128 x 64 pixels

};

#endif  // OLED_DRIVER_H
