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
int main(int argc, char **argv)
{
    int portnum = 0;
    u_char serial[8];
    float temp=0,rh=0;
    
    if (argc != 3)
    {
        fputs("humsens device address\n", stderr);
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
    if(ReadSHT11(portnum, serial, &temp, &rh))
    {
        printf("Temp %3.1f RH  %3.1f \n", temp, rh);
    }
    return 0;
}

