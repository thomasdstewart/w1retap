/*
 * Copyright (C) 2005 Jonathan Hudson <jh+w1retap@daria.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>
#include "w1retap.h"


void w1_logger (w1_devlist_t *w1, char *logfile)
{
    int i;
    char timb[TBUF_SZ];
    w1_device_t *devs;
    FILE *lfp;
    
    if(logfile == NULL)
    {
        lfp = stdout;
        setvbuf(lfp, (char *)NULL, _IOLBF, 0);
    }
    else
    {
        if(*logfile == '|')
        {
            lfp = popen(logfile+1,"w");
        }
        else
        {
            lfp = fopen(logfile, "a");
        }
    }
  
    if(lfp == NULL)
    {
        return;
    }
    
    logtimes(w1, w1->logtime, timb);
    fprintf(lfp, "\"%s\"", timb);
    
    for(devs=w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < devs->ns; j++)
            {
                if(devs->s[j].valid)
                {
                    fprintf(lfp, ",\"%s\",%f,\"%s\"",
                            devs->s[j].abbrv, devs->s[j].value,
                            (devs->s[j].units) ? (devs->s[j].units) : ""
                            );
                }
            }
        }
    }
    fputc('\n',lfp);
    if(logfile)
    {
        if (*logfile == '|')
        {
            pclose(lfp);
        }
        else
        {
            fclose(lfp);
        }
    }
}
