#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

#define RING_BUFFER_SIZE (400 * 1024) //assuming 20kb per 5s audio & 1min buffer

#define PACKET_SIZE 512 // test 512 bytes per packet

static RingbufHandle_t ringbuf = NULL;

// sim incoming audio packets (fills buffer with random data)
void producer_task(void *arg) {
    while (true) {
        //fake ADPCM packet
        uint8_t *packet = (uint8_t *)malloc(PACKET_SIZE);
        if (packet == NULL) {
            printf("Memory allocation failed!\n");
            continue;
        }

        //  random data in packet
        for (int i = 0; i < PACKET_SIZE; i++) {
            packet[i] = rand() % 256;
        }

        if (xRingbufferSend(ringbuf, packet, PACKET_SIZE, pdMS_TO_TICKS(100)) == pdFALSE) {
            printf("Buffer full, dropping oldest data.\n");
        }

        free(packet);

        vTaskDelay(pdMS_TO_TICKS(100));  //new audio packet arrival rate
    }
}

void consumer_task(void *arg) {
    while (true) {
        size_t item_size;
        uint8_t *received_data = (uint8_t *)xRingbufferReceive(ringbuf, &item_size, pdMS_TO_TICKS(500));

        if (received_data != NULL) {
            printf("Received packet of size: %d bytes\n", (int)item_size);
            vRingbufferReturnItem(ringbuf, received_data); 
        } else {
            printf("No data available in buffer.\n");
        }

        vTaskDelay(pdMS_TO_TICKS(500));  // transmission intervals
    }
}

// Initialize ring buffer and create tasks
extern "C" void app_main() {
    ringbuf = xRingbufferCreate(RING_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
    if (ringbuf == NULL) {
        printf("Failed to create ring buffer!\n");
        return;
    }

    xTaskCreate(producer_task, "Producer", 4096, NULL, 1, NULL);

    xTaskCreate(consumer_task, "Consumer", 4096, NULL, 1, NULL);
}