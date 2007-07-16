#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "ownet.h"
#include "findtype.h"
#include "swt1f.h"

extern int send_text_lcd(int, char *, int, int);
extern int init_lcd(int, uchar *);

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

int main(int argc, char **argv)
{
   int portnum = 0;
   char *serial = NULL, *txt1=NULL, *txt2=NULL, *dev=NULL;
   u_char ident[8];
   char *coupler=NULL;
   u_char *cident=NULL;
   int c;
   int bra=0;

   while((c = getopt(argc, argv, "amc:s:1:2:")) != EOF)
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
               break;
           case '1':
               txt1 = strdup(optarg);
               break;
           case '2':
               txt2 = strdup(optarg);
               break;
         default:
               break;
       }
   }
   dev = argv[optind];
   if (dev == NULL || serial == NULL)
   {
       fputs("usage: lcdtest -s serial -1 \"Line 1\" -2 \"Line 2\" device\n"
             "       device => /dev/ttyS0 or DS2490-1\n", stderr);
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
       

       if (txt1 == NULL)
       {
               //  0123456789012345"
           txt1 = "w1retap LCD test";
       }
       if (txt2 == NULL)
       {
           time_t now;
           struct tm *tm;
           now=time(NULL);
           tm = localtime(&now);
           txt2 = malloc(20);
           strftime(txt2, 20, "%FT%H:%M", tm);
       }
       int i,rv0, rv00=0, rv1,rv2=0;
       rv0 = init_lcd(portnum, ident);
       rv1 = send_text_lcd(portnum, txt1, 1, 1);
       rv00 = init_lcd(portnum, ident);
       rv2 = send_text_lcd(portnum, txt2, 2, 0);

       fputs( "---",stdout);
       for(i=0; i<16;i++)
       {
           printf("%01x", i);
       }
       fputs( "---\n",stdout);
       printf("1: %s => %d\n2: %s => %d\n", txt1, rv1, txt2, rv2);
       printf("%d %d\n", rv0, rv00);
       if(coupler)
       {
           w1_set_coupler(portnum, cident, 0);
       }
   }
   return 0;
}
