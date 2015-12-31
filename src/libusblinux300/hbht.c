/*
 * hbht.c -- HobbyBoards UVI interface for w1retap
 *  (c) 2015 Jonathan Hudson <jh+w1retap@daria.co.uk>
 *
 * Depends on the Dallas PD kit, so let's assume their PD licence (or
 * MIT/X if that works for you)
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ownet.h"
#include "hbht.h"

static char debug;

static int HBHT_access(int portnum, uchar *serno)
{
    owTouchReset(portnum);
    msDelay(100);
    owSerialNum(portnum,serno,FALSE);
    return owAccess(portnum);
}

int HBHT_setup (int portnum, uchar *serno, hbht_t *hb, char *params)
{
    int ret = 0;
    debug = 1;
    memset(hb, 0, sizeof(*hb));
    if (HBHT_access(portnum, serno))
    {
        if (params && *params)
        {
            uchar c;
            short hoff;
            char *p1, *p2;

            for(p1 = params; (p2 = strtok(p1,", "));p1=NULL)
            {
                if (sscanf(p2,"pf=%hhu", &c))
                {
                    HBHT_set_poll_freq(portnum, serno, c);
                }
                else if (sscanf(p2,"ho=%hd", &hoff))
                {
                    HBHT_set_hum_offset(portnum, serno, hoff);
                }
            }
        }
        HBHT_read_version(portnum, serno, hb->version);
        hb->humid_offset = HBHT_get_hum_offset(portnum, serno);
        hb->poll_freq = HBHT_get_poll_freq(portnum, serno);
        ret = 1;
    }
    return ret;
}

static void HBHT_set_byteval(int portnum, uchar *snum, uchar act, uchar val)
{
    uchar block[2];
    if (HBHT_access(portnum, snum))
    {
        block[0] = act;
        block[1] = val;
        owBlock(portnum, FALSE, block, sizeof(block));
    }
}

void HBHT_get_byteval (int portnum, uchar *snum, uchar act, uchar *val)
{
    uchar block[2];

    if (HBHT_access(portnum, snum))
    {
        block[0] = act;
        block[1] = 0xff;
        if(owBlock(portnum, FALSE, block, sizeof(block)))
        {
            *val = block[1];
        }
    }
}

void HBHT_get_bytes (int portnum, uchar *snum, uchar act,
                        uchar *val, int nval)
{
    uchar block[32];
    int i;

    if (HBHT_access(portnum, snum))
    {
        block[0] = act;
        for (i = 0; i < nval; i++)
        {
            block[1+i] = 0xff;
        }
        if(owBlock(portnum, FALSE, block, 1+nval))
        {
            for (i = 0; i < nval; i++)
            {
                *(val+i) = block[1+i];
            }
        }
    }
}

void HBHT_set_poll_freq (int portnum, uchar *snum, uchar val)
{
    HBHT_set_byteval(portnum, snum, HBHT_SETPOLLFREQ, val);
}

uchar HBHT_get_poll_freq (int portnum, uchar *snum)
{
    uchar val;
    HBHT_get_byteval(portnum, snum, HBHT_GETPOLLFREQ, &val);
    return val;
}

short HBHT_get_hum_offset(int portnum, uchar *snum)
{
    uchar hoff_raw[2];
    short hoff;
    HBHT_get_bytes(portnum, snum, HBHT_GETHUMIDOFFSET, hoff_raw, 2);
    hoff = (hoff_raw[0] | (hoff_raw[1] << 8));
    if(debug)
        fprintf(stderr, "read raw hoff bytes %x %x = %d\n", hoff_raw[0], hoff_raw[1], hoff);
    return hoff;
}

void HBHT_set_hum_offset(int portnum, uchar *snum, short hoff)
{
    uchar block[4];

    if (HBHT_access(portnum, snum))
    {
        block[0] = HBHT_SETHUMIDOFFSET;
        block[1] = hoff & 0xff;
        block[2] = hoff >> 8;
        owBlock(portnum, FALSE, block, 1+sizeof(short));
    }
}

void HBHT_read_version(int portnum, uchar *snum, char *vers)
{
    HBHT_get_bytes(portnum, snum, HBHT_READVERSION, (uchar*)vers, 2);
}

int HBHT_read_data(int portnum, uchar *snum, hbht_res_t *hb)
{
    uchar tchar[2];
    HBHT_get_bytes(portnum, snum, HBHT_GETTEMPERATURE_C, (uchar*)&tchar, 2);
    hb->temp = (float)((short)(tchar[0] | (tchar[1] << 8)))/10.0;
    HBHT_get_bytes(portnum, snum, HBHT_GETCORRHUMID, (uchar*)&tchar, 2);
    hb->humid = (float)((short)(tchar[0] | (tchar[1] << 8)))/100.0;
    if(debug)
    {
        fprintf(stderr, "read temp bytes %02x %02x = %.1f\n", tchar[0], tchar[1], hb->temp);
        fprintf(stderr, "read humid bytes %x %x = %.1f\n", tchar[0], tchar[1], hb->humid);
    }
    return 1;
}
