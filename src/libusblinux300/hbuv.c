/*
 * hbuv.c -- HobbyBoards UVI interface for w1retap
 *  (c) 2011 Jonathan Hudson <jh+w1retap@daria.co.uk>
 *
 * Depends on the Dallas PD kit, so let's assume their PD licence (or
 * MIT/X if that works for you)
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ownet.h"
#include "hbuv.h"

int HBUV_setup (int portnum, uchar *serno, hbuv_t *hb, char *params)
{
    int ret = 0;
    memset(hb, 0, sizeof(*hb));
    owSerialNum(portnum,serno,FALSE);
    if (owAccess(portnum))
    {
        if (params && *params)
        {
            char c;
            char *p1, *p2;

            for(p1 = params; (p2 = strtok(p1,", "));p1=NULL)
            {
                if (sscanf(p2,"uo=%hhd", &c))
                {
                    HBUV_set_uvi_offset(portnum, serno, c);
                }
                else if (sscanf(p2,"to=%hhd", &c))
                {
                    HBUV_set_temp_offset(portnum, serno, c);
                }
                else if (sscanf(p2,"case=%hhd", &c))
                {
                    if (c != 0) c = 0xff;
                    HBUV_set_case (portnum, serno, (uchar)c);
                }
            }
        }
        HBUV_read_version(portnum, serno, hb->version);
        HBUV_get_case(portnum, serno, &hb->has_case);
        HBUV_get_temp_offset(portnum, serno, &hb->temp_offset);
        HBUV_get_uvi_offset(portnum, serno, &hb->uvi_offset);
        ret = 1;
    }
    return ret;
}


static void HBUV_set_byteval(int portnum, uchar *snum, uchar act, uchar val)
{
    uchar block[2];
    owSerialNum(portnum,snum,FALSE);
    if (owAccess(portnum))
    {
        block[0] = act;
        block[1] = val;
        owBlock(portnum, FALSE, block, sizeof(block));
    }
}

void HBUV_get_byteval (int portnum, uchar *snum, uchar act, uchar *val)
{
    uchar block[2];
    owSerialNum(portnum,snum,FALSE);
    if (owAccess(portnum))
    {
        block[0] = act;
        block[1] = 0xff;
        if(owBlock(portnum, FALSE, block, sizeof(block)))
        {
            *val = block[1];
        }
    }
}

void HBUV_get_bytes (int portnum, uchar *snum, uchar act,
                        uchar *val, int nval)
{
    uchar block[32];
    int i;

    owSerialNum(portnum,snum,FALSE);
    if (owAccess(portnum))
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

void HBUV_set_case (int portnum, uchar *snum, uchar val)
{
    HBUV_set_byteval(portnum, snum, HBUV_SETINCASE, val);
}

void HBUV_get_case (int portnum, uchar *snum, uchar *val)
{
    HBUV_get_byteval(portnum, snum, HBUV_READINCASE, val);
}

void HBUV_set_uvi_offset (int portnum, uchar *snum, char val)
{
    HBUV_set_byteval(portnum, snum, HBUV_SETUVIOFFSET, (uchar)val);
}

void HBUV_get_uvi_offset (int portnum, uchar *snum, char *val)
{
    HBUV_get_byteval(portnum, snum, HBUV_READUVIOFFSET, (uchar*)val);
}

void HBUV_set_temp_offset (int portnum, uchar *snum, char val)
{
    HBUV_set_byteval(portnum, snum, HBUV_SETTEMPOFFSET, (uchar)val);
}

void HBUV_get_temp_offset (int portnum, uchar *snum, char *val)
{
    HBUV_get_byteval(portnum, snum, HBUV_READTEMPOFFSET, (uchar *)val);
}

void HBUV_read_version(int portnum, uchar *snum, char *vers)
{
    HBUV_get_bytes(portnum, snum, HBUV_READVERSION, (uchar*)vers, 2);
}

int HBUV_read_data(int portnum, uchar *snum, hbuv_t *hb)
{
    uchar uvi=0;
    char tchar[2];
    int ret = 0;

    HBUV_get_byteval(portnum, snum, HBUV_READUVI, &uvi);
    if (uvi == 0xff)
    {
        HBUV_get_byteval(portnum, snum, HBUV_READUVI, &uvi);
    }
    if(uvi != 0xff)
    {
        ret = 1;
        hb->raw_uvi = (uchar)uvi;
        HBUV_get_bytes(portnum, snum, HBUV_READTEMPERATURE, (uchar*)&tchar, 2);
        hb->raw_temp = tchar[0] | (tchar[1] << 8);
    }
    return ret;
}
