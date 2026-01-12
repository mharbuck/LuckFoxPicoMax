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

#define WS2812_0 0xC0
#define WS2812_1 0xFC

// Global configuration variables
float g_brightness = 0.5;
int g_speed = 2;

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

void print_usage(char *prog_name) {
    printf("Usage: %s [-s speed] [-b brightness]\n", prog_name);
    printf("  -s : Speed of animation (1-20, default 2)\n");
    printf("  -b : Brightness (0.0 to 1.0, default 0.5)\n");
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "s:b:h")) != -1) {
        switch (opt) {
            case 's': g_speed = atoi(optarg); break;
            case 'b': g_brightness = atof(optarg); break;
            case 'h': print_usage(argv[0]); return 0;
            default: print_usage(argv[0]); return 1;
        }
    }

    int fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) { perror("Can't open SPI device"); return 1; }

    uint8_t leds[LED_COUNT * 3] = {0};
    uint8_t hue_offset = 0;

    printf("Running: Speed=%d, Brightness=%.1f\n", g_speed, g_brightness);

    while (1) {
        for (int i = 0; i < LED_COUNT; i++) {
            uint8_t r, g, b;
            // The '5' here determines the color spread across the grid
            uint8_t hue = hue_offset + (i * 5); 
            hsv_to_rgb(hue, &r, &g, &b);

            leds[i * 3]     = g; 
            leds[i * 3 + 1] = r; 
            leds[i * 3 + 2] = b; 
        }

        transmit_leds(fd, leds);
        
        hue_offset += g_speed;      
        usleep(20000); // Maintain ~50 FPS
    }

    close(fd);
    return 0;
}
