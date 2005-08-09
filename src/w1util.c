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
#include <sqlite3.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>
#include <gmodule.h>
#include "w1retap.h"


char * w1_get_from_home(const char *f)
{
    char *fname = NULL;
    char *p,*q;
    if(NULL != f && NULL != (p = getenv("HOME")))
    {
        fname = malloc(strlen(p) + strlen(f) + 2);
        q = stpcpy(fname, p);
        *q++ = '/';
        stpcpy(q,f);
    }
    return fname;
}

void w1_enumdevs(w1_device_t * w)
{
    if(strncasecmp("temp", w->devtype, 4) == 0)
    {
        w->stype=W1_TEMP;
    }
    else if(strncasecmp("humid", w->devtype, 5) == 0)
    {
        w->stype=W1_HUMID;
    }
    else if(strncasecmp("pres", w->devtype, 4) == 0)
    {
        w->stype=W1_PRES;
    }
    else if(strncasecmp("rain", w->devtype, 4) == 0)
    {
        w->stype=W1_RAIN;
    }
}

void w1_freeup(w1_devlist_t * w1)
{
    int i;
    w1_device_t * devs = NULL;
    void (*func)(void);
    
    for(i = 0; i < w1->ndll; i++)
    {
        if (w1->dlls[i].type == 'l' && NULL != w1->dlls[i].handle)
        {
            if(g_module_symbol(w1->dlls[i].handle, "w1_cleanup",
                               (gpointer *)&func))
            {
                (func)();
            }
        }
    }
    
    for(devs=w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->serial)free(devs->serial);
        if(devs->devtype) free(devs->devtype);
        if(devs->s[0].abbrv) free(devs->s[0].abbrv);
        if(devs->s[0].name) free(devs->s[0].name);
        if(devs->s[0].units) free(devs->s[0].units);        
        if(devs->s[1].abbrv) free(devs->s[1].abbrv);
        if(devs->s[1].name) free(devs->s[1].name);
        if(devs->s[1].units) free(devs->s[1].units);                 
    }
    free(w1->devs);
    w1->numdev = 0;
    w1->devs = NULL;
}

void logtimes(time_t now, char *tbuf)
{
    struct tm *tm;
    tm = localtime(&now);
    strftime(tbuf, TBUF_SZ, "%FT%T%z", tm);
}

void w1_tmpfilelog (w1_devlist_t *w1)
{
    char line[256];
    *line = 0;
    int n = 0;
    int i;
    w1_device_t *devs;
    
    for(devs=w1->devs, i = 0; i < w1->numdev; i++,devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < 2; j++)
            {
                if(devs->s[j].valid)
                {
                    n += sprintf(line+n, "%s=%.2f %s\n",
                                 devs->s[j].abbrv, devs->s[j].value,
                                 (devs->s[j].units) ? (devs->s[j].units) : ""
                                 );
                }
            }
        }
    }
    if(n)
    {
        char tbuf[TBUF_SZ];
        logtimes(w1->logtime, tbuf);
        n += sprintf(line+n,"udate=%ld\ndate=%s\n", w1->logtime, tbuf);
        
        int fd = open("/tmp/.w1retap.dat", O_WRONLY|O_CREAT|O_TRUNC, 0664);
        flock(fd, LOCK_EX);
        write(fd,line,n);
        flock(fd, LOCK_UN);        
        close(fd);
    }
}

FILE * w1_file_open(char *logfile)
{
    FILE *lfp = NULL;

    if(logfile == NULL)
    {
        lfp = stdout;
        setvbuf(lfp, (char *)NULL, _IOLBF, 0);
    }
    else
    {
        char md[2] = {"w"}, *pmd;
        if(NULL != (pmd = strchr(logfile,':')))
        {
            if(*(pmd+1) == 'a')
            {
                *md = 'a';
                *pmd = '\0';
            }
            lfp = fopen(logfile, md);
            if(*pmd) *pmd = ':';
        }
    }
    return lfp;
}
