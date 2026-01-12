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

// WS2812B "Bit" encoding via SPI bytes
// At ~6.4MHz, 0xC0 (~25% duty) is '0' and 0xFC (~75% duty) is '1'
#define WS2812_0 0xC0
#define WS2812_1 0xFC

// --- Helper: HSV to RGB ---
// Hue is 0-255, Saturation 255, Value (Brightness) 255
void hsv_to_rgb(uint8_t h, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;
    uint8_t q = 255 - remainder;
    uint8_t t = remainder;

    switch (region) {
        case 0:  *r = 255; *g = t;   *b = 0;   break;
        case 1:  *r = q;   *g = 255; *b = 0;   break;
        case 2:  *r = 0;   *g = 255; *b = t;   break;
        case 3:  *r = 0;   *g = q;   *b = 255; break;
        case 4:  *r = t;   *g = 0;   *b = 255; break;
        default: *r = 255; *g = 0;   *b = q;   break;
    }
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
        .len = buffer_size,
        .speed_hz = 6400000,
        .bits_per_word = 8,
        .delay_usecs = 50, // Latch time (Reset pulse)
    };

    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    free(spi_buf);
}

int main() {
    int fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) { perror("Can't open SPI device"); return 1; }

    uint8_t leds[LED_COUNT * 3] = {0};
    uint8_t hue_offset = 0;

    printf("Starting Effects on 8x8 Grid... Press Ctrl+C to stop.\n");

    while (1) {
        // --- Effect 1: Rainbow Wave ---
        // Change the '10' and '5' to adjust how "stretched" the rainbow is
        for (int i = 0; i < LED_COUNT; i++) {
            uint8_t r, g, b;
            uint8_t hue = hue_offset + (i * 5); 
            hsv_to_rgb(hue, &r, &g, &b);

            // WS2812B expects G-R-B order
            leds[i * 3]     = g; 
            leds[i * 3 + 1] = r; 
            leds[i * 3 + 2] = b; 
        }

        /* // --- Effect 2: Global Fade (Uncomment to use) ---
        uint8_t fr, fg, fb;
        hsv_to_rgb(hue_offset, &fr, &fg, &fb);
        for (int i = 0; i < LED_COUNT; i++) {
            leds[i*3] = fg; leds[i*3+1] = fr; leds[i*3+2] = fb;
        }
        */

        transmit_leds(fd, leds);
        
        hue_offset += 2;      // Speed of the color cycle
        usleep(20000);        // ~50 FPS
    }

    close(fd);
    return 0;
}
