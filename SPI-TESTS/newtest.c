#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define SPI_PATH "/dev/spidev0.0"
#define BUFFER_SIZE 32

int main() {
    int fd;
    uint8_t tx_buffer[BUFFER_SIZE] = "Hello Luckfox SPI Loopback!";
    uint8_t rx_buffer[BUFFER_SIZE] = {0}; // Initialize receive buffer
    
    // ... (rest of the SPI configuration code as per the Wiki, using ioctl) ...

    struct spi_ioc_transfer transfer = {
        .tx_buf = (unsigned long)tx_buffer,
        .rx_buf = (unsigned long)rx_buffer,
        .len = sizeof(tx_buffer),
        .delay_usecs = 0,
        .speed_hz = 1000000, // Example speed
        .bits_per_word = 8,
    };

    fd = open(SPI_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        return EXIT_FAILURE;
    }

    // Perform SPI transfer
    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);
    if (ret < 0) {
        perror("Failed to perform SPI transfer");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    printf("Sent: %s\n", tx_buffer);
    printf("Received: %s\n", rx_buffer);

    // Add logic to compare tx_buffer and rx_buffer to verify loopback
    if (strcmp((const char*)tx_buffer, (const char*)rx_buffer) == 0) {
        printf("Loopback test successful!\n");
    } else {
        printf("Loopback test failed, data mismatch.\n");
    }

    return EXIT_SUCCESS;
}
