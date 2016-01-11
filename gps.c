#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "bcm2835_i2cbb.h"

#define MAX_GPS_MSG_LEN 1024


static void gps_delay_ms(uint32_t millis) {
   struct timespec ts;
   ts.tv_sec = millis/1000;
   ts.tv_nsec = (millis%1000)*1000000;
   nanosleep(&ts, NULL);
}


static void gps_process_message(const char* msg, int len) {
   const char* chk = strchr(msg, '*');
   if ((chk == NULL) || (msg[0] != '$')) {
      printf("Got message '%s'\n", msg);
      printf("-- Invalid, missing *\n");
   } else {
      uint32_t expected = strtoul(chk+1, NULL, 16);
      uint32_t chksum = 0;
      const char* x = msg+1; //skip '$'
      while (x < chk) {
         chksum ^= (uint8_t)*x;
         x++;
      }
      if (chksum != expected) {
         printf("Got message '%s'\n", msg);
         printf("-- Invalid checksum, got 0x%x but expected 0x%x\n", chksum, expected);
      } else {
         printf("Got message '%s'\n", msg);
         if (strncmp("$GPGGA", msg, 6) == 0) {
            // Global positioning system fix data
         } else if (strncmp("$GPGGA", msg, 6) == 0) {
            // Global positioning system fix data
         }
         //...
      }
   }
}

static int  gps_read_message(struct bcm2835_i2cbb *bb, char* msg) {
   int num;
   uint u;
   for (num = 0; num < MAX_GPS_MSG_LEN; num++) {
      uint8_t u = bcm8235_i2cbb_getc(bb);
      msg[num] = (char)u;
      if (u == 0xff) {
         // bad
         num = 0;
         break;
      } else if (u == '\n') {
         // end of crlf => parse message
         if ((num > 0) && (msg[num-1] == '\r')) {
            num--;
         }
         break;
      }
   }
   msg[num] = '\0';
   return num;
}

int main(int argc, char **argv)
{
   char msg[MAX_GPS_MSG_LEN+1];
   struct bcm2835_i2cbb ibb;
   uint32_t speed = 250;
   uint32_t kbit = 100;

   if (argc > 1) {
      kbit = atoi(argv[1]);
      switch (kbit) {
      case  50: speed = 500; break; // 500 gives roughly  50Kbit/s
      case 100: speed = 250; break; // 250 gives roughly 100Kbit/s
      case 200: speed = 100; break; // 100 gives roughly 200Kbit/s
      case 300: speed =  50; break; //  50 gives roughly 300Kbit/s
      case 400: speed =  23; break; //  23 gives roughly 400Kbit/s
      default:
         printf("Bad speed '%s'=%u, should be 50,100,200,300 or 400\n", argv[1], kbit);
         exit(1);
      }
   }
   printf("Using speed ~ %u Kbit/s\n", kbit);

   // address, sda,scl,clock_freq,timeout
   if(bcm2835_i2cbb_open(&ibb,0x42,2,3,speed,1000000)) {
      printf("Failed to initialize bitbanged I2C. Aborting...\n");
      exit(1);
   }

   while(1) {
      int len = gps_read_message(&ibb, msg);
      if (len > 0) {
         gps_process_message(msg, len);
      }
      gps_delay_ms(100);
   }

   exit(0);
}

