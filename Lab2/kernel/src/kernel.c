#include <adc.h>
#include <gpio.h>
#include <i2c.h>
#include <printk.h>
#include <uart_polling.h>
#include <unistd.h>
#include <led_driver.h>
#include <stdlib.h>

/**@brief clap detection threshold */
#define threshold 190


/**@brief clap detection processing */
uint32_t avgClap(uint16_t numSamples);


int kernel_main() {
    uart_polling_init(0);
    i2c_master_init(0x50);
    led_driver_init();
    adc_init();

    uint8_t userInput,i;
    uint32_t val;

    while(1){
        printk("Enter a number from 0-9:\n");
        userInput= uart_polling_get_byte();
        uart_polling_put_byte(userInput);
        printk("\n");

        if(userInput == '0'){
            val= adc_read_pin(8);
            led_set_display(val);
        }else if(userInput > '0' && userInput <='9'){
            i= userInput-0x30; //use ASCII offset to get int

            while(i){
                if((val=avgClap(500)) > (uint32_t)threshold){
                    printk("Clap detected!\n",val);
                    i--;
                }
            }
            printk("Done!\n");
        }else{
            printk("Invalid Input: %c\n", userInput, val);
        }
    }
    return 0;
}

uint32_t avgClap(uint16_t numSamples){
    uint8_t val;
    uint32_t avg =0;
    int i=0;

    while(i<numSamples){
        val= adc_read_pin(11);
        i++;
        avg += val;
    }
    return avg/numSamples;
}
