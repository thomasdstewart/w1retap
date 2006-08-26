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
#include <gmodule.h>
#include <math.h>
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

static w1_params_t * w1_dev_params(char *params)
{
    w1_params_t *p = NULL;

    if(params)
    {
        char *sp = strdup(params);
        char *r,*s;
        int j = 0;

        // specificly use strtok() to elimiate repeated spaces
        
        for(r = sp; (s = strtok(r,"|: ")); j++,r=NULL)
            ; // NULL loop to count entries;

        free(sp);
        if(j)
        {
            sp = strdup(params);
            p = calloc(1,(sizeof(int) + j* sizeof(double)));
            p->num = j;
            for(j =0, r = sp; (s = strtok(r,"|: ")); r=NULL,j++)    
            {
                p->values[j] = strtod(s,NULL);
            }
            free(sp);
        }
    }
    return p;
}

w1_sensor_t * w1_find_sensor(w1_devlist_t * w1, const char * s)
{
    w1_sensor_t *sensor = NULL;
    w1_device_t * devs;
    int i,j;
    
    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        for (j = 0; j < 2; j++)
        {
            if(devs->s[j].abbrv && (0 == strcmp(devs->s[j].abbrv, s)))
            {
                sensor = &devs->s[j];
                break;
            }
        }
    }
    return sensor;
}

void w1_set_device_data_index(w1_device_t * w1, int idx, char *sv)
{
    switch (idx)
    {
        case 0:
            w1->serial = sv;
            break;
        case 1:
            w1->devtype = sv;
            break;
        case 2:
            w1->s[0].abbrv = sv;
            break;
        case 3:
            w1->s[0].name = sv;
            break;
        case 4:
            w1->s[0].units = sv;
            break;
        case 5:
            w1->s[1].abbrv = sv;
            break;
        case 6:
            w1->s[1].name = sv;
            break;
        case 7:
            w1->s[1].units = sv;
            break;
        case 8:
            w1->params = w1_dev_params(sv);
            break;
    }
}

void w1_set_device_data(w1_device_t * w1, const char *fnam, char *sv)
{
    if (0 == strcmp(fnam, "device"))
    {
        w1->serial = sv;
    }
    else if (0 == strcmp(fnam, "type"))
    {
        w1->devtype = sv;
    }
    else if (0 == strcmp(fnam, "abbrv1"))
    {
        w1->s[0].abbrv = sv;
    }
    else if (0 == strcmp(fnam, "name1"))
    {
        w1->s[0].name = sv;
    }
    else if (0 == strcmp(fnam, "units1"))
    {
        w1->s[0].units = sv;
    }
    else if (0 == strcmp(fnam, "abbrv2"))
    {
        w1->s[1].abbrv = sv;
    }
    else if (0 == strcmp(fnam, "name2"))
    {
        w1->s[1].name = sv;
    }
    else if (0 == strcmp(fnam, "units2"))
    {
        w1->s[1].units = sv;
    }
    else if (0 == strcmp(fnam, "params"))
    {
        w1->params = w1_dev_params(sv);
    }
}

void w1_enumdevs(w1_device_t * w)
{
#define MATCHES(__p1) (strncasecmp(__p1, w->devtype, (sizeof(__p1)-1)) == 0)
    
    if( MATCHES("TEMPERATURE") || MATCHES("DS1820") || MATCHES("DS18S20") )
    {
        w->stype=W1_TEMP;
    }
    else if( MATCHES("HUMIDITY") || MATCHES("TAI8540") )
    {
        w->stype=W1_HUMID;
    }
    else if( MATCHES("PRESSURE") || MATCHES("TAI8570") )
    {
        w->stype=W1_PRES;
    }
    else if( MATCHES("RAIN") || MATCHES("COUNTER")
             || MATCHES("TAI8575") || MATCHES("DS2423"))
    {
        w->stype=W1_COUNTER;
    }
    else if( MATCHES("Bray") || MATCHES("MPX4115A") )
    {
        w->stype=W1_BRAY;
    }
    else if( MATCHES ("sht11") )
    {
        w->stype=W1_SHT11;
    }
    else if( MATCHES ("WINDVANE") || MATCHES("WEATHERVANE") ||
             MATCHES ("TAI8515") )
    {
        w->stype=W1_WINDVANE;
    }
    else if( MATCHES("Coupler") || MATCHES("DS2409") )
    {
        w->stype=W1_COUPLER;
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
                               (void *)&func))
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
        if(devs->c) free(devs->c);
        if(devs->params) free(devs->params);        
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
        }
	lfp = fopen(logfile, md);
	if(pmd) *pmd = ':';
    }
    return lfp;
}
