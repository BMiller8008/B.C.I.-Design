/******************************************************************************
**************************Hardware interface layer*****************************
  | file        : DEV_Config.cpp
  | version     : V1.0
  | date        : 2020-06-16
  | function    : Provide the hardware underlying interface
******************************************************************************/

#include "DEV_Config.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

/********************************************************************************
  function: System Init and exit
  note:
  Initialize the communication method
********************************************************************************/

static spi_device_handle_t spi;

// SPI Configuration Parameters
uint8_t System_Init(void)
{
    gpio_set_direction(OLED_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(OLED_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(OLED_DC, GPIO_MODE_OUTPUT);

    ESP_LOGI("OLED", "Initializing SPI bus...");

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = GPIO_NUM_23;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = GPIO_NUM_18;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.data4_io_num = -1;  // New field
    buscfg.data5_io_num = -1;  // New field
    buscfg.data6_io_num = -1;  // New field
    buscfg.data7_io_num = -1;  // New field
    buscfg.max_transfer_sz = 128 * 8;

    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits = 0;
    devcfg.address_bits = 0;
    devcfg.dummy_bits = 0;
    devcfg.mode = 0;                     // SPI mode 0
    devcfg.duty_cycle_pos = 0;            // Needed to match struct order
    devcfg.cs_ena_pretrans = 0;
    devcfg.cs_ena_posttrans = 0;
    devcfg.clock_speed_hz = 5 * 1000 * 1000;  // 5MHz
    devcfg.input_delay_ns = 0;
    devcfg.spics_io_num = OLED_CS;        // Chip select pin
    devcfg.flags = 0;                     // Ensure default settings
    devcfg.queue_size = 1;
    devcfg.pre_cb = NULL;
    devcfg.post_cb = NULL;

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE("OLED", "Failed to initialize SPI bus!");
        return 1;
    }

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE("OLED", "Failed to add SPI device!");
        return 1;
    }

    return 0;
}

/********************************************************************************
  function: Hardware interface
  note:
  SPI4W_Write_Byte(value) :
    hardware SPI
********************************************************************************/
void SPI4W_Write_Byte(uint8_t DATA)
{
    spi_transaction_t t = {};
    t.flags = SPI_TRANS_USE_TXDATA;  // Use `tx_data` buffer instead of dynamic allocation
    t.length = 8;  // 8-bit transfer
    t.tx_data[0] = DATA;

    spi_device_transmit(spi, &t);
}

/********************************************************************************
  function: Delay function
  note:
  Driver_Delay_ms(xms) : Delay x ms
  Driver_Delay_us(xus) : Delay x us
********************************************************************************/
void Driver_Delay_ms(unsigned long xms)
{
    vTaskDelay(pdMS_TO_TICKS(xms)); // Convert milliseconds to ticks
}

void Driver_Delay_us(int xus)
{
    esp_rom_delay_us(xus);
}
