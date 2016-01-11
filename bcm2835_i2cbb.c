/* ========================================================================== */
/*                                                                            */
/*   bcm2835.c                                                                */ 
/*   For Raspberry Pi August 2012                                             */
/*   http://www.byvac.com                                                     */
/*                                                                            */
/*   Description                                                              */
/*   This is a bit banged I2C driver that uses GPIO from user space           */
/*   There is much more control over the bus using this method and any pins   */
/*   can be used. The reason for this file is that the BCM hardware does      */
/*   not appear to support clock stretch                                      */  
/* ========================================================================== */
// Version 0.1 7/9/2012
// * removed a line of debug code

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "bcm2835.h"  //http://www.open.com.au/mikem/bcm2835/
#include "bcm2835_i2cbb.h"

#ifndef BCM2835_I2CBB_H
struct bcm2835_i2cbb {
    uint8_t address; // 7 bit address
    uint8_t sda; // pin used for sda coresponds to gpio
    uint8_t scl; // clock
    uint32_t clock_delay; // proporional to bus speed
    uint32_t timeout; //
};
#endif // BCM2835_I2CBB_H

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
        uint32_t timeout) // clock stretch & timeout
{
    bb->address = adr;
    bb->sda = data;
    bb->scl = clock;
    bb->clock_delay = speed;
    bb->timeout = timeout;
    if (!bcm2835_init())  {
	    fprintf(stderr, "bcm2835_i2cbb: Unable to open: %s\n", strerror(errno)) ;
    	return 1;
    }
    // also they should be set low, input - output determins level
    bcm2835_gpio_write(bb->sda, LOW);
    bcm2835_gpio_write(bb->scl, LOW);
    // both pins should be input at start
    bcm2835_gpio_fsel(bb->sda, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(bb->scl, BCM2835_GPIO_FSEL_INPT);
    return 0;
}

// *****************************************************************************
// bit delay, determins bus speed. nanosleep does not give the required delay
// its too much, by about a factor of 100
// This simple delay using the current Aug 2012 board gives aproximately:
// 500 = 50kHz. Obviously a better method of delay is needed.
// *****************************************************************************
void bcm2835_i2cbb_bitdelay(uint32_t del)
{
    while (del--);
}

// *****************************************************************************
// clock with stretch - bit level
// puts clock line high and checks that it does go high. When bit level
// stretching is used the clock needs checking at each transition
// *****************************************************************************
void bcm2835_i2cbb_sclH(struct bcm2835_i2cbb *bb)
{
    uint32_t to = bb->timeout;
    bcm2835_gpio_fsel(bb->scl, BCM2835_GPIO_FSEL_INPT); // high
    // check that it is high
    while(!bcm2835_gpio_lev(bb->scl)) {
        if(!to--) {
            fprintf(stderr, "bcm2835_i2cbb: Clock line held by slave\n");
            exit(1);
        }
    }
}
// *****************************************************************************
// other line conditions
// *****************************************************************************
void bcm2835_i2cbb_sclL(struct bcm2835_i2cbb *bb)
{
    bcm2835_gpio_fsel(bb->scl, BCM2835_GPIO_FSEL_OUTP);
}
void bcm2835_i2cbb_sdaH(struct bcm2835_i2cbb *bb)
{
    bcm2835_gpio_fsel(bb->sda, BCM2835_GPIO_FSEL_INPT);
}
void bcm2835_i2cbb_sdaL(struct bcm2835_i2cbb *bb)
{
    bcm2835_gpio_fsel(bb->sda, BCM2835_GPIO_FSEL_OUTP);
}

// *****************************************************************************
// Returns 1 if bus is free, i.e. both sda and scl high
// *****************************************************************************
int bcm2835_i2cbb_free(struct bcm2835_i2cbb *bb)
{
    return(bcm2835_gpio_lev(bb->sda) && bcm2835_gpio_lev(bb->scl));
}

// *****************************************************************************
// Start condition
// This is when sda is pulled low when clock is high. This also puls the clock
// low ready to send or receive data so both sda and scl end up low after this.
// *****************************************************************************
int bcm2835_i2cbb_start(struct bcm2835_i2cbb *bb)
{
    uint32_t to = bb->timeout;
    // bus must be free for start condition
    while(to--)
        if(bcm2835_i2cbb_free(bb)) break;

    if(to <=0) {
        fprintf(stderr, "bcm2835_i2cbb: Cannot set start condition\n");
        return 1;
    }

    // start condition is when data linegoes low when clock is high
    bcm2835_i2cbb_sdaL(bb);
    bcm2835_i2cbb_bitdelay((bb->clock_delay)/2);
    bcm2835_i2cbb_sclL(bb); // clock low now ready to send data
    bcm2835_i2cbb_bitdelay(bb->clock_delay);
    return 0;
}

// *****************************************************************************
// re-start condition, SDA goes low when SCL is high
// then set SCL low in prep for sending data
// for restart the SCL line is low already so this needs setting high first
// *****************************************************************************
int bcm2835_i2cbb_restart(struct bcm2835_i2cbb *bb)
{
    uint32_t to = bb->timeout;
    // bus must be free for start condition
    while(to--)
        if(bcm2835_i2cbb_free(bb)) break;

    if(to <=0) {
        fprintf(stderr, "bcm2835_i2cbb: Cannot set re-start condition\n");
        return 1;
    }

    // start condition is when data linegoes low when clock is high
    bcm2835_i2cbb_sdaL(bb);
    bcm2835_i2cbb_bitdelay(bb->clock_delay);
    bcm2835_i2cbb_sclL(bb); // clock low now ready to send data
    bcm2835_i2cbb_sdaH(bb);
    bcm2835_i2cbb_bitdelay(bb->clock_delay);
    return 0;
}

// *****************************************************************************
// stop condition
// when the clock is high, sda goes from low to high
// *****************************************************************************
void bcm2835_i2cbb_stop(struct bcm2835_i2cbb *bb)
{
    bcm2835_i2cbb_sdaL(bb); // needs to be low for this
    bcm2835_i2cbb_bitdelay(bb->clock_delay);
    bcm2835_i2cbb_sclH(bb); // clock will be low from read/write, put high
    bcm2835_i2cbb_bitdelay(bb->clock_delay);
    bcm2835_i2cbb_sdaH(bb); // release bus
}

// *****************************************************************************
// sends a byte to the bus, this is an 8 bit unit so could be address or data
// msb first
// returns 1 for NACK and 0 for ACK (0 is good)
// *****************************************************************************
int bcm2835_i2cbb_send(struct bcm2835_i2cbb *bb, uint8_t value)
{
    uint32_t rv;
    uint8_t j, mask=0x80;

    // clock is already low from start condition
    for(j=0;j<8;j++) {
        bcm2835_i2cbb_bitdelay(bb->clock_delay);
        if(value & mask) bcm2835_i2cbb_sdaH(bb);
        else bcm2835_i2cbb_sdaL(bb);
        // clock out data
        bcm2835_i2cbb_sclH(bb);  // clock it out
        bcm2835_i2cbb_bitdelay(bb->clock_delay);
        bcm2835_i2cbb_sclL(bb);      // back to low so data can change
        mask>>= 1;      // next bit along
    }
    // release bus for slave ack or nack
    bcm2835_i2cbb_sdaH(bb);
    bcm2835_i2cbb_bitdelay(bb->clock_delay);
    bcm2835_i2cbb_sclH(bb);     // and clock high tels slave to NACK/ACK
    bcm2835_i2cbb_bitdelay(bb->clock_delay); // delay for slave to act
    rv=bcm2835_gpio_lev(bb->sda);     // get ACK, NACK from slave
    bcm2835_i2cbb_sclL(bb);     // low to keep hold of bus as start condition
//    bcm2835_i2cbb_bitdelay(bb->clock_delay);
//    bcm2835_i2cbb_scl(bb, 1);     // idle state ready for stop or start
    return rv;
}

// *****************************************************************************
// receive 1 char from bus
// Input
// send: 1=nack, (last byte) 0 = ack (get another)
// *****************************************************************************
uint8_t bcm2835_i2cbb_read(struct bcm2835_i2cbb *bb, uint8_t ack)
{
    uint8_t j, data=0;

    for(j=0;j<8;j++) {
        data<<= 1;      // shift in
        bcm2835_i2cbb_bitdelay(bb->clock_delay);
        bcm2835_i2cbb_sclH(bb);;      // set clock high to get data
        bcm2835_i2cbb_bitdelay(bb->clock_delay); // delay for slave
        if(bcm2835_gpio_lev(bb->sda)) data++;   // get data
        bcm2835_i2cbb_sclL(bb);  // clock back to low
    }

   // clock has been left low at this point
   // send ack or nack
   bcm2835_i2cbb_bitdelay(bb->clock_delay);
   if(ack) bcm2835_i2cbb_sdaH(bb);
   else bcm2835_i2cbb_sdaL(bb);
   bcm2835_i2cbb_bitdelay(bb->clock_delay);
   bcm2835_i2cbb_sclH(bb);    // clock it in
   bcm2835_i2cbb_bitdelay(bb->clock_delay);
   bcm2835_i2cbb_sclL(bb); // bak to low
   bcm2835_i2cbb_sdaH(bb);      // release data line
   return data;
}

// =============================================================================
// Common scenarios for I2C
// =============================================================================
// *****************************************************************************
// writes just one byte to the given address
// *****************************************************************************
void bcm8235_i2cbb_putc(struct bcm2835_i2cbb *bb, uint8_t value)
{
    bcm2835_i2cbb_start(bb);
    bcm2835_i2cbb_send(bb, bb->address * 2); // address
    bcm2835_i2cbb_send(bb, value);
    bcm2835_i2cbb_stop(bb); // stop    
}
// *****************************************************************************
// writes buffer
// *****************************************************************************
void bcm8235_i2cbb_puts(struct bcm2835_i2cbb *bb, uint8_t *s, uint32_t len)
{
    bcm2835_i2cbb_start(bb);
    bcm2835_i2cbb_send(bb, bb->address * 2); // address
    while(len) {
        bcm2835_i2cbb_send(bb, *(s++));
        len--;
    }
    bcm2835_i2cbb_stop(bb); // stop    
}
// *****************************************************************************
// read one byte
// *****************************************************************************
uint8_t bcm8235_i2cbb_getc(struct bcm2835_i2cbb *bb)
{
    uint8_t rv;
    bcm2835_i2cbb_start(bb);
    bcm2835_i2cbb_send(bb, (bb->address * 2)+1); // address
    rv = bcm2835_i2cbb_read(bb, 1);
    bcm2835_i2cbb_stop(bb); // stop
    return rv;    
}
// *****************************************************************************
// reads into buffer
// *****************************************************************************
void bcm8235_i2cbb_gets(struct bcm2835_i2cbb *bb, uint8_t *buf, uint32_t len)
{
    uint8_t *bp = buf;
    bcm2835_i2cbb_start(bb);
    bcm2835_i2cbb_send(bb, (bb->address * 2)+1); // address
    while(len) {
        if(len == 1) {
            *(bp++) = bcm2835_i2cbb_read(bb, 1);
            *bp = 0; // in case its a string
            break;
        }                        
        *(bp++) = bcm2835_i2cbb_read(bb, 0);
        len--;
    }
    bcm2835_i2cbb_stop(bb); // stop    
}

// *****************************************************************************
// from i2c-tools but this will safely scan the bus
// *****************************************************************************
void bcm8235_i2cbb_discover(struct bcm2835_i2cbb *bb, uint8_t first, uint8_t last)
{
	int i, j;
	int res;

	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");

	for (i = 0; i < 128; i += 16) {
		printf("%02x: ", i);
		for(j = 0; j < 16; j++) {
			fflush(stdout);

			/* Skip unwanted addresses */
			if (i+j < first || i+j > last) {
				printf("   ");
				continue;
			}
			
            // odd 8 bit addresses are not shown as these are the read
            // address for the shown 7 bit address.	
            if((i+j) & 1)
			         printf("-- ");
            else {         	
    			bcm2835_i2cbb_start(bb);
	       		if(!bcm2835_i2cbb_send(bb, i+j)) {
		      	    printf("%02x ", (i+j)/2); // display 7 bit address
			     } else
			         printf("-- ");
                bcm2835_i2cbb_stop(bb);
            }
		}
		printf("\n");
	}
}

#ifndef BCM2835_I2CBB_H

// *****************************************************************************
// example for reading device ID
// address is the 7 bit address
// *****************************************************************************
int main(int argc, char **argv)
{
    uint8_t buf[64], j;
    struct bcm2835_i2cbb ibb;
    // address, sda,scl,clock_freq,timeout
    if(bcm2835_i2cbb_open(&ibb,0x21,17,21,300,1000000)) exit(1);
    
    // discover addresses on bus
    bcm8235_i2cbb_discover(&ibb, 0x10, 0xe0);
    
    // most devices have a command 0x55, this will return an incrementing
    // count
//    &ibb.address = 0x21; // change address if required 
//    bcm8235_i2cbb_putc(&ibb, 0x55);
//    bcm8235_i2cbb_gets(&ibb, buf, 4);
//    for(j=0;j<4;j++) {
//        printf("read %x\n",buf[j]);
//    }

    // BV4208 & BV4218 I2C LCD display
    // Clear screen and print Hello
//    ibb.address = 0x21; // address
//    buf[0]=1; // lcd command
//    buf[1]=1; // clear screen
//    bcm8235_i2cbb_puts(&ibb, buf, 2);
//    sleep(1); // cls takes a while
//    strcpy(buf,"\2Hello"); // 2 is command for writing data
//    bcm8235_i2cbb_puts(&ibb, buf, strlen(buf)); 
  
    // Relay BV4502      
//    ibb.address = 0x31; // address
//    buf[0]=1; // relay A
//    buf[1]=1; // on
//    bcm8235_i2cbb_puts(&ibb, buf, 2);
//    sleep(3);
//    buf[1]=0; // off
//    bcm8235_i2cbb_puts(&ibb, buf, 2);

    // ADC BV4205
//    ibb.address = 0x31; // address
    // set auto scan
//    buf[0]=6; // autoscan command
//    buf[1]=1; // on
//    bcm8235_i2cbb_puts(&ibb, buf, 2);
//    buf[0] = 7; // autoscan read
//    buf[1] = 0; // start at 0
    // get all 20 readings
//    bcm8235_i2cbb_puts(&ibb, buf, 2); // auto scan
//    bcm8235_i2cbb_gets(&ibb, buf, 20);

//    uint16_t v;
//    for(j=0;j<20;j+=2) {
//        v = (buf[j]*256) + buf[j]+1;
//        printf("\n A%d %u ",j,v);
//    }  

    exit(0);
}
#endif // BCM2835_I2CBB_H
