#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "ownet.h"
#include "findtype.h"
#include "math.h"
#include <time.h>

static int init_device(int portnum, uchar *snum)
{
    uchar block[32],b;
    int cnt=0;
    int rv = -1;
    
    owSerialNum(portnum,snum,FALSE);
    block[cnt++] = 0xcc;
    block[cnt++] = 0x8d;
    block[cnt++] = 0x00;
    block[cnt++] = 0x04;

    if (owBlock(portnum, FALSE, block, cnt))
    {
        if(owAccess(portnum))
        {
            cnt = 0;
            block[cnt++] = 0xf0;
            block[cnt++] = 0x8d;
            block[cnt++] = 0x00;
            if (owBlock(portnum, FALSE, block, cnt))
            {
                b = owReadByte(portnum);
                if (b == 0x84)
                {
                    if (owAccess(portnum))
                    {
                        owWriteByte(portnum, 0x5a);
                        rv = 0;
                    }
                }
            }
        }
    }
    return rv;
}

static uchar *add_byte(uchar *buf, uchar b)
{
    *buf++ = b;
    *buf++ = ~b;
    *buf++ = 0xff;
    *buf++ = 0xff;
    return buf;
}

static uchar *add_char(uchar *buf, uchar b, short flag)
{
    uchar n;
    n = b & 0xf0;
    if (flag) n += 0x08;
    buf = add_byte(buf, n);
    n = (b << 4) & 0xf0;
    if (flag) n += 0x08;
    buf = add_byte(buf, n);
    return buf;
}

static void init_display(int portnum, int cls)
{
    uchar block[128],*p;
    p = block;
    
    if (owAccess(portnum))
    {
        p = add_byte(p, 0x30);
        p = add_byte(p, 0x30);
        p = add_byte(p, 0x30);
        p = add_byte(p, 0x20);
        p = add_char(p, 0x28, 0);
        if(cls) p = add_char(p, 0x01, 0);
        p = add_char(p, 0x0c,0);
        p = add_char(p, 0x06,0);
        if (cls) add_char(p,0x80,0);
        int n =  (int)(p-block);
        owBlock(portnum, FALSE, block, n);
        fprintf(stderr, "Init display %d\n", n);
    }
}

static void set_line(int portnum, int line)
{
    uchar block[32],*p;
    p = block;
    
    if(owAccess(portnum))
    {
        uchar c;
        switch (line)
        {
            case 2:
                c = 0xc0;
                break;
            case 3:
                c = 0x94;
            case 4:
                c = 0xd4;
                break;
            default:
                c = 0x80;
                break;
        }
        p = add_char(p, c, 0);        
        owBlock(portnum, FALSE, block, (int)(p-block));        
    }
}

static void show_text(int portnum, char *text)
{
    if(owAccess(portnum))
    {
        char c;
        uchar buf[512],*p;
        p = buf;
        while((c = *text++))
        {
            if (c < ' ') c = ' ';
            p = add_char(p, c, 1);
        }
        int n =  (int)(p-buf);
        owBlock(portnum, FALSE, buf, n);
        fprintf(stderr, "show text %d\n", n);

    }
}

int send_text_lcd(int portnum, uchar *snum, char *text, int line, int cls)
{
    int rv = -1;
    
    if (0 == (rv = init_device(portnum, snum)))
    {
        fputs("Init device\n", stderr);
        init_display(portnum, cls);
        set_line(portnum, line);
        show_text(portnum, text);
    }
    return rv;
}

