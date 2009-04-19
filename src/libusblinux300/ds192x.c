#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "ownet.h"
#include "humutil.h"
#include "ds192x.h"

#define CONVERT_TEMPERATURE  0x44
#define STATUS_OFFSET 0x214
#define TEMP_OFFSET 0x211

static int WriteScratch(int,uchar *,int,int);
static int CopyScratch(int,int,int);
static int WriteMemory(int,uchar *, int, int);

#ifdef TESTMAIN
#include <stdio.h>
#include "../../config.h"
#define TRACE(params) printf params
#define DUMPBLOCK(a,b,c) dumpblock(a,b,c)
static void dumpblock(char *label, uchar *block, int size)
{
    int j;
    fputs(label, stdout);
    for(j=0; j<size; j++)
    {
        printf(" %02x", block[j]);
    }
    fputc('\n', stdout);
}
#else
#define TRACE(params)
#define DUMPBLOCK(a,b,c)
#endif


//----------------------------------------------------------------------------}
// Write a memory location. Data must all be on the same page
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number is provided to
//              indicate the symbolic port number.
//
int WriteMemory(int portnum, uchar *Buf, int ln, int adr)
{
   // write to scratch and then copy
   if (WriteScratch(portnum,Buf,ln,adr)) 
      return CopyScratch(portnum,ln,adr);

   return FALSE;
}

//----------------------------------------------------------------------------}
// Write the scratch pad
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number is provided to
//              indicate the symbolic port number.
//
int WriteScratch(int portnum, uchar *Buf, int ln, int adr)
{
   int i;
   uchar pbuf[80];

   // check for alarm indicator 
   if (owAccess(portnum)) 
   {
      // construct a packet to send  
      pbuf[0] = 0x0F; // write scratch command 
      pbuf[1] = (adr & 0xFF); // address 1 
      pbuf[2] = ((adr >> 8) & 0xFF); // address 2 

      // the write bytes 
      for (i = 0; i < ln; i++)
        pbuf[3 + i] = (uchar)(Buf[i]); // data 

      // perform the block 
      if (!owBlock(portnum,FALSE,pbuf,ln+3))
         return FALSE;

      // Now read back the scratch 
      if (owAccess(portnum)) 
      {
         // construct a packet to send 
         pbuf[0] = 0xAA; // read scratch command 
         pbuf[1] = 0xFF; // address 1 
         pbuf[2] = 0xFF; // address 2 
         pbuf[3] = 0xFF; // offset 

         // the write bytes 
         for (i = 0; i < ln; i++)
            pbuf[4 + i] = 0xFF; // data 

         // perform the block  
         if (!owBlock(portnum,FALSE,pbuf,ln+4))
            return FALSE;

         // read address 1 
         if (pbuf[1] != (adr & 0xFF)) 
            return FALSE;
         // read address 2 
         if (pbuf[2] != ((adr >> 8) & 0xFF)) 
            return FALSE;
         // read the offset 
         if (pbuf[3] != ((adr + ln - 1) & 0x1F)) 
            return FALSE;
         // read and compare the contents 
         for (i = 0; i < ln; i++)
         {
            if (pbuf[4 + i] != Buf[i]) 
              return FALSE;
         }
         // success
         return TRUE;
      }
   }

   OWERROR(OWERROR_ACCESS_FAILED);
   return FALSE;
}

//----------------------------------------------------------------------------}
// Copy the scratch pad
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number is provided to
//              indicate the symbolic port number.
//
int CopyScratch(int portnum, int ln, int adr)
{
   int i;
   uchar pbuf[50];

   // check for alarm indicator 
   if (owAccess(portnum)) 
   {
      // construct a packet to send 
      pbuf[0] = 0x55;                  // copy scratch command 
      pbuf[1] = (adr & 0xFF);          // address 1 
      pbuf[2] = ((adr >> 8) & 0xFF);   // address 2 
      pbuf[3] = (adr + ln - 1) & 0x1F; // offset 
      for (i = 0; i <= 9; i++)
         pbuf[4 + i] = 0xFF;           // result of copy 

      // perform the block 
      if (owBlock(portnum,FALSE,pbuf,14))
      {
         if ((pbuf[13] == 0x55) ||
             (pbuf[13] == 0xAA)) 
           return TRUE;
      }
   }

   OWERROR(OWERROR_ACCESS_FAILED);
   return FALSE;
}

#define READ_MEMORY 0xf0

static int readblock(int portnum, uchar *snum, short offset, uchar *buf)
{
    uchar block[16];
    int i = 0;
    int len=0;
    *buf=0xff;
    
    if(owAccess(portnum))
    {
        TRACE (("ds1921: send read\n"));
        block[len++]  = READ_MEMORY;
        block[len++] = offset & 0xff;
        block[len++] = (offset & 0xff00) >> 8;
        block[len++] = 0xff;
        i = owBlock(portnum, FALSE, block, len);
        *buf = block[3];
        TRACE (("readblock %d \n", i));
        DUMPBLOCK("data: ", block, len);
    }
    return i;
}

#define BIT_READY (1 << 7)
#define BIT_MIP (1 << 5)
#define STS_SIP (1 << 4)

int ReadDS1921(int portnum, uchar *snum, ds1921_t *v)
{
    int ret = 0;
    uchar val[2];
    unsigned short id;
    
    v->temp = -42;
    
    owSerialNum(portnum,snum,FALSE);
    if ((ret = readblock(portnum, snum, 0x214, val)))
    {
        if (val[0] & BIT_MIP)
        {
            TRACE (("Mission in progress\n"));
            if (v->kill)
            {
                val[0] = 0;
                ret = WriteMemory(portnum, val, 1, 0x214);
                TRACE (("** Mission killed **\n"));
            }
        }
        else
        {
            TRACE( ("Attempting DS1921\n") );
            if (owAccess(portnum)) 
                ret = owTouchByte(portnum,0x44);
        }

        if(ret)
        {
            for(ret= 0; (ret = readblock(portnum, snum, 0x214, val)) &&
                !(val[0] & BIT_READY);)
            {
                msDelay(1);
            }

            if((ret = readblock(portnum, snum, 0x211, val)))
            {
                TRACE( ("Temp, raw %02x\n", val[0]) );
                id = (*(snum+5) >> 4) | *(snum+6) << 4;
                if (id == 0x4f2) // 1921H
                {
                    TRACE(("1921H "));
                    v->temp = val[0] / 8.0 + 14.500;
                }
                else if (id == 0x3B2) // 1921K
                {
                    TRACE(("1921Z "));       
                    v->temp = val[0] / 8.0 - 5.500;
                }
                else
                {
                    TRACE(("1921G? "));
                    v->temp  = val[0] /2.0 - 40.0;
                }
                TRACE( ("temp = %f C\n", v->temp) );
            }
        }
    }
    return ret;
}

int ReadDS1923(int portnum, uchar *snum, ds1923_t *v)
{
    int ret = 0;
    uchar state[96];
    double val,valsq,error;
    configLog config ={0};

    v->temp = -42;
    v->rh = 200;

    if(v->kill)
    {
        stopMission(portnum, snum);
    }
    
        // set up config for DS1923
    config.configByte = 0x20;
    TRACE( ("Attempting DS1923\n") );
    if(readDevice(portnum,snum,state,&config))
    {
        TRACE( ("Read OK\n") );
        
        if(doTemperatureConvert(portnum,snum,state))
        {
            TRACE ( ("Convert OK\n") );
            config.useTemperatureCalibration = 1;
            val = decodeTemperature(&state[12],2,FALSE,config);
            valsq = val*val;
            error = config.tempCoeffA*valsq +
                config.tempCoeffB*val + config.tempCoeffC;
            v->temp = val - error;
            
            config.useHumidityCalibration = 1;
            val = decodeHumidity(&state[14],2,FALSE,config);
            valsq = val*val;
            error = config.humCoeffA*valsq +
                config.humCoeffB*val + config.humCoeffC;
            v->rh = val - error;
            
            if(v->rh < 0.0)
                v->rh = 0.0;

            TRACE ( ("DS1923 result %f C %f %%\n", v->temp, v->rh) );
            ret = 1;
        }
    }
    return ret;
}

#ifdef TESTMAIN
static void w1_make_serial(char * asc, unsigned char *bin)
{
    int i,j;
    for(i = j = 0; i < 8; i++)
    {
        bin[i] = ToHex(asc[j]) << 4; j++;
        bin[i] |= ToHex(asc[j]); j++;
    }
}

int main(int argc, char **argv)
{
    int portnum = 0;
    char *serial21 = NULL;
    char *serial23 = NULL;
    u_char ident[8];
    int c;
    char *dev;
    int n1 = -42, n3 = -42;
    int dokill = 0;
    
    while((c = getopt(argc, argv, "1:3:k")) != EOF)
    {
        switch (c)
        {
            case '1':
                serial21 = strdup(optarg);
                break;
            case '3':
                serial23 = strdup(optarg);
                break;
                
            case 'k':
                dokill = 1;
                break;
                
            default:
                break;
        }
    }
    dev = argv[optind];

    TRACE(("DS192x tester %s\n",argv[0]));
    TRACE((__FILE__ " "  __DATE__ " "  __TIME__ "\n"));
    TRACE(("Based on w1retap " VERSION  "\n" ));
    
    if (dev == NULL || (serial21 == NULL && serial23 == NULL))
    {
        fprintf(stderr, "Usage: %s -1 DS1921address -3 DS1923address device\n""          device  -- DS2490-1 = first USB device\n"
              "                     /dev/ttyS0 = first serial port\n"
              "                     /dev/ttyUSB0 = USB/serial adaptor\n"
              "\tyou MUST give one of -1 addr or -3 addr\n"
              " e.g. %s -1 21F8190100204FC7 -3 410FA8010000005C /dev/ttyS0\n",
                argv[0], argv[0]);
        return 0;
    }


    if((portnum = owAcquireEx(dev)) < 0)
    {
        fputs("Failed to acquire port.\n", stdout);
    }
    else
    {
      
        if(serial21)
        {
            ds1921_t v;
            v.kill = dokill;
            w1_make_serial(serial21, ident);
            n1 = ReadDS1921(portnum, ident, &v);
        }
        if(serial23)
        {
            ds1923_t v;
            w1_make_serial(serial23, ident);
            n3 = ReadDS1923(portnum, ident, &v);
        }
    }
    printf("finally %d %d\n", n1, n3);
    return 0;
}
#endif
