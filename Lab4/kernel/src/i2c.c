#include <gpio.h>
#include <i2c.h>
#include <unistd.h>
#include <rcc.h>
#include <printk.h>

/** @brief The I2C register map. */
struct i2c_reg_map {
    volatile uint32_t CR1; /**< Control Register 1 */
    volatile uint32_t CR2; /**< Control Register 2 */
    volatile uint32_t OAR1; /**< Own Address Register 1 */
    volatile uint32_t OAR2; /**< Own Address Register 2 */
    volatile uint32_t DR; /**< Data Register */
    volatile uint32_t SR1; /**< Status Register 1 */
    volatile uint32_t SR2; /**< Status Register 2 */
    volatile uint32_t CCR; /**< Clock Control Register */
    volatile uint32_t TRISE; /**< TRISE Register */
    volatile uint32_t FLTR; /**< FLTR Register */
};

/** @brief base address for I2C */
#define I2C_BASE (struct i2c_reg_map *) 0x40005400

/** @brief Enable Bit for I2C_1 clock in APB*/
#define RCC_EN (1 << 21)

/** @brief I2C's value for APB start clock (16MHz) */
#define APB_16M (1 << 4)

/** @brief I2C set slave mode */
#define SM (1 << 15)

/** @brief I2C slave mode set 7bit addr mode*/
#define ADDMODE (1 << 15)

/** @brief I2C peripheral enanle */
#define PE 1


/** @brief SR1 value to wait for slave to recv start*/
#define SB 1

/** @brief Start generation I2C master*/
#define START (1<<8)

/** @brief Stop generation I2C master*/
#define STOP (1<<9)


/** @brief SR1 value to wait for TX to finish*/
#define TXE (1<<7)

/** @brief SR1 value to wait for RX reg to be empty*/
#define RXNE (1<<6)

/** @brief SR1 value to wait for slave to recv addr*/
#define ADDR (1<<1)

/** @brief CR1 value to wait for slave to recv addr*/
#define ACK (1<<10)

/** @brief TRISE value of max rise time is 16+1*/
#define TRISE_17 0x11

void i2c_master_init(uint16_t clk){
    struct rcc_reg_map *rcc= RCC_BASE;
    struct i2c_reg_map *i2c= I2C_BASE;

    gpio_init(GPIO_B, 8, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW,
            PUPD_PULL_UP, ALT4); //init SCL I2C2 PB_10
    gpio_init(GPIO_B, 9, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW,
            PUPD_PULL_UP, ALT4); //init SDA I2C2 PB_3

    rcc->apb1_enr|= RCC_EN;      //enable APB peripheral clk
    i2c->CR2= (i2c->CR2 & 0xffffffc0) | APB_16M; //I2C APB clk

    i2c->CCR&= ~SM;          //set Slave mode
    i2c->CCR= (i2c->CCR & 0xfffff800) | clk;

    i2c->TRISE= (i2c->TRISE & 0xffffffc0) | TRISE_17;

    i2c->OAR1&= ~ADDMODE;        //set 7bit addr mode
    i2c->CR1|= PE;               //enable I2C

    return;
}

void i2c_master_start(){
    struct i2c_reg_map *i2c= I2C_BASE;
    i2c->CR1|= START; //start generation send
    //wait for START ACK ; switch to master mode
    while(!(i2c->SR1 & SB));
    return;
}

void i2c_master_stop(){
    struct i2c_reg_map *i2c= I2C_BASE;
    i2c->CR1|= STOP; //stop generation send
    return;
}

int i2c_master_write(uint8_t *buf, uint16_t len, uint8_t slave_addr){
    struct i2c_reg_map *i2c= I2C_BASE;
    uint8_t slaveByte= (slave_addr << 1) | 0x0;
    uint8_t data, tmp;

    i2c_master_start();
    tmp= i2c->SR1;             //clears START generation bit
    i2c->DR= (i2c->DR & 0xffffff00) | slaveByte;

    while(!(i2c->SR1 & ADDR)); //wait for ADDR ACK
    tmp=i2c->SR1;
    tmp=i2c->SR2;              //clear ADDR bit

    /* send len bytes from data buffer*/
    int i=0;
    while(i<len){
        data= buf[i];
        i2c->DR= (i2c->DR & 0xffffff00) | data;
        while(!((tmp=i2c->SR1) & TXE)); //wait for ACK from slave
        i++;
    }

    i2c_master_stop();

    (void)tmp;
    return len;
}
