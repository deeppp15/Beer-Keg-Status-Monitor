/* ----------------------------------------------------------------------- *
 * Title:         i2clcd                                                   *
 * Description:   C-code for PCF8574T backpack controlling LCD through I2C *
 *                Tested on BeagleBone AI                                  *
 *                11/1/2019 Sean Miller                                    *
 * ported from:                                                            *
 * https://github.com/fm4dd/i2c-lcd/blob/master/pcf8574-lcd-demo.c         *
 *                             *
 * Compilation:   i2clcd.c -o i2clcd                                       *
 *------------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include<sys/ioctl.h>
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
  
int lcd_backlight;
int debug;
char address; 
int i2cFile;
unsigned char ctrlRegisters(int backLight,int enable,  int read_write, int register_select);
void clearDisplay();
void i2c_first() ;
void i2c_stop();
void i2c_send_byte(unsigned char data) ;
void send_string(const char *str);
void start(const char *message) ;

void i2c_first() {
   if((i2cFile = open(I2C_BUS, O_RDWR)) < 0) {
      printf("Error failed to open I2C bus [%s].\n", I2C_BUS);
      exit(-1);
   }
   //set the I2C slave address for all subsequent I2C device transfers
   if (ioctl(i2cFile, I2C_SLAVE, I2C_ADDR) < 0) {
      printf("Error failed to set I2C address [%s].\n", I2C_ADDR);
      exit(-1);
   }
}


void i2c_stop() { close(i2cFile); }


void i2c_send_byte(unsigned char data) {
   unsigned char byte[1];
   byte[0] = data;
   if(debug) printf(BINARY_FORMAT, BYTE_TO_BINARY(byte[0]));
   printf("\n");
   write(i2cFile, byte, sizeof(byte)); 
   /* -------------------------------------------------------------------- *
    * Below wait creates 1msec delay, needed by display to catch commands  *
    * -------------------------------------------------------------------- */
   usleep(1000);
}

unsigned char ctrlRegisters(int backLight,int enable,  int read_write, int register_select){
    unsigned char byte = 0;
    byte |= (backLight << 3);  // Shift 'a' to the left by 3 positions (MSB)
    byte |= (enable << 2);  // Shift 'b' to the left by 2 positions
    byte |= (read_write << 1);  // Shift 'c' to the left by 1 position
    byte |= register_select;  
    return byte;
}

void send_string(const char *str) {
   int enable = 0;

    size_t length = strlen(str);
    for (size_t i = 0; i < length; ++i) {
      
      char mainChar= str[i];
      char data = (int)mainChar; 
   
      // Extract upper 4 bits and lower 4
      unsigned char upper_4_bits[2];
      upper_4_bits[0]= data >> 4;
      upper_4_bits[1]=(data & 0x0F);
     
      // Append additional bits to make it 8 bits
      int additional_bits = 4; // Example: Append 4 bits
      //Bit manipulation to to move 4 bits to the right and append the bits of data & 0xF
       enable ^= 1;
      unsigned char appended_value = (upper_4_bits[0] << additional_bits) |ctrlRegisters(1,enable,0, 1);// first time when enable is 1
      i2c_send_byte(appended_value);

      enable ^= 1;
      appended_value = (upper_4_bits[0] << additional_bits) |ctrlRegisters(1,enable,0, 1);//second time when enable is 0
      i2c_send_byte(appended_value);
      
      enable ^=1;
      //same bit manipulation of lower 4 bits and the ctrl data
      appended_value = (upper_4_bits[1] << additional_bits) | ctrlRegisters(1,enable,0, 1);//first time when enable is again 1
      i2c_send_byte(appended_value);
     
      enable ^=1;
      appended_value = (upper_4_bits[1] << additional_bits) | ctrlRegisters(1,enable,0, 1);//when enable is again 0
      i2c_send_byte(appended_value);

    }
}



void start(const char *message) { 
   i2c_first(); 
   debug=0; //change to 1 to see messages.


   /* -------------------------------------------------------------------- *
    * Initialize the display, using the 4-bit mode initialization sequence *
    * -------------------------------------------------------------------- */
   if(debug) printf("Init Start:\n");


   usleep(15000);             // wait 15msec
   i2c_send_byte(0b00110100); // D7=0, D6=0, D5=1, D4=1, RS,RW=0 EN=1
   i2c_send_byte(0b00110000); // D7=0, D6=0, D5=1, D4=1, RS,RW=0 EN=0
   usleep(4100);              // wait 4.1msec
   i2c_send_byte(0b00110100); // 
   i2c_send_byte(0b00110000); // same
   usleep(100);               // wait 100usec
   i2c_send_byte(0b00110100); //
   i2c_send_byte(0b00110000); // 8-bit mode init complete
   usleep(4100);              // wait 4.1msec
   i2c_send_byte(0b00100100); //
   i2c_send_byte(0b00100000); // switched now to 4-bit mode


   /* -------------------------------------------------------------------- *
    * 4-bit mode initialization complete. Now configuring the function set *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00100100); //
   i2c_send_byte(0b00100000); // keep 4-bit mode
   i2c_send_byte(0b10000100); //
   i2c_send_byte(0b10000000); // D3=2lines, D2=char5x8


   /* -------------------------------------------------------------------- *
    * Next turn display off                                                *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00000100); //
   i2c_send_byte(0b00000000); // D7-D4=0
   i2c_send_byte(0b10000100); //
   i2c_send_byte(0b10000000); // D3=1 D2=display_off, D1=cursor_off, D0=cursor_blink


   /* -------------------------------------------------------------------- *
    * Display clear, cursor home                                           *
    * -------------------------------------------------------------------- */
      clearDisplay();
   /* -------------------------------------------------------------------- *
    * Set cursor direction                                                 *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00000100); //
   i2c_send_byte(0b00000000); // D7-D4=0
   i2c_send_byte(0b01100100); //
   i2c_send_byte(0b01100000); // print left to right


   /* -------------------------------------------------------------------- *
    * Turn on the display                                                  *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00000100); //
   i2c_send_byte(0b00000000); // D7-D4=0
   i2c_send_byte(0b11100100); //
   i2c_send_byte(0b11100000); // D3=1 D2=display_on, D1=cursor_on, D0=cursor_blink


   if(debug) printf("Init End.\n");
   sleep(1);


   if(debug) printf("Writing H to display\n");
   if(debug) printf("D7 D6 D5 D4 BL EN RW RS\n");

   //const char *message = "Temp:29C Wght:10kg";
   send_string(message);

   if(debug) printf("Finished writing to display.\n");
   i2c_stop(); 
}


void clearDisplay(){
   /* -------------------------------------------------------------------- *
    * Display clear, cursor home                                           *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00000100); //
   i2c_send_byte(0b00000000); // D7-D4=0
   i2c_send_byte(0b00010100); //
   i2c_send_byte(0b00010000); // D0=display_clear
}
