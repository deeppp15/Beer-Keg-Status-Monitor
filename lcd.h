#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
// #include <linux/i2c-dev.h>
#include <fcntl.h>
// #include<sys/ioctl.h>
#include<string.h>
#define I2C_BUS        "/dev/i2c-2" // I2C bus device
#define I2C_ADDR       0x27         // I2C slave address for the LCD module
#define BINARY_FORMAT  " %c  %c  %c  %c  %c  %c  %c  %c\n"
#define BYTE_TO_BINARY(byte) \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

static int32_t debug=0;
int32_t lcd_backlight;
static int32_t debug;
char address; 
int32_t i2cFile;
unsigned char i2c_ctrl(int32_t backLight,int32_t enable,  int32_t read_write, int32_t register_select);
void clearDisplay();
void i2c_init() ;
void i2c_stop();
void i2c_send_byte(unsigned char data) ;
int32_t i2c_msg(const char *str);
