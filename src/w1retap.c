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
#include <glib.h>
#include <assert.h>
#include <syslog.h>

#include "ownet.h"

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
                                 "w1_init", (void *)&func)))
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
                                  (void *)&w1->dlls[n].func)))
            {
                perror("logger");
                exit(1);
            }
        }
        else if (w1->dlls[n].type == 'r')
        {
            if(!(g_module_symbol (w1->dlls[n].handle,
                                 "w1_report",
                                  (void *)&w1->dlls[n].func)))
            {
                perror("replogger");
                exit(1);
            }
        }
    }

    if (done_i == 0)
    {
        fputs("Init fails: No w1_init() in init= loadable library\n", stderr);
        exit(1);
    }
    w1_initialize_couplers(w1);
}

void dll_parse(w1_devlist_t *w1, int typ, char *params)
{
    char *sep;
    char *sofile = NULL;
    
    if(w1->ndll == (MAXDLL-1))
    {
        fputs("Too many DLLs\n", stderr);
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

/*
static void w1_show_id(uchar *id, char *res)
{
    int i;
    for(i=0; i<8; i++)
    {
        sprintf((res+i*2), "%02X", id[i]);
    }
}
*/


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

            if(w1->devs[i].coupler)
            {
	        char *branch = (w1->devs[i].coupler->branch == COUPLER_AUX_ON) ? "aux" : "main";
                fprintf(stderr, "\tVia coupler: %s, %s\n", w1->devs[i].coupler->coupler_device->serial, branch);
            }

            if(w1->devs[i].params && w1->devs[i].stype !=  W1_COUPLER)
            {
                int j;
                fputs("\tParameters:", stderr);
                for(j =0 ; j < w1->devs[i].params->num; j++)
                {
                    fprintf(stderr, " %f", w1->devs[i].params->values[j]);
                }
                fputc('\n', stderr);
            }
            
            for(j = 0; j < w1->devs[i].ns; j++)
            {
                if(w1->devs[i].s[j].abbrv)
                {
                    fprintf(stderr, "\t%d: %s %s %s",j+1,
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
            char *s;
            char *p=NULL;
            
            if((s=w1->dlls[i].param))
            {
                if ((p = strcasestr(w1->dlls[i].param,"password=")))
                {
                    char *q;
                    s = strdup(w1->dlls[i].param);
                    q = (char*)((s+(p-w1->dlls[i].param))+sizeof("password=")-1);
                    while (*q && !isspace(*q))
                    {
                        *q++='*';
                    }
                }
            }
            else
            {
                s="";
            }
            
            fprintf(stderr, "%2d: %c [%p] %s => %s\n",
                    i,
                    w1->dlls[i].type,
                    w1->dlls[i].handle,
                    g_module_name(w1->dlls[i].handle),s);
            if(p) free(s);
        }

        if(w1->altitude)
        {
            fprintf(stderr,"Normalising pressure for %dm\n",w1->altitude);
        }
        if(w1->vane_offset)
        {
            fprintf(stderr,"Vane offset %d\n",w1->vane_offset);
        }
        if(w1->logtmp)
        {
            fprintf (stderr, "Log file is %s\n", w1->tmpname);
        }
    }
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
    struct sigaction act ={{0}};
    gboolean immed = 1;
    gboolean showvers=0;
    gboolean once=0;
    int n;
    char *p;
    GError *error = NULL;
    GOptionContext *context;
    w1_devlist_t *w1 = calloc(1, sizeof(w1_devlist_t));
    GOptionEntry entries[]=
    {
        {"wait",'w', 0, G_OPTION_ARG_NONE, &immed,
         "At startup, wait until next interval",NULL},
        {"once-only",'1', 0, G_OPTION_ARG_NONE, &once,
         "Read once and exit",NULL},
        {"daemonise",'d', 0, G_OPTION_ARG_NONE, &w1->daemonise,
         "Daemonise (background) application", NULL},
        {"no-tmp-log",'T',0, G_OPTION_ARG_NONE, &w1->logtmp,
         "Disables /tmp/.w1retap.dat logging", NULL},
        {"tmp-log-name",'l',0, G_OPTION_ARG_STRING, &w1->tmpname,
         "Names logging file (/tmp/.w1retap.dat)", "FILE"},
        {"interface", 'i', 0, G_OPTION_ARG_STRING, &w1->iface,
         "Interface device", "DEVICE"},
        {"cycle-time", 't', 0, G_OPTION_ARG_INT, &w1->delay,
         "Time (secs) between device readings", "SECS"},
        {"dont-read",'N', 0, G_OPTION_ARG_NONE, &w1->doread, 
         "Don't read sensors (for debugging)",NULL},
        {"verbose", 'v', 0, G_OPTION_ARG_NONE, &w1->verbose,
         "Verbose messages", NULL},
        {"vane-offset", 'o', 0, G_OPTION_ARG_INT, &w1->vane_offset,
         "Value for N for weather vane (0-15)", "VAL"},
        {"version",'V', 0, G_OPTION_ARG_NONE, &showvers,
         "Display version number (and exit)", NULL},
        {"report-log",'r', 0, G_OPTION_ARG_STRING, &w1->repfile,
         "Report log file", "FILE"},
        {NULL}
    };

    openlog("w1retap", LOG_PID, LOG_USER);

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

    w1->logtmp = 1;
    w1->portnum = -1;

    if((p = getenv("W1RCFILE")))
    {
	w1->rcfile = strdup(p);
    }
    read_config(w1);

    context = g_option_context_new ("- w1retap");
    g_option_context_set_ignore_unknown_options(context,FALSE);
    g_option_context_add_main_entries (context, entries,NULL);
    if(g_option_context_parse (context, &argc, &argv,&error) == FALSE)
    {
        if (error != NULL)
        {
            fprintf (stderr, "%s\n", error->message);
            g_error_free (error);
            exit(1);
        } 
    }

    if(w1->tmpname == NULL)
    {
        w1->tmpname = "/tmp/.w1retap.dat";
    }

    
    w1->doread ^= 1;

    if(showvers)
        w1->verbose = 2;
    
    w1->vane_offset &= 0xf;
    
    if(w1->verbose)
    {
        fputs("w1retap v" VERSION " (c) 2005-2008 Jonathan Hudson\n", stderr);
        if(w1->verbose == 2)
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
    
    w1_show(w1, 0);
    
    if(w1->daemonise)
    {
        // preserve redirected log files unless TTYs
        int iclose;
        iclose = !(isatty(1) && isatty(2));
        assert(0 == daemon(0,iclose));
    }

    
    if(w1->pidfile)
    {
        FILE *fp;
        if(NULL != (fp = fopen(w1->pidfile,"w")))
        {
            fprintf(fp,"%d\n", getpid());
            fclose(fp);
        }
        free(w1->pidfile);
    }
    
    w1_all_couplers_off(w1);

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

        if(once)
            break;
        
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
                                        "Start : %ld %09ld\n"
                                        "Expect: %ld %09ld\n"
                                        "Actual: %ld %09ld\n",
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
