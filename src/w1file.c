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
    int n = 0, nn = 0;
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
                char *serno=NULL;
                char *devtype=NULL;
                if(NULL != (s = strchr(line,'\n')))
                {
                    *s = 0;
                }
                
                for(j =0, s = line; (p = strsep(&s,"|:"));  j++)
                {
                    char *sv = (p && *p) ? strdup(p) : NULL;
                    switch(j)
                    {
                        case 0:
                            serno = sv;
                            break;
                        case 1:
                            devtype =sv;
                            if((nn = w1_get_device_index(devs, n, serno, devtype)) == -1)
                            {
                                if(n == alldev)
                                {
                                    alldev += ALLOCDEV;
                                    devs = realloc(devs, sizeof(w1_device_t)*alldev);
                                    memset(devs+n, 0, sizeof(w1_device_t)*ALLOCDEV);
                                }
                                
                                nn = n;
                                n++;
                            }
                            w1_set_device_data_index (devs+nn, 0, serno);
                            w1_set_device_data_index (devs+nn, 1, devtype);
                            w1_enumdevs(devs+nn);
                            break;
                        default:
                            w1_set_device_data_index (devs+nn, j, sv);
                            break;
                    }
                }
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
    
    logtimes(w1->logtime, timb);

    for(devs=w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < devs->ns; j++)
            {
                if(devs->s[j].valid)
                {
                    fprintf(lfp, "%s%s%s%s%f%s%s",
                            timb, w1->log_delim,
                            devs->s[j].abbrv, w1->log_delim,
                            devs->s[j].value, w1->log_delim,
                            (devs->s[j].units) ? (devs->s[j].units) : "");
                    if(w1->log_timet)
                    {
                        fprintf(lfp, "%s%ld", w1->log_delim, w1->logtime);
                    }
                    fputc('\n',lfp);
                }
            }
        }
    }

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
