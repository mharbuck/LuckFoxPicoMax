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
// At ~6.4MHz, 0b11000000 is a '0' and 0b11111100 is a '1'
#define WS2812_0 0xC0
#define WS2812_1 0xFC

void transmit_leds(int fd, uint8_t *rgb_data) {
    // Each 1 bit of RGB data becomes 1 byte of SPI data
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
        .speed_hz = 6400000, // 6.4 MHz timing
        .bits_per_word = 8,
    };

    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    free(spi_buf);
}

int main() {
    int fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) { perror("Can't open SPI device"); return 1; }

    // 64 LEDs * 3 bytes (G, R, B order for WS2812B)
    uint8_t leds[LED_COUNT * 3] = {0};

    printf("Initializing 64 LED Grid...\n");

    while (1) {
        // Example: Set all LEDs to a dim Green
        for (int i = 0; i < LED_COUNT * 3; i += 3) {
            leds[i] = 0x10;   // Green
            leds[i+1] = 0x00; // Red
            leds[i+2] = 0x00; // Blue
        }
        
        transmit_leds(fd, leds);
        usleep(500000); // 500ms
        
        // Simple Toggle Off
        memset(leds, 0, sizeof(leds));
        transmit_leds(fd, leds);
        usleep(500000);
    }

    close(fd);
    return 0;
}
