#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define LED_COUNT 64
#define BITS_PER_RGB 24
#define SPI_DEVICE "/dev/spidev0.0"

// WS2812B "Bit" encoding via SPI bytes
#define WS2812_0 0xC0
#define WS2812_1 0xFC

void transmit_leds(int fd, uint8_t *rgb_data) {
    size_t buffer_size = LED_COUNT * BITS_PER_RGB;
    uint8_t *spi_buf = malloc(buffer_size);

    for (int i = 0; i < LED_COUNT * 3; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            spi_buf[i * 8 + (7 - bit)] = (rgb_data[i] & (1 << bit)) ? WS2812_1 : WS2812_0;
        }
    }

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)spi_buf,
        .len = buffer_size,
        .speed_hz = 6400000,
        .bits_per_word = 8,
    };

    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    free(spi_buf);
}

int main() {
    int fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) { 
        perror("Can't open SPI device - check if SPI is enabled in config.txt"); 
        return 1; 
    }

    uint8_t leds[LED_COUNT * 3] = {0};
    printf("Actuating 64 LED Grid via Generic SPI Controller...\n");

    while (1) {
        for (int i = 0; i < LED_COUNT * 3; i += 3) {
            leds[i] = 0x20;   // Green
            leds[i+1] = 0x00; // Red
            leds[i+2] = 0x00; // Blue
        }
        transmit_leds(fd, leds);
        usleep(500000);

        memset(leds, 0, sizeof(leds));
        transmit_leds(fd, leds);
        usleep(500000);
    }

    close(fd);
    return 0;
}
