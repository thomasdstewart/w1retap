#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "ownet.h"
#include "findtype.h"
#include "math.h"
#include <time.h>

static void output_byte(int portnum, uchar b)
{
    uchar n,n1;
    
    owWriteByte(portnum, (uchar)b);
    owWriteByte(portnum, (uchar)(~b));
    n = owReadByte(portnum);
    n1 = owReadByte(portnum);
    if (n != 0xaa) fprintf(stdout, "failed to write %02x %02x %02x\n", b, n, n1);
}

static void output_char(int portnum, uchar b, short flag)
{
    uchar n;
    n = b & 0xf0;
    if (flag) n |= 0x10;
    output_byte (portnum, n);
    n = ((b << 4) & 0xf0);
    if (flag) n |= 0x10;
    output_byte(portnum, n);
}

int init_lcd(int portnum, uchar *snum)
{
    uchar b;
    int rv = -1;

    owSerialNum(portnum,snum,FALSE);

    if(owAccess(portnum))
    {
        owWriteByte(portnum, 0xcc);
        owWriteByte(portnum, 0x8d);
        owWriteByte(portnum, 0x00);
        owWriteByte(portnum, 0x04);
        owTouchReset(portnum);

        owAccess(portnum); /* a5 */
        owWriteByte(portnum, 0xf0);            

        owWriteByte(portnum, 0x8d);
        owWriteByte(portnum, 0x00);
        b = owReadByte(portnum);
        if (b != 0x84)
        {
            fputs("Init fails\n", stdout);
            owTouchReset(portnum);
            return rv;
        }

        owTouchReset(portnum);        
        owAccess(portnum); /* a5 */
        owWriteByte(portnum, 0x5a);                
        rv = 0;
    }
    return rv;
}

void prep2display(portnum)
{
    
    fputs("Start init disp\n", stdout);
    output_byte(portnum, 0x30);
    msDelay(5);
    output_byte(portnum, 0x30);
    output_byte(portnum, 0x30);
    output_byte(portnum, 0x20);
        
    output_char(portnum, 0x28, 0);
    output_char(portnum, 0x0c, 0);        
    output_char(portnum, 0x01, 0);
    output_char(portnum, 0x06, 0);
    output_char(portnum, 0x80, 0);
}

static void set_line(int portnum, int line)
{
//    if(owAccess(portnum))
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
        output_char(portnum, c, 0);        
    }
}

static void prep_display(portnum)
{
    owTouchReset(portnum);
    if(owAccess(portnum))
    {
        owWriteByte(portnum,0x5a);
    }
    msDelay(5);
}


static void show_text(int portnum, uchar *text)
{
//    if(owAccess(portnum))
    {
        uchar c;
        while((c = *text++))
        {
            if (c < ' ') c = ' ';
            output_char(portnum, c, 1);
            msDelay(1);
        }
    }
}

int send_text_lcd(int portnum, char *text, int line, int cls)
{
    
    fputs("Init device\n", stdout);
    prep_display(portnum);
    set_line(portnum, line);
    fprintf(stdout, "Text is %s\n", text);
    show_text(portnum, text);
    return 0;
}

