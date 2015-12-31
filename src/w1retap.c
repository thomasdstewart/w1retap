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
#include <errno.h>
#include "ownet.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "w1retap.h"

enum W1_sigflag
{
    W1_NOFLAG =0,
    W1_RECONF = (1),
    W1_READALL = (2),
    W1_SHOWCF = (4)
};


static volatile enum W1_sigflag sigme;

#ifndef HAVE_CLOCK_NANOSLEEP
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
#endif

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
                logtimes(w1,w1->logtime, s);
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
                fputs("replogger entry point not found\n", stderr);
                exit(1);
            }
        }
    }

    if (done_i == 0)
    {
        fputs("Init fails: No w1_init() in init= loadable library\n"
              "Please check the init= lines in your configuration file\n"
              "and the manual entry 'Configuring the W1RETAP software'.\n"
              "This is typically a configuration or build error.\n\n"
              "You should also check that the required data driver is\n"
              "present under $prefix/lib/w1retap/, for example, for sqlite,\n"
              "a set of libraries libw1sqlite.so{,.0,.0.0.0} should be present.\n", stderr);
        exit(1);
    }
    w1_verify_intervals (w1);
    w1_initialize_couplers(w1);
}

void init_interfaces(w1_devlist_t *w1)
{
    if(w1->iface == NULL)
        w1->iface = strdup("DS2490-1");
    if(w1->portnum == -1)
    {
        if((w1->portnum = owAcquireEx(w1->iface)) < 0)
        {
            OWERROR_DUMP(stdout);
            exit(1);
        }
    }
//    w1_initialize_couplers(w1);
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

            fprintf(stderr, "%s %s (%ds)%s\n",
                    w1->devs[i].serial, w1->devs[i].devtype, w1->devs[i].intvl,
                    (w1->devs[i].stype == W1_INVALID) ? " !" : "");

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
        fprintf(stderr,"Interval %ds, cycle %ds\n", w1->delay, w1->cycle);
        fprintf(stderr,"Release i/face %d\n", w1->release_me);
        if (w1->pres_reduction_temp)
        {
            fprintf(stderr,"Pressure reduction temp %.2f\n", *w1->pres_reduction_temp);
        }
        if(w1->allow_escape)
        {
            fputs ("Allowing rate limit escapes\n", stderr);
        }
        if(w1->force_utc)
        {
            fputs ("dates logged at UTC\n", stderr);
        }

    }
}

int main(int argc, char **argv)
{
    struct timeval now = {0};
    struct sigaction act ={{0}};
    gboolean immed = 0;
    gboolean showvers=0;
    gboolean once=0;
    gboolean w1_simul = 0;
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
        {"release-interface",'R', 0, G_OPTION_ARG_NONE, &w1->release_me,
         "Release the 1Wire interface between reads",NULL},
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
        {"simulate",'s', 0, G_OPTION_ARG_NONE, &w1_simul,
        "Simulate readings (for testing, not yet implemented)", NULL},
        {"use-utc",'u', 0, G_OPTION_ARG_NONE, &w1->force_utc,
        "Store dates as UTC (vice localtime)", NULL},
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
    w1->log_delim[0] = ' ';
    w1->delay = w1->cycle = W1_DEFAULT_INTVL;
    w1->temp_scan = 1000;
    w1->pres_reduction_temp = NULL;

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

    immed ^= 1; // weird JH logic ...

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
        fputs("w1retap v" VERSION " (c) 2005-2015 Jonathan Hudson\n", stderr);
        fputs("Built: " __DATE__ " " __TIME__  " gcc " __VERSION__ "\n", stderr);
        if(w1->verbose == 2)
        {
            exit (0);
        }
    }

    do_init(w1);
    w1_show(w1, 0);

    if(!w1->doread)
    {
        exit(0);
    }

    init_interfaces(w1);
    w1->logtime =time(NULL);
    w1_replog (w1, "Startup w1retap v" VERSION);

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
        if (immed)
        {
            if(w1->verbose)
                fputs("reading ...\n", stderr);
            nv = w1_read_all_sensors(w1, now.tv_sec);
            if(nv)
            {
                for(n = 0; n < w1->ndll; n++)
                {
                    if (w1->dlls[n].type == 'l' && w1->dlls[n].func)
                    {
                        (w1->dlls[n].func)(w1, w1->dlls[n].param);
                    }
                }

                if(w1->logtmp && ((now.tv_sec % w1->cycle) == 0))
                {
                    w1_tmpfilelog (w1);
                }
            }
            if(w1->release_me)
            {
                owRelease(w1->portnum);
                w1->portnum = -1;
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
                struct timespec req;
                ns = 0;
                if(sigme & W1_READALL)
                {
                    sigme &= ~W1_READALL;
                    nv = w1_read_all_sensors(w1, 0);
                    w1_tmpfilelog(w1);
                }

                if(sigme & W1_RECONF)
                {
                    sigme &= ~W1_RECONF;
                    cleanup (0, w1);
                    read_config(w1);
                    do_init(w1);
                    w1_show(w1, 0);
                    init_interfaces(w1);
                }

                if(sigme & W1_SHOWCF)
                {
                    sigme &= ~W1_SHOWCF;
                    w1_show(w1, 1);
                }

                if(w1->verbose)
                    fputs("Waiting ... ", stderr);
		gettimeofday(&now, NULL);
#ifdef HAVE_CLOCK_NANOSLEEP
                req.tv_sec = (now.tv_sec / w1->delay)*w1->delay + w1->delay;
                req.tv_nsec = 0;
                ns  = clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME, &req, NULL);
#else
		struct timespec then, nnow;
                then.tv_sec = w1->delay * (1 + now.tv_sec / w1->delay);
                then.tv_nsec = 200*1000*1000; /* ensure tick crossed */
                nnow.tv_sec = now.tv_sec;
                nnow.tv_nsec = now.tv_usec * 1000;
                nanosub(&then, &nnow, &req);
                ns  = nanosleep(&req, NULL);
#endif
                if(ns == 0)
                {
                    gettimeofday(&now, NULL);
                    w1->logtime = now.tv_sec;
                    if(w1->verbose)
                        fputs(ctime(&now.tv_sec),stderr);
                }
            } while ((ns == EINTR) || (ns ==  -1 && errno == EINTR));
        }
        else
        {
            break;
        }
    }
    return 0;
}
