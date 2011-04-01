//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//---------------------------------------------------------------------------
//
//  gethumd.c - This utility gets the Volts for pins Vad and Vdd from the DS2438.
//
//  Version:   2.00
//  History:

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "ownet.h"
#include "atod26.h"
#include "findtype.h"
#include "math.h"

#define MAXDEVICES 5
#define ONEKBITADD 0x89

#define SLOPE (34.249672152)
#define OFFSET (762.374681772)

/* 35.949367089 751.075949363 */


float pressure_at_msl(float pres, float temp, int altitude)
{
#define __CONST_G (9.80665)
#define __CONST_R (287.04)
    float kt = temp +  273.15;
    float x = (__CONST_G * altitude / (__CONST_R * kt));
    pres *= exp(x);
    return pres;
}

float GetPressure(float Vad, float temp, int alt)
{
    float pres = SLOPE * Vad + OFFSET;
    return (alt) ? pressure_at_msl(pres, temp, alt) : pres;
}

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
   float Vdd,Vad;
   int c,alt = 0;
   char *dev;
   char *serial = NULL;
   u_char ident[8];
   int verbose = 0;
   short dac=0;
    
   while((c = getopt(argc, argv, "da:s:v")) != EOF)
   {
       switch (c)
       {
           case 'v':
               verbose = 1;
               break;
           case 's':
               serial = strdup(optarg);
               break;
           case 'a':
               alt = atoi(optarg);
               break;
           case 'd':
               dac = 1;
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
       if(verbose)
       {
           int i;
           fprintf(stderr,"Serial: %s\n", serial);
           fputs("Ident: ", stderr);
           for(i=0; i < 8; i++)
           {
               fprintf(stderr, "%02X", ident[i]);
           }
           fputc('\n', stderr);
           fprintf(stderr, "Altitude: %d\n", alt);
       }

       Vdd = ReadAtoD(portnum,TRUE, ident);
       if(Vdd > 5.8)
       {
           Vdd = (float)5.8;
       }
       else if(Vdd < 4.0)
       {
           Vdd = (float) 4.0;
       }
       Vad = ReadAtoD(portnum,FALSE,ident);
       printf("VSup %.2f Vout %.2f", Vdd, Vad);
       if (dac)
       {
           fputc('\n', stdout);	       
       }
       else
       {
           float temp = Get_Temperature(portnum,ident);
           float pres = GetPressure(Vad, temp, alt);
           printf(" Temp %.2f Pressure %.1f\n", temp, pres);
       }
   }
   return 0;
}
