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


void w1_init(w1_devlist_t * w1, char *fname)
{
    FILE *fp;
    int n = 0;
    w1_device_t * devs = NULL;
    int alldev = 0;
    short allocf = 0;
    
    if(fname == NULL)
    {
        fname = w1_get_from_home(".config/w1retap/sensors");
        allocf = !!fname;
    }
    
    if((fp = fopen(fname, "r")))
    {
        char line[256];
        while(fgets(line, sizeof(line), fp))
        {
            int j;
            char *s,*p;
            
            if(isalnum(*line))
            {
                if(NULL != (s = strchr(line,'\n')))
                {
                    *s = 0;
                }
                
                if(n == alldev)
                {
                    alldev += ALLOCDEV;
                    devs = realloc(devs, sizeof(w1_device_t)*alldev);
                    memset(devs+n, 0, sizeof(w1_device_t)*ALLOCDEV);
                }

                for(j =0, s = line; (p = strtok(s,"|:")); s=NULL, j++)
                {
                    
                    switch (j)
                    {
                        case 0:
                            devs[n].serial = strdup(p);
                            break;
                        case 1:
                            devs[n].devtype = strdup(p);
                            break;
                        case 2:
                            devs[n].s[0].abbrv = strdup(p);
                            break;
                        case 3:
                            devs[n].s[0].name = strdup(p);
                            break;
                        case 4:
                            devs[n].s[0].units = strdup(p);
                            break;
                        case 5:
                            devs[n].s[1].abbrv = strdup(p);
                            break;
                        case 6:
                            devs[n].s[1].name = strdup(p);
                            break;
                        case 7:
                            devs[n].s[1].units = strdup(p);
                            break;
                    }
                }
                w1_enumdevs(devs+n);
                n++;
            }
        }
        fclose(fp);
        if(allocf && fname)
            free(fname);
    }
    else
    {
        exit(1);
    }
    w1->numdev = n;
    w1->devs=devs;
}


void w1_logger (w1_devlist_t *w1, char *logfile)
{
    int i;
    char timb[TBUF_SZ];
    w1_device_t *devs;
    
    static FILE *lfp;

    if(lfp == NULL)
    {
        lfp = w1_file_open(logfile);
    }
    
    if(lfp == NULL)
    {
        return;
    }
    
    logtimes(w1->logtime, timb);

    for(devs=w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < 2; j++)
            {
                if(devs->s[j].valid)
                {
                    fprintf(lfp, "%s %s %f %s\n",
                            timb, devs->s[j].abbrv, devs->s[j].value,
                            (devs->s[j].units) ? (devs->s[j].units) : ""
                            );
                }
            }
        }
    }
    fflush(lfp);
}



