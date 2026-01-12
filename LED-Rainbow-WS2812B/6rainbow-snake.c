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

#define WS2812_0 0xC0
#define WS2812_1 0xFC

// Global Config
float g_brightness = 0.3;
int g_speed = 4;

// Spiral Lookup Table: The order of LED indices from outside to inside
// This represents an 8x8 grid where index = (y * 8) + x
const uint8_t spiral_map[64] = {
    0,  1,  2,  3,  4,  5,  6,  7, 
    15, 23, 31, 39, 47, 55, 63, 62, 
    61, 60, 59, 58, 57, 56, 48, 40, 
    32, 24, 16, 8,  9,  10, 11, 12, 
    13, 14, 22, 30, 38, 46, 54, 53, 
    52, 51, 50, 49, 41, 33, 25, 17, 
    18, 19, 20, 21, 29, 37, 45, 44, 
    43, 42, 34, 26, 27, 28, 36, 35
};

void hsv_to_rgb(uint8_t h, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;
    uint8_t q = 255 - remainder, t = remainder;
    uint8_t raw_r, raw_g, raw_b;

    switch (region) {
        case 0:  raw_r = 255; raw_g = t;   raw_b = 0;   break;
        case 1:  raw_r = q;   raw_g = 255; raw_b = 0;   break;
        case 2:  raw_r = 0;   raw_g = 255; raw_b = t;   break;
        case 3:  raw_r = 0;   raw_g = q;   raw_b = 255; break;
        case 4:  raw_r = t;   raw_g = 0;   raw_b = 255; break;
        default: raw_r = 255; raw_g = 0;   raw_b = q;   break;
    }
    *r = (uint8_t)(raw_r * g_brightness);
    *g = (uint8_t)(raw_g * g_brightness);
    *b = (uint8_t)(raw_b * g_brightness);
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
        .tx_buf = (unsigned long)spi_buf, .len = (uint32_t)buffer_size,
        .speed_hz = 6400000, .bits_per_word = 8, .delay_usecs = 50, 
    };
    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    free(spi_buf);
}

int main() {
    int fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) { perror("SPI open failed"); return 1; }

    uint8_t leds[LED_COUNT * 3];
    uint8_t head_pos = 0; // Current position of the snake's head
    uint8_t hue_offset = 0;

    while (1) {
        memset(leds, 0, sizeof(leds)); // Clear grid

        // Draw a snake that is 10 LEDs long
        for (int j = 0; j < 15; j++) {
            // Calculate which part of the spiral this segment is on
            int pos = (head_pos - j + LED_COUNT) % LED_COUNT;
            int led_index = spiral_map[pos];

            uint8_t r, g, b;
            hsv_to_rgb(hue_offset + (j * 10), &r, &g, &b);

            // Fade the tail (optional)
            float tail_fade = (15.0f - j) / 15.0f;
            leds[led_index * 3]     = (uint8_t)(g * tail_fade);
            leds[led_index * 3 + 1] = (uint8_t)(r * tail_fade);
            leds[led_index * 3 + 2] = (uint8_t)(b * tail_fade);
        }

        transmit_leds(fd, leds);
        
        head_pos = (head_pos + 1) % LED_COUNT; // Move head along the spiral
        hue_offset += 5; // Cycle colors
        usleep(50000); // Speed of the snake
    }

    close(fd);
    return 0;
}
