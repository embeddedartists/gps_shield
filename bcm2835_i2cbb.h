#ifndef BCM2835_I2CBB_H
#define BCM2835_I2CBB_H

struct bcm2835_i2cbb {
    uint8_t address; // 7 bit address
    uint8_t sda; // pin used for sda coresponds to gpio
    uint8_t scl; // clock
    uint32_t clock_delay; // proporional to bus speed
    uint32_t timeout; //
};

// *****************************************************************************
// open bus, sets structure and initialises GPIO
// The scl and sda line are set to be always 0 (low) output, when a high is
// required they are set to be an input.
// *****************************************************************************
int bcm2835_i2cbb_open(struct bcm2835_i2cbb *bb,
        uint8_t adr, // 7 bit address
        uint8_t data,   // GPIO pin for data 
        uint8_t clock,  // GPIO pin for clock
        uint32_t speed, // clock delay 250 = 100KHz 500 = 50KHz (apx)
        uint32_t timeout); // clock stretch & timeout

// =============================================================================
// Common scenarios for I2C
// =============================================================================
// *****************************************************************************
// writes just one byte to the given address
// *****************************************************************************
void bcm8235_i2cbb_putc(struct bcm2835_i2cbb *bb, uint8_t value);

// *****************************************************************************
// writes buffer
// *****************************************************************************
void bcm8235_i2cbb_puts(struct bcm2835_i2cbb *bb, uint8_t *s, uint32_t len);

// *****************************************************************************
// read one byte
// *****************************************************************************
uint8_t bcm8235_i2cbb_getc(struct bcm2835_i2cbb *bb);

// *****************************************************************************
// reads into buffer
// *****************************************************************************
void bcm8235_i2cbb_gets(struct bcm2835_i2cbb *bb, uint8_t *buf, uint32_t len);


#endif // BCM2835_I2CBB_H
