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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <gmodule.h>

#include "ownet.h"
#include "temp10.h"
#include "atod26.h"
#include "cnt1d.h"
#include "pressure.h"

#include "w1retap.h"

enum W1_sigflag
{
    W1_NOFLAG =0,
    W1_RECONF = (1),
    W1_READALL = (2),
    W1_SHOWCF = (4)
};
    

static volatile enum W1_sigflag sigme;
   
static void sig_hup(int x)
{
    sigme |= W1_RECONF;
}


static void sig_usr1(int x)
{
    sigme |= W1_READALL;
}

static void sig_usr2(int x)
{
    sigme |= W1_SHOWCF;
}

static int  w1_check_device(int portnum, u_char serno[8])
{
    u_char thisdev[8];
    int found = 0;
        
    owFamilySearchSetup(portnum,serno[0]);
    while (owNext(portnum,TRUE, FALSE))
    {
        owSerialNum(portnum, thisdev, TRUE);
        if(0 == memcmp(thisdev, serno, sizeof(thisdev)))
        {
            found = 1;
            break;
        }
    }
    return found;
}


static void w1_make_serial(char * asc, unsigned char *bin)
{
    int i,j;
    for(i = j = 0; i < 8; i++)
    {
        bin[i] = ToHex(asc[j]) << 4; j++;
        bin[i] += ToHex(asc[j]); j++;
    }
}

static int w1_read_temp(int portnum, w1_device_t *w)
{
    static unsigned char serno[8];
    if(w->init  == 0)
    {
        w1_make_serial(w->serial, serno);
        w->init = 1;
    }

    if(w1_check_device(portnum, serno))
    {
        w->s[0].valid = ReadTemperature(portnum, serno, &w->s[0].value);
    }
    else
    {
        w->s[0].valid = 0;
    }

    return (w->s[0].valid);
}

static int w1_read_rainfall(int portnum, w1_device_t *w)
{
    static unsigned char serno[8];
    unsigned long cnt;
    int nv = 0;
    
    if(w->init  == 0)
    {
        w1_make_serial(w->serial, serno);
        w->init = 1;
    }

    if(w1_check_device(portnum, serno))
    {
        if(w->s[0].abbrv)
        {
            if((w->s[0].valid = ReadCounter(portnum, serno, 14, &cnt)))
            {
                w->s[0].value = cnt;
                nv++;
            }
        }
    
        if(w->s[1].abbrv)
        {
            if((w->s[1].valid = ReadCounter(portnum, serno, 15, &cnt)))
            {
                w->s[1].value = cnt;
                nv++;
            }
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    return nv;
}

static int w1_read_humidity(int portnum, w1_device_t *w)
{
    static unsigned char serno[8];
    float humid,temp;
    float Vdd,Vad;
    int nv = 0;
    
    if(w->init  == 0)
    {
        w1_make_serial(w->serial, serno);
        w->init = 1;
    }

    if(w1_check_device(portnum, serno))
    {
        Vdd = ReadAtoD(portnum,TRUE, serno);
        if(Vdd > 5.8)
        {
            Vdd = (float)5.8;
        }
        else if(Vdd < 4.0)
        {
            Vdd = (float) 4.0;
        }
        
        Vad = ReadAtoD(portnum, FALSE, serno);
        temp = Get_Temperature(portnum, serno);
        
        humid = (((Vad/Vdd) - 0.16)/0.0062)/(1.0546 - 0.00216 * temp);
        if(humid > 100)
        {
            humid = 100;
        }
        else if(humid < 0)
        {
            humid = 0;
        }
        
        if(w->s[0].name)
        {
            w->s[0].value = (strcasestr(w->s[0].name, "Humidity"))
                ? humid : temp;
            w->s[0].valid = 1;
            nv++;
        }
        else
        {
            w->s[0].valid = 0;
        }
        
        if(w->s[1].name)
        {
            w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                ? temp : humid;
            w->s[1].valid = 1;
            nv++;            
        }
        else
        {
            w->s[1].valid = 0;
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    return nv;
}

static int w1_read_pressure(int portnum, w1_device_t *w)
{
    static unsigned char serno[8];
    float temp,pres;
    int nv = 0;
    
    if(w->init  == 0)
    {
        w1_make_serial(w->serial, serno);
        w->init = 1;
    }
    
    if(w1_check_device(portnum, serno))
    {
        if(w->init == 1)
        {
            if(Init_Pressure(portnum, serno))
            {
                w->init = 2;
            }
        }
        
        if(ReadPressureValues (portnum, &temp, &pres) &&
           (pres > 900 && pres < 1100) )
        {
            if(w->s[0].name)
            {
                w->s[0].value = (strcasestr(w->s[0].name, "Pres"))
                    ? pres : temp;
                w->s[0].valid = 1;
                nv++;
            }
            else
            {
                w->s[0].valid = 0;
            }
        
            if(w->s[1].name)
            {
                w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                    ? temp : pres;
                w->s[1].valid = 1;
                nv++;
            }
            else
            {
                w->s[1].valid = 0;
            }
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
        w->init  = 1;
    }
    return nv;
}

static void do_init(w1_devlist_t *w1)
{
    int n;
    int done_i = 0;
    void (*func)(w1_devlist_t *w1, char *);
    
    for(n = 0; n < w1->ndll; n++)
    {
        if(w1->dlls[n].type == 'c' && done_i == 0)
        {
            if((g_module_symbol (w1->dlls[n].handle,
                                 "w1_init", (gpointer *)&func)))
            {
                (func)(w1, w1->dlls[n].param);
                done_i = 1;
            }
            else
            {
                perror("init");
                exit(1);
            }
        }
        else if (w1->dlls[n].type == 'l')
        {
            if(!(g_module_symbol (w1->dlls[n].handle,
                                 "w1_logger",
                                  (gpointer *)&w1->dlls[n].func)))
            {
                perror("logger");
                exit(1);
            }
        }
    }

    if (done_i == 0)
    {
        perror("Init fails");
        exit(1);
    }
}

void dll_parse(w1_devlist_t *w1, int typ, char *params)
{
    char *sep;
    char *sofile = NULL;
    
    if(w1->ndll == (MAXDLL-1))
    {
        perror("Too many DLLs");
        exit(1);
    }

    sep = strpbrk(params, ":;=");
    if(sep)
    {
        *sep++ = 0;
    }

    if(*params == '.' || *params == '/')
    {
        sofile = strdup(params);
    }
    else
    {
        sofile = g_module_build_path(MODULE_DIR, params);
    }

    if(NULL != (w1->dlls[w1->ndll].handle = g_module_open(sofile, G_MODULE_BIND_LAZY)))
    {
        w1->dlls[w1->ndll].type = typ;            
        if(sep && *sep)
        {
            w1->dlls[w1->ndll].param = strdup(sep);
        }
        w1->ndll++;
    }
    free(sofile);
}

static void cleanup(int n, void * w)
{
    int i;
    w1_devlist_t *w1 = w;
    
    w1_freeup(w1);
    for(i = 0; i < w1->ndll; i++)
    {
        if (w1->dlls[i].handle)
        {
            g_module_close(w1->dlls[i].handle);
            w1->dlls[i].handle = 0;
        }
        
        if(w1->dlls[i].param)
        {
            free(w1->dlls[i].param);
            w1->dlls[i].param = NULL;
        }
        w1->dlls[w1->ndll].type = 0;            
    }
    w1->ndll = 0;
    free(w1->rcfile);
    w1->rcfile = NULL;
}

static void w1_show(w1_devlist_t *w1, int forced)
{
    if(w1->verbose || forced)
    {
        int i;
        
        if(w1->numdev)
        {
            fputs("Sensors:\n", stderr);
        }
        
        for(i = 0; i < w1->numdev; i++)
        {
            int j;
            
            fprintf(stderr, "%s %s\n",
                    w1->devs[i].serial, w1->devs[i].devtype);

            for(j = 0; j < 2; j++)
            {
                if(w1->devs[i].s[j].abbrv)
                {
                    fprintf(stderr, "\t1: %s %s %s\n",
                            w1->devs[i].s[j].abbrv, w1->devs[i].s[j].name,
                            (w1->devs[i].s[j].units) ?
                            (w1->devs[i].s[j].units) : "");
                }
            }
        }

        if(w1->ndll)
        {
            fputs("Plugins:\n", stderr);
        }
        
        for(i = 0; i < w1->ndll; i++)
        {
            fprintf(stderr, "%2d: %c [%p] %s => %s\n",
                    i,
                    w1->dlls[i].type,
                    w1->dlls[i].handle,
                    g_module_name(w1->dlls[i].handle),
                    (w1->dlls[i].param) ? w1->dlls[i].param : "");
        }
    }
}

static int w1_read_all_sensors(w1_devlist_t *w1)
{

    int nv = 0;

    if(w1->doread)
    {
        w1_device_t *d;
        int n;
        time_t now = time(NULL);

        for(d = w1->devs, n = 0; n < w1->numdev; n++, d++)
        {
            switch(d->stype)
            {
                case W1_TEMP:
                    nv += w1_read_temp(w1->portnum, d);
                    break;
                    
                case W1_HUMID:
                    nv += w1_read_humidity(w1->portnum, d);
                    break;
                    
                case W1_PRES:
                    nv += w1_read_pressure(w1->portnum, d);
                    break;
                    
                case W1_RAIN:
                    nv += w1_read_rainfall(w1->portnum, d);
                    break;
                    
                case W1_INVALID:
                default:
                    break;
            }
        }

        if(nv)
        {
            w1->logtime = now;
        }
    }
    return nv;
}

static void usage(void)
{
    fputs("w1retap v" VERSION " (c) 2005 Jonathan Hudson\n"
          "$ w1retap [-T] [-d] [-i interface] [-N] [-v] [-t sec]\n", stderr);
    exit(1);
}

static inline void nanosub(struct timespec *a, struct timespec *b,
                           struct timespec *result)
{
    result->tv_sec = a->tv_sec - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0)
    {
        --result->tv_sec;
        result->tv_nsec += 1000000000;
    }
}

int main(int argc, char **argv)
{

    w1_devlist_t *w1;
    struct sigaction act ={{0}};
    int doversion = 0;
    int n;
    int c;

    
    if(!g_module_supported())
    {
        exit(2);
    }
    
    act.sa_flags = SA_RESTART;
    sigemptyset(&(act.sa_mask));

    act.sa_handler = sig_usr1;
    sigaction (SIGUSR1, &act, NULL);

    act.sa_handler = sig_usr2;
    sigaction (SIGUSR2, &act, NULL);

    act.sa_handler = sig_hup;
    sigaction (SIGHUP, &act, NULL);

    w1 = calloc(1, sizeof(w1_devlist_t));
    w1->logtmp = 1;
    w1->portnum = -1; 
    w1->doread = 1;
    
    read_config(w1);
    
    while((c = getopt (argc, argv,"dt:i:TvNh?V")) != EOF)
    {
        switch(c)
        {
            case 'T':
                w1->logtmp = 0;
                break;
            case 'd':
                w1->daemonise = 1;
                break;
            case 'i':
                w1->iface = strdup(optarg);
                break;
            case 't':
                w1->delay = strtol(optarg, NULL, 0);
                break;
            case 'N':
                w1->doread = 0;
                break;
            case 'v':
                w1->verbose = 1;
                break;
            case 'V':
                w1->verbose = 1;
                doversion = 1;
                break;
            case 'h':
            case '?':
                usage();
                break;
        }
    }


    if(w1->verbose)
    {
        fputs("w1retap v" VERSION " (c) 2005 Jonathan Hudson\n", stderr);
        if(doversion)
        {
            exit (0);
        }
    }
    
    if(w1->doread)
    {
        if(w1->iface == NULL)
            w1->iface = strdup("DS2490-1");

        if((w1->portnum = owAcquireEx(w1->iface)) < 0)
        {
            OWERROR_DUMP(stdout);
            exit(1);
        }
    }
    
    do_init(w1);
    on_exit(cleanup, w1);
    w1_show(w1, 0);
    if(w1->daemonise)
        daemon(0,0);
    
    while(1)
    {
        int nv = w1_read_all_sensors(w1);
        
        if(nv)
        {
            for(n = 0; n < w1->ndll; n++)
            {
                if (w1->dlls[n].func)
                {
                    (w1->dlls[n].func)(w1, w1->dlls[n].param);
                }
            }

            if(w1->logtmp)
            {
                w1_tmpfilelog (w1);
            }
        }
    
        if(w1->delay)
        {
            int ns;
            do
            {
                struct timeval now;
                struct timespec then, req, nnow;

                ns = 0;

                if(sigme & W1_READALL)
                {
                    sigme &= ~W1_READALL;                    
                    nv = w1_read_all_sensors(w1);
                    w1_tmpfilelog(w1);
                }
                
                if(sigme & W1_RECONF)
                {
                    sigme &= ~W1_RECONF;
                    cleanup (0, w1);
                    read_config(w1);
                    do_init(w1);
                    w1_show(w1, 0);
                }

                if(sigme & W1_SHOWCF)
                {
                    sigme &= ~W1_SHOWCF;
                    w1_show(w1, 1);
                }
                gettimeofday(&now, NULL);

                if(w1->verbose)
                {
                    fputs(ctime(&now.tv_sec),stderr);
                }
                
                then.tv_sec = w1->delay * (1 + now.tv_sec / w1->delay);
                then.tv_nsec = 20*1000*1000; /* ensure tick crossed */
                nnow.tv_sec = now.tv_sec;
                nnow.tv_nsec = now.tv_usec * 1000;
                nanosub(&then, &nnow, &req);
                ns  = nanosleep(&req, NULL);
            } while (ns != 0);
        }
        else
        {
            break;
        }
    }
    return 0;
}

