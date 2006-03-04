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
#include <math.h>

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

void w1_replog(w1_devlist_t *w1, const char *fmt,...)
{
    static FILE *fp;
    va_list va;
    char *p;
    int nc;
    
    va_start (va, fmt);
    if ((nc = vasprintf (&p, fmt, va)) != -1)
    {
        int n;
        w1->lastmsg = p;
        for(n = 0; n < w1->ndll; n++)
        {
            if (w1->dlls[n].type == 'r' && w1->dlls[n].func)
            {
                (w1->dlls[n].func)(w1, w1->dlls[n].param);
            }
        }
        w1->lastmsg = NULL;
        
        if(w1->repfile)
        {
            if(fp == NULL)
            {
                if(*w1->repfile != '-')
                {
                    fp = fopen(w1->repfile, "a");
                }
                else
                {
                    fp = stderr;
                }
            }
            if(fp)
            {
                char s[64];
                logtimes(w1->logtime, s);
                fputs(s, fp);
                fputc(' ', fp);                
                fputs(p, fp);
                fputc('\n', fp);
                fflush(fp);
            }
        }
        free(p);
    }
    va_end (va);
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

static int w1_validate(w1_devlist_t *w1, w1_sensor_t *s)
{
    int chk = 0;
    float act =  s->value;
    float rate = 0;
    
    s->valid = 1;

    if((s->flags & W1_RMIN) && (s->value < s->rmin))
    {
        s->value = (s->ltime) ? s->lval : s->rmin;
        chk = 1;
    }
    if((s->flags & W1_RMAX) && (s->value > s->rmax))
    {
        s->value = (s->ltime) ? s->lval : s->rmax;
        chk = 2;
    }

    if(chk == 0 && (s->flags & W1_ROC))
    {
        if (s->ltime > 0 && s->ltime != w1->logtime)
        {
            rate = fabs(s->value - s->lval) * 60.0 /
                (w1->logtime -s->ltime);
            if (rate > s->roc)
            {
                s->valid = 0;
                s->value = s->lval;
                chk = 3;
            }
        }
        if(s->valid)
        {
            s->ltime = w1->logtime;
            s->lval = s->value;
        }
    }
    if(chk != 0)
    {
        w1_replog (w1, "%s %.2f %.2f %.2f %.2f %d %d (%d)",
                   s->abbrv, s->value, act, rate,
                   s->lval, s->ltime, w1->logtime, chk);
    }
    return (int)s->valid;
}

static int w1_read_temp(w1_devlist_t *w1, w1_device_t *w)
{
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    if(w1_check_device(w1->portnum, w->serno))
    {
        if (ReadTemperature(w1->portnum, w->serno, &w->s[0].value))
        {
            w1_validate(w1, &w->s[0]);
        }
    }
    else
    {
        w->s[0].valid = 0;
    }

    return (w->s[0].valid);
}

static int w1_read_rainfall(w1_devlist_t *w1, w1_device_t *w)
{
    unsigned long cnt;
    int nv = 0;
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    if(w1_check_device(w1->portnum, w->serno))
    {
        if(w->s[0].abbrv)
        {
            if((w->s[0].valid = ReadCounter(w1->portnum, w->serno, 14, &cnt)))
            {
                w->s[0].value = cnt;
                nv += w1_validate(w1, &w->s[0]);
            }
        }
    
        if(w->s[1].abbrv)
        {
            if((w->s[1].valid = ReadCounter(w1->portnum, w->serno, 15, &cnt)))
            {
                w->s[1].value = cnt;
                nv += w1_validate(w1, &w->s[1]);
            }
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    return nv;
}

static int w1_read_humidity(w1_devlist_t *w1, w1_device_t *w)
{
    float humid=0,temp=0;
    float vdd =0 ,vad =0 ,vddx =0;
    int nv = 0;
    char vind=' ';
    
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    if(w1_check_device(w1->portnum, w->serno))
    {
        vdd = vddx = ReadAtoD(w1->portnum,TRUE, w->serno);
        vad = ReadAtoD(w1->portnum, FALSE, w->serno);
        temp = Get_Temperature(w1->portnum, w->serno);

        if(vdd > 5.8)
        {

            vdd = (float)5.8;
            vind='+';
        }
        else if(vdd < 4.0)
        {
            vdd = (float) 4.0;
            vind = '-';
        }

        humid = (((vad/vdd) - 0.16)/0.0062)/(1.0546 - 0.00216 * temp);

        if(w->s[0].name)
        {
            w->s[0].value = (strcasestr(w->s[0].name, "Humidity"))
                ? humid : temp;
            nv += w1_validate(w1, &w->s[0]);
        }
        else
        {
            w->s[0].valid = 0;
        }

        if(w->s[1].name)
        {
            w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                ? temp : humid;
            nv += w1_validate(w1, &w->s[1]);
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
    
    if(w->s[0].valid == 0 || w->s[1].valid == 0 )
    {
        w1_replog (w1, "%f %f %f %f %c", vddx, vad, temp, humid, vind);
    }
    return nv;
}

static int w1_read_pressure(w1_devlist_t *w1, w1_device_t *w)
{
    float temp,pres;
    int nv = 0;
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    
    if(w1_check_device(w1->portnum, w->serno))
    {
        if(w->init == 1)
        {
            if(Init_Pressure(w1->portnum, w->serno))
            {
                w->init = 2;
            }
        }
        if (w->init == 2)
        {
            if(ReadPressureValues (w1->portnum, &temp, &pres) )
            {
                if(w->s[0].name)
                {
                    w->s[0].value = (strcasestr(w->s[0].name, "Pres"))
                        ? pres : temp;
                    nv += w1_validate(w1, &w->s[0]);
                }
                else
                {
                    w->s[0].valid = 0;
                }
                
                if(w->s[1].name)
                {
                    w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                        ? temp : pres;
                    nv += w1_validate(w1, &w->s[1]);
                }
                else
                {
                    w->s[1].valid = 0;
                }
            }
            else
            {
                w->init = 0;
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
        else if (w1->dlls[n].type == 'r')
        {
            if(!(g_module_symbol (w1->dlls[n].handle,
                                 "w1_report",
                                  (gpointer *)&w1->dlls[n].func)))
            {
                perror("replogger");
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
                    fprintf(stderr, "\t1: %s %s %s",
                            w1->devs[i].s[j].abbrv, w1->devs[i].s[j].name,
                            (w1->devs[i].s[j].units) ?
                            (w1->devs[i].s[j].units) : "");
                    if(w1->devs[i].s[j].flags & W1_ROC)
                    {
                        fprintf(stderr,", %.4f /min", w1->devs[i].s[j].roc);
                    }
                    if(w1->devs[i].s[j].flags & W1_RMIN)
                    {
                        fprintf(stderr,", min=%.2f", w1->devs[i].s[j].rmin);
                    }
                    if(w1->devs[i].s[j].flags & W1_RMAX)
                    {
                        fprintf(stderr,", max=%.2f", w1->devs[i].s[j].rmax);
                    }
                    fputc('\n', stderr);
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

        for(d = w1->devs, n = 0; n < w1->numdev; n++, d++)
        {
            switch(d->stype)
            {
                case W1_TEMP:
                    nv += w1_read_temp(w1, d);
                    break;
                    
                case W1_HUMID:
                    nv += w1_read_humidity(w1, d);
                    break;
                    
                case W1_PRES:
                    nv += w1_read_pressure(w1, d);
                    break;
                    
                case W1_RAIN:
                    nv += w1_read_rainfall(w1, d);
                    break;
                    
                case W1_INVALID:
                default:
                    break;
            }
        }

    }
    return nv;
}

static void usage(void)
{
    fputs("w1retap v" VERSION " (c) 2005,2006 Jonathan Hudson\n"
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
    int immed = 1;
    int n;
    int c;
    char *p;
    
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

    if((p = getenv("W1RCFILE")))
    {
	w1->rcfile = strdup(p);
    }
    read_config(w1);

    while((c = getopt (argc, argv,"r:dt:i:TvNh?Vw")) != EOF)
    {
        switch(c)
        {
            case 'w':
                immed = 0;
                break;
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
            case 'r':
                w1->repfile = strdup(optarg);
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
    w1->logtime =time(NULL);
    w1_replog (w1, "Startup w1retap v" VERSION);
    
    on_exit(cleanup, w1);
    w1_show(w1, 0);
    if(w1->daemonise)
        daemon(0,0);
    
    while(1)
    {
        int nv = 0;
        
        if(immed)
        {
            nv = w1_read_all_sensors(w1);
        
            if(nv)
            {
                for(n = 0; n < w1->ndll; n++)
                {
                    if (w1->dlls[n].type == 'l' && w1->dlls[n].func)
                    {
                        (w1->dlls[n].func)(w1, w1->dlls[n].param);
                    }
                }

                if(w1->logtmp)
                {
                    w1_tmpfilelog (w1);
                }
            }
        }
        
        if(w1->delay)
        {
            int ns;
            immed = 1;
            
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
                then.tv_sec = w1->delay * (1 + now.tv_sec / w1->delay);
                then.tv_nsec = 200*1000*1000; /* ensure tick crossed */
                nnow.tv_sec = now.tv_sec;
                nnow.tv_nsec = now.tv_usec * 1000;
                nanosub(&then, &nnow, &req);
                ns  = nanosleep(&req, NULL);
                if(ns == 0)
                {
                    gettimeofday(&now, NULL);
                    if(now.tv_sec % w1->delay == 0)
                    {
                        w1->logtime = now.tv_sec;
                    }
                    else
                    {
                        w1->logtime = then.tv_sec;
                        {
                            FILE *fp = fopen("/tmp/w1retap.tlog", "a");
                            if(fp)
                            {
                                fprintf(fp,
                                        "Start : %d %09d\n"
                                        "Expect: %d %09d\n"
                                        "Actual: %d %09d\n",
                                        then.tv_sec, then.tv_nsec,
                                        nnow.tv_sec, nnow.tv_nsec,
                                        now.tv_sec, now.tv_usec * 1000);
                                fclose(fp);
                            }
                        }
                    }
                    if(w1->verbose)
                        fputs(ctime(&now.tv_sec),stderr);
                }
            } while (ns != 0);
        }
        else
        {
            break;
        }
    }
    return 0;
}
