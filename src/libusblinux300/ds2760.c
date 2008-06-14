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
//--------------------------------------------------------------------------
//
//  ds2760.c - Reads the stuff on the 1-Wire of the DS2760. 
//  modified from atod26.c
//

// Include Files
#include <stdio.h>
#include "ownet.h"
#include "ds2760.h"

int ReadDS2760(int portnum, uchar *snum, ds2760_t *v)
{
    int ret = 0;
    uchar send_block[16];
    int i;
    
    owSerialNum(portnum,snum,FALSE);
    if (owAccess(portnum))
    {
        send_block[0] = 0x69; // Data command
        send_block[1] = 0x0c; // offset to first data (volt, current);
        for(i=2; i< 8;i++)
        {
            *(send_block+i) = 0xff;
        }

        if(owBlock(portnum, FALSE, send_block, 8))
        {
            short ival;
#ifdef DSDEBUG
            int k;
            fputs("ds2760 - got block1: ", stderr);
            for(k = 0; k < 8; k++)
            {
                fprintf(stderr, "%02x ", send_block[k]);
            }
            fputc('\n', stderr);
#endif
            /* 10 bit voltage, sign extended as necessary */
            ival = (send_block[2] << 3) | (send_block[3] >> 5);
            if (ival & 0x400) ival |= 0xfc00;
            v->volts = ival * 0.00488;

            /* 12 bit current, sign extended as necessary */
            ival = (send_block[4] << 5) | (send_block[5] >> 3);
#ifdef DSDEBUG
            fprintf(stderr,"Raw Current1: %04hx ", ival);
#endif        
            if (ival & 0x1000) ival |= 0xf000;
#ifdef DSDEBUG
            fprintf(stderr,"Raw Current2: %04hx\n", ival);
#endif        
            v->curr = ival * 0.000625;

            /* 16 bit acculated charge */
            ival = (send_block[6] << 8) | send_block[7];
#ifdef DSDEBUG
            fprintf(stderr,"Raw Accu: %04hx\n", ival);
#endif
            v->accum = ival * 0.00025;

            owSerialNum(portnum,snum,FALSE);
            if(owAccess(portnum))
            {
#ifdef DSDEBUG
                fprintf(stderr, "owAccess %d\n", iret);
#endif
                send_block[0] = 0x69; // Data command
                send_block[1] = 0x18; // offset to temp data
                *(send_block+2) = *(send_block+3) = 0xff;

                if (owBlock(portnum, FALSE, send_block, 4))
                {
#ifdef DSDEBUG
                    int k;
                    fputs("ds2760 - got block2: ", stderr);
                    for(k = 0; k < 4; k++)
                    {
                        fprintf(stderr, "%02x ", send_block[k]);
                    }
                    fputc('\n', stderr);
#endif
                    ival = (send_block[2] << 3) | (send_block[3] >> 5);
#ifdef DSDEBUG
                    fprintf(stderr,"Raw Temp1: %04hx ", ival);
#endif
                    if (ival & 0x400) ival |= 0xfc00;
#ifdef DSDEBUG
                    fprintf(stderr,"Raw Temp2: %04hx\n", ival);
#endif
                    v->temp = ival * 0.125;
                    ret = TRUE;
                }
            }
        }
    }
    return ret;
}

