#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ownet.h"
#include "findtype.h"
#include "swt1f.h"
#include "devlist.h"

void listdev(uchar *id)
{
    w1devs_t *w;

    for(w = devlist; w->family; w++)
    {
        if (w->family == id[0])
        {
            printf("\t%s:%s", (w->dev) ? w->dev : "", w->name);
            break;
        }
    }
}


void showid(uchar *id)
{
    int i;
    for(i=0; i<8; i++)
    {
        printf("%02X", id[i]);
    }
    listdev(id);
    printf("\n");
}

void search_coupler (int portnum, uchar *id)
{
    int j,k,res;
    uchar a[4];
    uchar snum[8];

    showid(id);           
    for(j = 1; j < 3; j++)
    {
        k = 0;
       if (j == 1)
       {
           SetSwitch1F(portnum, id, 4, 2, a, TRUE);
       }
       else
       {
           SetSwitch1F(portnum, id, 2, 2, a, TRUE);
       }
       
       res = owFirst(portnum, FALSE, FALSE);
       while (res)
       {
           k++;
           printf("\t(%s.%d) ",(j==1) ? "Main" : " Aux" ,k);
           owSerialNum(portnum,snum,TRUE);
           showid(snum);
           SetSwitch1F(portnum, id, j, 2, a, TRUE);           
           res = owNext(portnum, FALSE, FALSE);
       }
    }
}

int main(int argc, char **argv)
{
   int rslt,cnt;
   int portnum=0;
   uchar snum[8];
   int nc = 0;
   uchar cplr[32][8];
   
   if (argc != 2)
   {
      printf("1-Wire device name required "
             " (e.g: \"/dev/ttyS0\" (DS2480), \"DS2490-1\" (USB) \n");
      exit(1);
   }

   if((portnum = owAcquireEx(argv[1])) < 0)
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   nc = FindDevices(portnum, cplr, SWITCH_FAMILY, 32);
   owSkipFamily(portnum);

   int j;
   for(j = 0; j < nc; j++)
   {
       uchar a[3];
       SetSwitch1F(portnum, cplr[j], 0, 2, a, TRUE);
   }
   cnt = 0;
   rslt = owFirst(portnum, TRUE, FALSE);
   while (rslt)
   {
       owSerialNum(portnum,snum,TRUE);
       if (snum[0] != 0x1f)
       {
           cnt++;
           printf("(%d) ",cnt);
           showid(snum);           
       }
       rslt = owNext(portnum, TRUE, FALSE);
   }
   for(j = 0; j < nc; j++)
   {
       cnt++;
       printf("(%d) ",cnt);
       search_coupler(portnum, cplr[j]);
   }
   owRelease(portnum);
   return 0;
}
