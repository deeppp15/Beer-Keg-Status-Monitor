// Resources used:
// https://elinux.org/EBC_Exercise_11b_gpio_via_mmap
// Shivani's email
// https://www.ti.com/lit/ug/spruh73q/spruh73q.pdf

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define GPIO_ADDRESS_BASE 0x4804C000  // Base address for GPIO1 registers
#define GPIO_END_ADDRESS 0x4804D000
// note: since these GPIOs are on GPIOChip1, bit number = (GPIO Number - 32)
// ex. GPIO 48 - 32 = 16th bit
#define PD_SCK_PIN 48                // GPIO number for PD_SCK
#define DOUT_PIN 60                  // GPIO number for DOUT
#define PD_SCK_BIT (1 << (PD_SCK_PIN - 32)) // GPIO bit number for PD_SCK (P9_15 or GPIO_48)
#define DOUT_BIT (1 << (DOUT_PIN - 32))     // GPIO bit number for DOUT (P9_12 or GPIO_60)
#define BLOCK_SIZE (GPIO_END_ADDRESS - GPIO_ADDRESS_BASE)
// used for clear and setting the value of output GPIO pins
#define GPIO_SETDATAOUT 0x194
#define GPIO_CLEARDATAOUT 0x190
// used for reading the values of GPIO pins with direction "in" or "out"
#define GPIO_DATAIN 0x138
#define GPIO_DATAOUT 0x13C
#define MAX_BUFFER_SIZE 50

// global constant for GPIO PATH on BeagleBone Black
static const char *GPIO_Path = (char *) "/sys/class/gpio/";

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    volatile unsigned int* gpio_addr = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_ADDRESS_BASE);
    char direction_path[MAX_BUFFER_SIZE] = {0};
    FILE *fp = NULL;

    // Set PD_SCK_PIN as output and DOUT_PIN as input
    snprintf(direction_path, MAX_BUFFER_SIZE, "%sgpio%d/direction", GPIO_Path, PD_SCK_PIN);
    fp = fopen(direction_path, "w");
    fprintf(fp, "out");
    fclose(fp);

    snprintf(direction_path, MAX_BUFFER_SIZE, "%sgpio%d/direction", GPIO_Path, DOUT_PIN);
    fp = fopen(direction_path, "w");
    fprintf(fp, "in");
    fclose(fp);

    // Configure other GPIO settings
    

    while (1) {
        unsigned long data = 0;
        for (int i = 0; i < 24; i++) {
            // Toggle PD_SCK high and low

            // Read bit from DOUT

            data = (data << 1) | READ_BIT_FROM_DOUT;
        }
        // Use data
        printf("Weight reading: %lu\n", data);
        sleep(1);  // Sleep for a second
    }

    munmap((void*)gpio_addr, BLOCK_SIZE);
    close(fd);
    return 0;
}
