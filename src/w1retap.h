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

#include <gmodule.h>

enum W1_type {W1_INVALID, W1_TEMP, W1_HUMID, W1_PRES, W1_RAIN};
enum W1_so_opts {W1_SO_INIT=1, W1_SO_LOG=2};

#define ALLOCDEV 8
#define TBUF_SZ 32
#define MAXDLL 16


typedef struct w1_devlist w1_devlist_t;

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
    float value;
    short valid;
} w1_sensor_t;

typedef struct
{
    char *serial;
    char *devtype;
    short init;
    enum W1_type stype;
    w1_sensor_t s[2];
} w1_device_t;

struct w1_devlist
{
    int numdev;
    int ndll;
    int delay;
    int portnum;    
    char *iface;
    char *rcfile;    
    time_t logtime;
    dlldef_t dlls[MAXDLL];
    short verbose;
    short daemonise;
    short logtmp;
    short doread;
    w1_device_t *devs;
};

extern void w1_tmpfilelog (w1_devlist_t *);
extern void w1_freeup(w1_devlist_t *);
extern void w1_enumdevs(w1_device_t *);
extern void logtimes(time_t, char *);
extern char *w1_get_from_home(const char *);
extern void dll_parse(w1_devlist_t *, int, char *);
extern void read_config(w1_devlist_t *);
extern FILE * w1_file_open(char *);
#endif

