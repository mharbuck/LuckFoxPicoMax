#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define LED_COUNT 64
#define LED_WIDTH 8
#define LED_HEIGHT 8
#define BITS_PER_RGB 24
#define SPI_DEVICE "/dev/spidev0.0"

#define WS2812_0 0xC0
#define WS2812_1 0xFC

// Global Config
float g_brightness = 0.3; // Kept low for patterns
int g_speed = 4;

// --- The Pattern ---
// 1 = Rainbow Color, 0 = Off
// This is a simple 8x8 Heart shape
const uint8_t heart_pattern[8][8] = {
    {0,1,1,0,0,1,1,0},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,0},
    {0,0,1,1,1,1,0,0},
    {0,0,0,1,1,0,0,0},
    {0,0,0,0,0,0,0,0}
};

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
    if (fd < 0) { perror("SPI open failed"); return 1; }

    uint8_t leds[LED_COUNT * 3];
    uint8_t hue_offset = 0;

    while (1) {
        memset(leds, 0, sizeof(leds)); // Start with a blank frame

        for (int y = 0; y < LED_HEIGHT; y++) {
            for (int x = 0; x < LED_WIDTH; x++) {
                
                // Only light up the LED if the pattern has a '1' at this coordinate
                if (heart_pattern[y][x] == 1) {
                    uint8_t r, g, b;
                    // Calculate rainbow hue based on position and time
                    uint8_t hue = hue_offset + (x * 15);
                    hsv_to_rgb(hue, &r, &g, &b);

                    int i = (y * LED_WIDTH) + x;
                    leds[i * 3]     = g; // WS2812 GRB
                    leds[i * 3 + 1] = r; 
                    leds[i * 3 + 2] = b; 
                }
            }
        }

        transmit_leds(fd, leds);
        hue_offset += g_speed;
        usleep(30000); // ~33 FPS
    }

    close(fd);
    return 0;
}
