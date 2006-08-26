#include <stdio.h>
#include "ownet.h"
#include "sht11.h"

int ReadSHT11(int portnum, u_char *ident,float *temp,  float *rh)
{
    int i;
    int rv = 0;
    
    owTouchReset(portnum);
    owTouchByte(portnum, 0x55);
    for(i=0;i<8;i++)          
    {
        owTouchByte(portnum, ident[i]);
    }

    if (!owTouchByte(portnum,0x44))
    {
	puts("WritePower");
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
	    unsigned short so;
	    so = (blk[2] << 8) | blk[3];
	    *temp = -40.0 + 0.01 * so;
	    so = (blk[7] << 8) | blk[8];
	    *rh = -4.0 + 0.0405*so - 0.0000028*so*so;
            rv = 1;
	}
        else
        {
            rv = 0;
        }
    }
    owTouchReset(portnum);
    return rv;
}

