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

#ifndef _W1FILE_H
#define _W1FILE_H

#if defined(HAVE_CONFIG_H)
# include "config.h"
#else
# define VERSION "undef"
#endif

#include <stdarg.h>
#include <glib.h>
#include <gmodule.h>

enum W1_type {W1_INVALID, W1_TEMP, W1_HUMID, W1_PRES, W1_COUNTER, W1_BRAY,
              W1_SHT11, W1_COUPLER, W1_WINDVANE, W1_DS2438V, W1_HBBARO,
              W1_HIH, W1_DS2760, W1_DS2450, W1_MS_TC, W1_DS1921, W1_DS1923};

enum W1_so_opts {W1_SO_INIT=1, W1_SO_LOG=2};

/* coupler states: see SetSwitch1F() in swt1f.c */
#define COUPLER_UNDEFINED -1
#define COUPLER_ALL_OFF   0 // All lines off
#define COUPLER_MAIN_ON   4 // Smart-On Main
#define COUPLER_AUX_ON    2 // Smart-On Aux

#define W1_ROC (1 << 0)
#define W1_RMIN (1 << 1)
#define W1_RMAX (1 << 2)

#define ALLOCDEV 8
#define TBUF_SZ 32
#define MAXDLL 16
#define MAXCPL 64
#define ALLOCSENS 8
#define W1_NOP (-1)
#define W1_MIN_INTVL 10
#define W1_DEFAULT_INTVL 120

typedef struct w1_devlist w1_devlist_t;
typedef struct w1_device w1_device_t;

typedef struct
{
    int type;
    GModule *handle;
    void (*func)(w1_devlist_t *, void *);
    char *param;
} dlldef_t;

typedef struct
{
    char *abbrv;
    char *name;
    char *units;
    double value;
    short valid;
    short flags;
    double rmin;
    double rmax;
    double roc;
    double lval;
    time_t ltime;
} w1_sensor_t;

typedef struct
{
    int num;
    double values[];
} w1_params_t;

typedef struct
{
    w1_device_t *coupler_device;
    short branch;
} w1_coupler_t;

struct w1_device
{
    char *serial;
    char *devtype;
    short init;
    short ignore;
    enum W1_type stype;
    w1_sensor_t *s;
    unsigned char serno[8];
    w1_coupler_t *coupler;
    w1_params_t *params;
    void *private;
    int ns;
    int intvl;
};

typedef struct
{
  int active_lines;
} w1_coupler_private_t;

typedef struct
{
    unsigned char control[16];
} w1_windvane_private_t;

typedef struct 
{
    char devid[32];
    int branch;
    w1_device_t *coupler_device;
} w1_couplist_t;

struct w1_devlist
{
    int numdev;
    int ndll;
    int delay;
    int cycle;
    int portnum;
    int altitude;
    char *iface;
    char *rcfile;
    char *repfile;
    time_t logtime;
    dlldef_t dlls[MAXDLL];
    gboolean verbose;
    gboolean daemonise;
    gboolean logtmp;
    gboolean doread;
    w1_device_t *devs;
    char *lastmsg;
    int timestamp;
    int vane_offset;
    char *pidfile;
    char *tmpname;
    char log_delim[2];
    short log_timet;
    int temp_scan;
};

extern void w1_tmpfilelog (w1_devlist_t *);
extern void w1_freeup(w1_devlist_t *);
extern void w1_enumdevs(w1_device_t *);
extern void logtimes(time_t, char *);
extern char *w1_get_from_home(const char *);
extern void dll_parse(w1_devlist_t *, int, char *);
extern void read_config(w1_devlist_t *);
extern FILE * w1_file_open(char *);
extern w1_sensor_t * w1_find_sensor(w1_devlist_t *, const char *);
extern w1_sensor_t * w1_match_sensor(w1_device_t *, const char *);
extern void w1_replog(w1_devlist_t *, const char *,...);
extern void w1_set_device_data(w1_device_t *, const char *, char *);
extern void w1_set_device_data_index(w1_device_t *, int, char *);
extern void w1_all_couplers_off(w1_devlist_t *);
extern int w1_read_all_sensors(w1_devlist_t *, time_t);
extern void w1_initialize_couplers(w1_devlist_t *);
extern int w1_get_device_index(w1_device_t *, int, char *, char *);
extern void w1_verify_intervals (w1_devlist_t *);
#endif

