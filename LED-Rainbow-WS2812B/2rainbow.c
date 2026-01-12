#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <math.h>

#define LED_COUNT 64
#define BITS_PER_RGB 24
#define SPI_DEVICE "/dev/spidev0.0"

// Global Brightness Control (0.0 to 1.0)
// Set to 0.5 for half intensity
#define BRIGHTNESS 0.25 

#define WS2812_0 0xC0
#define WS2812_1 0xFC

// --- Helper: HSV to RGB with scaling ---
void hsv_to_rgb(uint8_t h, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;
    uint8_t q = 255 - remainder;
    uint8_t t = remainder;

    uint8_t raw_r, raw_g, raw_b;

    switch (region) {
        case 0:  raw_r = 255; raw_g = t;   raw_b = 0;   break;
        case 1:  raw_r = q;   raw_g = 255; raw_b = 0;   break;
        case 2:  raw_r = 0;   raw_g = 255; raw_b = t;   break;
        case 3:  raw_r = 0;   raw_g = q;   raw_b = 255; break;
        case 4:  raw_r = t;   raw_g = 0;   raw_b = 255; break;
        default: raw_r = 255; raw_g = 0;   raw_b = q;   break;
    }

    // Apply brightness scaling here
    *r = (uint8_t)(raw_r * BRIGHTNESS);
    *g = (uint8_t)(raw_g * BRIGHTNESS);
    *b = (uint8_t)(raw_b * BRIGHTNESS);
}

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
        .len = (uint32_t)buffer_size,
        .speed_hz = 6400000,
        .bits_per_word = 8,
        .delay_usecs = 50, 
    };

    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    free(spi_buf);
}

int main() {
    int fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) { perror("Can't open SPI device"); return 1; }

    uint8_t leds[LED_COUNT * 3] = {0};
    uint8_t hue_offset = 0;

    printf("Starting Effects (Intensity: %.0f%%)... \n", BRIGHTNESS * 100);

    while (1) {
        for (int i = 0; i < LED_COUNT; i++) {
            uint8_t r, g, b;
            uint8_t hue = hue_offset + (i * 5); 
            hsv_to_rgb(hue, &r, &g, &b);

            // WS2812B GRB Order
            leds[i * 3]     = g; 
            leds[i * 3 + 1] = r; 
            leds[i * 3 + 2] = b; 
        }

        transmit_leds(fd, leds);
        
        hue_offset += 2;      
        usleep(20000); // 50 FPS
    }

    close(fd);
    return 0;
}
