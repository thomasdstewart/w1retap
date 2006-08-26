#include <stdio.h>
#include "ownet.h"

int main(int argc, char **argv)
{	
    unsigned char lc,blk[] =
    {0x54,0x3d,0x1a,0x2b,0x64,0x48,0x3d,0x05,0xd5,0x97};
    int i;
    setcrc8(0,0);
    for(i = 2; i < 5; i++)
    {
	lc = docrc8(0, blk[i]);
    }
    printf("CRC %d\n", lc);
    return 0;
}
