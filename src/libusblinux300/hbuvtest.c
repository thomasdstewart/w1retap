#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ownet.h"
#include "findtype.h"
#include "swt1f.h"
#include <unistd.h>
#include "hbuv.h"

static void w1_make_serial(char * asc, unsigned char *bin)
{
    int i,j;
    for(i = j = 0; i < 8; i++)
    {
        bin[i] = ToHex(asc[j]) << 4; j++;
        bin[i] += ToHex(asc[j]); j++;
    }
}

static void w1_set_coupler(int portnum, uchar *ident, int line)
{
  uchar a[4];
  SetSwitch1F(portnum, ident, line, 2, a, TRUE);
}

static int w1_select_device(int portnum, uchar *serno, uchar *coupler, int line)
{
    u_char thisdev[8];
    int found = 0;
    
    if(coupler)
    {
        w1_set_coupler(portnum, coupler, 0);
        w1_set_coupler(portnum, coupler, line);
    }

    owFamilySearchSetup(portnum,serno[0]);
    while (owNext(portnum,TRUE, FALSE)){
      owSerialNum(portnum, thisdev, TRUE);
      if(memcmp(thisdev, serno, sizeof(thisdev)) == 0){
	found = 1;
	break;
      }
    }
    return found;
}


static void usage(void)
{
    fputs(
        "usage: hbuvtest -s serial [-c coupler -a|-m ] [-p params] device\n\n"
        "	device => /dev/ttyS0 or DS2490-1\n"
        "	serial => Device serial number\n"
        "	coupler => Coupler serial number (if needed)\n"
        "	-m, -a =>  indicates coupler branch (_main or _aux)\n"
        "	params => settings for non-volatile RAM\n\n"
        "	The coupler setting are ony needed when the device is accessed\n"
        "	through a DS2409 microlan coupler\n\n"
        "	The params option provides values for non-volatile RAM. These values\n"
        "	only need to be supplied once, or if a change is required.\n"
        "	The option is supplied as a space or comma delimited string\n"
        "	\"uo=UVOFFSET to=TEMPOFFSET case=INCASE\" where:\n"
        "		uo=UVOFFSET supplies UVOFFSET as the UVI offset in tenths\n"
        "		to=TEMPOFFSET supplies TEMPOFFSET as the Temp offset in 0.5⁰C\n"
        "		case=INCASE supplies INCASE as the In Case flag\n"
        "		 (any non-zero INCASE value sets 0xFF)\n"
        "	Please see the vendor documentation for further details\n\n"
        " e.g.	./hbuvtest -s EEnnnnnnnnnnnnnn -c 1Fnnnnnnnnnnnnnn -m \\\n"
        "	  -p \"uo=7 to=-4 case=1\" DS2490-1\n"
        "	# you can omit not neeeded settings ...\n"
        "       ./hbuvtest -s EEnnnnnnnnnnnnnn -p \"to=42 case=0\" DS2490-1\n\n"
        "	The application reports current settings and read values\n"
        , stderr);
    exit(0);
}

int main(int argc, char **argv)
{
    float temp = -999, uvi = -999;
    int portnum = 0;
    char *serial = NULL, *dev=NULL;
    uchar ident[8];
    char *coupler=NULL;
    uchar *cident=NULL;
    char *params=NULL;
    int c;
    int bra=0;
    int ret = -1;
    
    hbuv_t hbuv={0};
    hbuv_t* hb = &hbuv;

    while((c = getopt(argc, argv, "amc:s:p:h?")) != EOF)
    {
        switch (c)
        {
            case 'a':
                bra = 2;
                break;
            case 'm':
                bra = 4;
                break;
            case 's':
                serial = strdup(optarg);
                break;
            case 'c':
                coupler = strdup(optarg);
            case 'p':
                params = strdup(optarg);
                break;
            case 'h':
            case '?':
            default:
                usage();
                break;
       }
    }
    
    dev = argv[optind];
    if (dev == NULL || serial == NULL)
    {
        usage();
    }

    if (0 == strncmp(dev,"usb", 3))
    {
        dev = strdup("DS2490-1");
    }
    
    if((portnum = owAcquireEx(dev)) < 0)
    {
        OWERROR_DUMP(stdout);
        exit(1);
    }
    
    w1_make_serial(serial, ident);
    if (coupler)
    {
        cident = malloc(32);
        w1_make_serial(coupler, cident);
    }
    if (!w1_select_device(portnum, ident, cident, bra))
    {
        fputs("No dev\n", stderr);
        exit(1);
    }
    
    ret = HBUV_setup(portnum, ident, hb, params);
    printf("HBUV_setup returns %d\n"
           "Version = %d.%d\n"
           "incase = 0x%02x\n"
           "temp offset = %d\n"
           "uvi offset = %d\n",
           ret, hb->version[0], hb->version[1], hb->has_case,
           hb->temp_offset, hb->uvi_offset);
    
    ret = HBUV_read_data(portnum, ident, hb);
    temp = hb->raw_temp * 0.5;
    uvi = hb->raw_uvi / 10.0;
    
    printf("HBUV_read_data returns %d\n"
           "raw temp = 0x%04x\n"
           "temp = %.1f ⁰C\n"
           "raw_uvi = 0x%x\n"
           "UVI = %.1f\n",
           ret, hb->raw_temp, temp, hb->raw_uvi, uvi);
    if(coupler)
    {
        w1_set_coupler(portnum, cident, 0);
    }
    owRelease(portnum);
    return 0;
}

