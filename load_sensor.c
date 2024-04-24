// Resources used:
// https://elinux.org/EBC_Exercise_11b_gpio_via_mmap
// Help from GTA Shivani
// https://www.ti.com/lit/ug/spruh73q/spruh73q.pdf
// https://github.com/MarkAYoder/BeagleBoard-exercises/blob/d07dc7500beca6a0310f574a36025d23be362631/sensors/mmap/gpioToggle.c

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h> // for exit() used for testing

#define GPIO_ADDRESS_BASE 0x4804C000  // Base address for GPIO1 registers
#define GPIO_END_ADDRESS 0x4804D000
// note: since these GPIOs are on GPIOChip1, bit number = (GPIO Number - 32)
// ex. GPIO 48 - 32 = 16th bit
#define PD_SCK_PIN 48                // GPIO number for PD_SCK
#define DOUT_PIN 49                  // GPIO number for DOUT
#define PD_SCK_BIT (1 << (PD_SCK_PIN - 32)) // GPIO bit number for PD_SCK (P9_15 or GPIO_48)
#define DOUT_BIT (1 << (DOUT_PIN - 32))     // GPIO bit number for DOUT (P9_12 or GPIO_60)
#define BLOCK_SIZE (GPIO_END_ADDRESS - GPIO_ADDRESS_BASE)
// used for clear and setting the value of output GPIO pins
#define GPIO_SETDATAOUT 0x194
#define GPIO_CLEARDATAOUT 0x190
// used for reading the values of GPIO pins with direction "in" or "out"
#define GPIO_DATAIN 0x138
// used for setting a GPIO pin to output or input
#define GPIO_OE 0x134
#define WORD_SIZE 4

// global constant for GPIO PATH on BeagleBone Black
static const char *GPIO_Path = (char *) "/sys/class/gpio/";

int main() {
    int fd;
    volatile unsigned int *gpio_addr;
    volatile unsigned int *gpio_setdataout_addr;
    volatile unsigned int *gpio_cleardataout_addr;
    volatile unsigned int *gpio_datain_addr;
    volatile unsigned int *gpio_oe_addr;
    unsigned int mem;
    
    fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0) {
        perror("Failed to open /dev/mem");
        return -1;
    }

    gpio_addr = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_ADDRESS_BASE);

    if (gpio_addr == MAP_FAILED) {
        perror("Failed to mmap");
        close(fd);
        return -2;
    }

    // initialize necessary gpio memory addresses
    gpio_setdataout_addr = gpio_addr + GPIO_SETDATAOUT / WORD_SIZE;
    gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT / WORD_SIZE;
    gpio_datain_addr = gpio_addr + GPIO_DATAIN / WORD_SIZE;
    gpio_oe_addr = gpio_addr + GPIO_OE / WORD_SIZE;

    // Set PD_SCK_PIN as output and DOUT_PIN as input
    // note: dereference of gpio_oe_addr is used to get to the values of the physical address
    mem = *gpio_oe_addr;
    // printf("GPIO1 configuration: %X\n", mem);

    // set PD_SCK_PIN to output (0)
    mem &= ~PD_SCK_PIN;
    *gpio_oe_addr = mem;
    
    // // set DOUT_PIN to input (1)
    mem = *gpio_oe_addr;
    mem |= DOUT_BIT;
    *gpio_oe_addr = mem;

    // Configure other GPIO settings   
    // set SCK_PIN's value to 0
    *gpio_cleardataout_addr = PD_SCK_BIT;

    while (1) {
        unsigned long data = 0;
        int i;

        // spin while DOUT is HIGH
        printf("%X\n", *gpio_datain_addr & DOUT_BIT);
        while (*gpio_datain_addr & DOUT_BIT) {
            printf("hello\n");
        }

        for (i = 0; i < 24; i++) {
            // set PD_SCK high
            *gpio_setdataout_addr = PD_SCK_BIT;
            // short delay for signal stability
            usleep(1);

            // Read bit from DOUT
            // data = (data << 1) | READ_BIT_FROM_DOUT;
            // printf("%X\n", (*gpio_datain_addr & DOUT_BIT));
            data = (data << 1) | (*gpio_datain_addr & DOUT_BIT || 0);

            // set PD_SCK low
            *gpio_cleardataout_addr = PD_SCK_BIT;
            // short delay for signal stability
            usleep(1);
        }
        // Use data
        printf("Weight reading: %lu\n", data);
        sleep(1);  // Sleep for a second
    }

    munmap((void*)gpio_addr, BLOCK_SIZE);
    close(fd);
    return 0;
}
