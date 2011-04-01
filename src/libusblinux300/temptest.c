#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ownet.h"
#include "temp10.h"
#include "findtype.h"
#include "swt1f.h"
#include <unistd.h>

#define TEMPMSECS 1000

static void w1_make_serial(char * asc, unsigned char *bin)
{
    int i,j;
    for(i = j = 0; i < 8; i++)
    {
        bin[i] = ToHex(asc[j]) << 4; j++;
        bin[i] += ToHex(asc[j]); j++;
    }
}

static void w1_set_coupler(int portnum, uchar *ident, int line)
{
  uchar a[4];
  SetSwitch1F(portnum, ident, line, 2, a, TRUE);
}

static int w1_select_device(int portnum, uchar *serno, uchar *coupler, int line)
{
    u_char thisdev[8];
    int found = 0;
    
    if(coupler)
    {
        w1_set_coupler(portnum, coupler, 0);
        w1_set_coupler(portnum, coupler, line);
    }

    owFamilySearchSetup(portnum,serno[0]);
    while (owNext(portnum,TRUE, FALSE)){
      owSerialNum(portnum, thisdev, TRUE);
      if(memcmp(thisdev, serno, sizeof(thisdev)) == 0){
	found = 1;
	break;
      }
    }
    return found;
}

int main(int argc, char **argv)
{
   float current_temp;
   int portnum = 0;
   char *serial = NULL, *dev=NULL;
   u_char ident[8];
   char *coupler=NULL;
   u_char *cident=NULL;
   int c;
   int bra=0;

   while((c = getopt(argc, argv, "amc:s:")) != EOF)
   {
       switch (c)
       {
           case 'a':
               bra = 2;
               break;
           case 'm':
               bra = 4;
               break;
           case 's':
               serial = strdup(optarg);
               break;
           case 'c':
               coupler = strdup(optarg);
               break;
         default:
               break;
       }
   }

   dev = argv[optind];
   if (dev == NULL || serial == NULL)
   {
       fputs("usage: temptest -s serial [-c coupler -a|-m ]   device\n"
             "       device => /dev/ttyS0 or DS2490-1\n", stderr);
       return 0;
   }

   if((portnum = owAcquireEx(dev)) < 0)
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   w1_make_serial(serial, ident);
   if (coupler)
   {
       cident = malloc(32);
       w1_make_serial(coupler, cident);
   }
   if (!w1_select_device(portnum, ident, cident, bra))
   {
       fputs("No dev\n", stderr);
       exit(1);
   }
   
   if (ReadTemperature(portnum, ident, &current_temp, TEMPMSECS))
   {
       PrintSerialNum(ident);
       printf(" %5.1fC  %5.1fF \n", current_temp,
              current_temp * 9 / 5 + 32);
           // converting temperature from Celsius to Fahrenheit
   }
   else
   {
       printf("     Error reading temperature, verify device present:%d\n",
              (int)owVerify(portnum, FALSE));
   }
   if(coupler)
   {
       w1_set_coupler(portnum, cident, 0);
   }
    
   owRelease(portnum);
   return 0;
}

