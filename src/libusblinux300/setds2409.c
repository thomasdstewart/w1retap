
// Include files
#include <stdio.h>
#include <stdlib.h>
#include "ownet.h"
#include "swt1f.h"
#include <unistd.h>
#include <string.h>

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
    uchar a[3];
    int portnum=0;
    char *serial = NULL;
    char *dev = NULL;
    int c, line = 0;
    u_char ident[8];
    
    while((c = getopt(argc, argv, "ams:")) != EOF)
    {
        switch (c)
        {
            case 's':
                serial = strdup(optarg);
                break;
            case 'm':
                line = 1;
                break;
            case 'a':
                line = 2;
                break;
            default:
                break;
        }
    }

   dev = argv[optind];

   if (dev == NULL || serial == NULL)
   {
       fputs("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n", stderr);
       return 0;
   }

   if((portnum = owAcquireEx(dev)) < 0)
   {
       fputs("Failed to acquire port.\n", stderr);
       return 0;
   }
   else
   {
       w1_make_serial(serial, ident);
       switch(line)
       {
           case 1:
               SetSwitch1F(portnum, ident, DIRECT_MAIN_ON, 0, a, TRUE);
               break;
           case 2:
               SetSwitch1F(portnum, ident, AUXILARY_ON, 2, a, TRUE);
               break;
       }
       owRelease(portnum);
   }
   return 0;
}


