#include <stdio.h>
#include "ownet.h"
#include "sht11.h"

#define MAXDEVICES         20

static void w1_make_serial(char * asc, unsigned char *bin)
{
    int i,j;
    for(i = j = 0; i < 8; i++)
    {
        bin[i] = ToHex(asc[j]) << 4; j++;
        bin[i] += ToHex(asc[j]); j++;
    }
}

static void TestScratch (int portnum, u_char *ident)
{
    int i;
    unsigned char c,buf[33];

    owTouchReset(portnum);
    owTouchByte(portnum, 0x55);
    for(i=0;i<8;i++)          
    {
        owTouchByte(portnum, ident[i]);
    }
    owTouchReset(0xBE);
    for(i=0;i<32;i++)
    {
        c = owTouchByte(portnum,0xff);
        buf[i] = c;
    }
    buf[32] = 0;
    for(i=0;i<32;i++)
    {
        printf("%02x ", buf[i]);
    }
    fputc('\n', stdout);
    puts((char *)buf);
    msDelay(100);
    owTouchReset(portnum);
}


int main(int argc, char **argv)
{
    int portnum = 0;
    u_char serial[8];
    float temp=0,rh=0;
    
    if (argc != 3)
    {
        fputs("humids device address\n"
              " Device: e.g. /dev/ttyS0 or DS2490-1\n"
              " Default is to read temp / humidity\n"
              " Environment variables\n"
              "  TEST_SHT11=1 humids device address : Read scratchpad\n"
              "  SHT11_VERBOSE=1 humids device address : More verbosity\n", stderr);
        exit(1);
        // e.g. /dev/ttyS0 or DS2490-1
   }
    /* attempt to acquire the 1-Wire Net */
    if((portnum = owAcquireEx(argv[1])) < 0)
    {
        OWERROR_DUMP(stdout);
        exit(1);
    }
    w1_make_serial(argv[2], serial);
    if(getenv("TEST_SHT11"))
    {
        TestScratch(portnum,serial);
    }
    else
    {
        if(ReadSHT11(portnum, serial, &temp, &rh))
        {
            printf("Temp %3.1f RH  %3.1f \n", temp, rh);
        }
    }
    return 0;
}

