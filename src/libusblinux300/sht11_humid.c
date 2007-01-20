#include <stdio.h>
#include "ownet.h"
#include "sht11.h"

/*
 * From: 
 * http://www.sensirion.com/pdf/product_information/CRC_Calculation_Humidity_Sensor_E.pdf
 */

static unsigned char CRC[] =
{
    0,49,98,83,196,245,166,151,185,136,219,234,125,76,31,46,67,114,33,16,
    135,182,229,212,250,203,152,169,62,15,92,109,134,183,228,213,66,115,32,17,
    63,14,93,108,251,202,153,168,197,244,167,150,1,48,99,82,124,77,30,47,
    184,137,218,235,61,12,95,110,249,200,155,170,132,181,230,215,64,113,34,19,
    126,79,28,45,186,139,216,233,199,246,165,148,3,50,97,80,187,138,217,232,
    127,78,29,44,2,51,96,81,198,247,164,149,248,201,154,171,60,13,94,111,
    65,112,35,18,133,180,231,214,122,75,24,41,190,143,220,237,195,242,161,144,
    7,54,101,84,57,8,91,106,253,204,159,174,128,177,226,211,68,117,38,23,
    252,205,158,175,56,9,90,107,69,116,39,22,129,176,227,210,191,142,221,236,
    123,74,25,40,6,55,100,85,194,243,160,145,71,118,37,20,131,178,225,208,
    254,207,156,173,58,11,88,105,4,53,102,87,192,241,162,147,189,140,223,238,
    121,72,27,42,193,240,163,146,5,52,103,86,120,73,26,43,188,141,222,239,
    130,179,224,209,70,119,36,21,59,10,89,104,255,206,157,172
};

#define NRETRY 3

static inline unsigned char revbits(unsigned char b)
{
  /*
   * Shameless taken from
   * http://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
   * Devised by Sean Anderson, July 13, 2001.
   * Typo spotted and correction supplied by Mike Keith, January 3, 2002.
   */
    return ((b * 0x0802LU & 0x22110LU) |
            (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static int checkcrc(unsigned char *blk,unsigned char crcs[2])
{
    int i=0;

    // CRC is made up from the SHT-11 command and data, thusly
    crcs[0] = revbits(CRC[((CRC[(CRC[3] ^ blk[2])]) ^ blk[3])]);
    crcs[1] = revbits(CRC[((CRC[(CRC[5] ^ blk[7])]) ^ blk[8])]);
    
    if(crcs[0] != blk[4])
    {
        i |= 1;
    }

    if (crcs[1] != blk[9])
    {
        i |= 2;
    }
    return i;
}

int ReadSHT11(int portnum, u_char *ident,float *temp,  float *rh)
{
    int i,j;
    int rv = 0;
    int verbose = !!getenv("SHT11_VERBOSE");
    
    for(j = 0; (rv ==0) && (j < NRETRY); j++)
    {
        owTouchReset(portnum);
        owTouchByte(portnum, 0x55);
        for(i=0;i<8;i++)          
        {
            owTouchByte(portnum, ident[i]);
        }

        if (!owTouchByte(portnum,0x44))
        {
            if (verbose) fputs("Failed WritePower 0x44\n", stderr);
            return rv;
        }
        msDelay(300);
           
        owTouchReset(portnum);
        owTouchByte(portnum, 0xCC);
        {
            unsigned char c;
            unsigned char blk[10];
            c = owTouchByte(portnum, 0x45);
            for(i = 0; i < 10; i++)
            {
                c = owTouchByte(portnum,0xff);
                blk[i] = c;
            }

            if(blk[0] =='T' && blk[5] == 'H' && blk[1] == '=' && blk[6] == '=')
            {
                int crcres;
                unsigned char crcs[2];
                
                if (0 == (crcres = checkcrc(blk,crcs)))
                {
                    unsigned short so;
                    so = (blk[2] << 8) | blk[3];
		    if (so > 10000)
		    {
			fprintf(stderr,"Invalid Temp data %d\n", so);
			break;
		    }
                    *temp = -40.0 + 0.01 * so;
                    so = (blk[7] << 8) | blk[8];
		    if (so < 99 || so > 3338)
		    {
			fprintf(stderr,"Invalid RH data %d\n", so);
			break;
		    }
   		    *rh = -4.0 + 0.0405*so - 0.0000028*so*so;
                    rv = 1;
                }
                else if (verbose)
                {
                    if (crcres & 1)
                    {
                        fprintf(stderr,"Temp CRC (%02x) : %02x %02x %02x ",
                                crcs[0],blk[2],blk[3],blk[4]);
                    }
                    if (crcres & 2)
                    {
                        fprintf(stderr,"RH CRC (%02x) : %02x %02x %02x",
                                crcs[1],blk[7],blk[8],blk[9]);
                    }
                    fputc('\n', stderr);
                }
            }
            else if (verbose)
            {
                fputs("No data signature\n", stderr);
            }
        }
	if(rv == 0) msDelay(100);
    }
    owTouchReset(portnum);
    return rv;
}

