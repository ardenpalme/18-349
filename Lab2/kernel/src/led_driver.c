#include <i2c.h>
#include <led_driver.h>
#include <unistd.h>

/* converts hexadecimal numbers to LCD RAM byte equivalents */
uint8_t hex_to_seven_segment(uint8_t hex);

/** @brief Fill 4 LED segments digits
 *  @param [in] digit byte array
 *  @param [in] 4 digit number to convert
 *  */
void fillDigits(uint32_t input, uint8_t *digits);

/** @brief Initialize LCD segment for display */
void led_driver_init(){
    uint8_t buf[17];
    uint8_t ht16k33_addr= 0b01110000;
    buf[0]= 0x2f; //Oscillator
    buf[1]= 0xae; // ROW/INT Set
    buf[2]= 0xe7; //Dimming Set
    buf[3]= 0x88; //No Blinking, display off

    //send initialization bytes with START/STOP after each
    i2c_master_write(buf, 1, ht16k33_addr);
    i2c_master_write(&buf[1], 1, ht16k33_addr);
    i2c_master_write(&buf[2], 1, ht16k33_addr);
    i2c_master_write(&buf[3], 1, ht16k33_addr);

    /* Clear RAM*/
    int i;
    for(i=0;i<16;i++){
        buf[i]= 0x0;
    }
    i2c_master_write(buf, 16, ht16k33_addr);


    buf[0]= 0x89; // display on
    i2c_master_write(buf, 1, ht16k33_addr);

    return;
}

void fillDigits(uint32_t input, uint8_t *digits){
    //count digits
    int ct;
    if(input < 9) ct= 1;
    else if(input < 99) ct= 2;
    else if(input < 999) ct= 3;
    else ct= 4;

    //fill digits
    int j=0;
    int i= input;
    while(j < ct){
        digits[3-j]= (i%10);
        i/=10;
        j++;
    }
}

/** @brief Displays a 4-digit number on LCD 7-seg display
 *  @param[in] 4 digit postive int input
 *  */
void led_set_display(uint32_t input){
    uint8_t ht16k33_addr= 0b01110000;
    uint8_t buf[17];

    //initialize digits based on input
    uint8_t digits[4]= {-1, -1, -1, -1};
    fillDigits(input, digits);

    buf[0]= 0x0;
    uint8_t tmp;
    int i;
    for(i=1;i<16;i++){
        //only certain bytes in RAM correspond to LCD digits
        switch(i){
            case 1:
                tmp=hex_to_seven_segment(digits[0]);
                break;
            case 3:
                tmp=hex_to_seven_segment(digits[1]);
                break;
            case 7:
                tmp=hex_to_seven_segment(digits[2]);
                break;
            case 9:
                tmp=hex_to_seven_segment(digits[3]);
                break;
            default:
                tmp=hex_to_seven_segment(i);
                break;
        }
        buf[i]= tmp;
    }
    i2c_master_write(buf, 16, ht16k33_addr); //write entire 16x8 RAM

    buf[0]= 0x89; // display on
    i2c_master_write(buf, 1, ht16k33_addr);
    return;
}

uint8_t hex_to_seven_segment(uint8_t hex){
  uint8_t result;
  switch (hex){
    case 0x0:
      result = 0b00111111;
      break;
    case 0x1:
      result = 0b00000110;
      break;
    case 0x2:
      result = 0b01011011;
      break;
    case 0x3:
      result = 0b01001111;
      break;
    case 0x4:
      result = 0b01100110;
      break;
    case 0x5:
      result = 0b01101101;
      break;
    case 0x6:
      result = 0b01111101;
      break;
    case 0x7:
      result = 0b00000111;
      break;
    case 0x8:
      result = 0b01111111;
      break;
    case 0x9:
      result = 0b01101111;
      break;
    case 0xA:
      result = 0b01110111;
      break;
    case 0xB:
      result = 0b01111100;
      break;
    case 0xC:
      result = 0b00111001;
      break;
    case 0xD:
      result = 0b01011110;
      break;
    case 0xE:
      result = 0b01111001;
      break;
    case 0xF:
      result = 0b01110001;
      break;
    default:
      result = 0;
  }
  return result;
}
