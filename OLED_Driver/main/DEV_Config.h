/******************************************************************************
**************************Hardware interface layer*****************************
* | file      	:	DEV_Config.h
* |	version		  :	V1.0
* | date		    :	2020-06-16
* | function	  :	Provide the hardware underlying interface	
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

/**
 * data
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

#define USE_SPI_4W  1
#define USE_IIC     0

#define IIC_CMD        0X00
#define IIC_RAM        0X40

/****************************************************************************************
    //Use the  library function definition
*****************************************************************************************/
//GPIO config
#define OLED_CS      GPIO_NUM_21
#define OLED_RST     GPIO_NUM_22
#define OLED_DC      GPIO_NUM_19

#define OLED_CS_0    gpio_set_level(OLED_CS, 0)
#define OLED_CS_1    gpio_set_level(OLED_CS, 1)

#define OLED_RST_0   gpio_set_level(OLED_RST, 0)
#define OLED_RST_1   gpio_set_level(OLED_RST, 1)

#define OLED_DC_0    gpio_set_level(OLED_DC, 0)
#define OLED_DC_1    gpio_set_level(OLED_DC, 1)

/*------------------------------------------------------------------------------------------------------*/
uint8_t System_Init(void);

void SPI4W_Write_Byte(uint8_t DATA);

void Driver_Delay_ms(unsigned long xms);
void Driver_Delay_us(int xus);

#endif
