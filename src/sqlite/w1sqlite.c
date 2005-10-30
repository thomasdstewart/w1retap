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
#include "w1retap.h"

static sqlite3 * w1_opendb(char *dbname)
{
    sqlite3 *mydb = NULL;
    if(sqlite3_open(dbname, &mydb) == SQLITE_OK)
    {
        sqlite3_busy_timeout(mydb,5000);
    }
    else
    {
        perror(sqlite3_errmsg(mydb));
    }
    return mydb;
}

#define GETVALUE(__p0,__p1) \
    if(nc > __p0) __p1 =  (rt[offset+__p0]) ? strdup(rt[offset+__p0]) : NULL

void  w1_init (w1_devlist_t *w1, char *dbnam)
{
    w1_device_t * devs = NULL;
    char *sql = "select device,type,abbrv1,name1,units1,abbrv2,name2,units2 from w1sensors";
    sqlite3 *db;
    char *err;
    int n = 0, nr = 0, nc;
    char **rt;

    db = w1_opendb(dbnam);
    if(sqlite3_get_table (db, sql, &rt, &nr, &nc, &err) == SQLITE_OK)
    {
        if(nr > 0 && nc > 0)
        {
            devs = malloc(sizeof(w1_device_t)*nr);
            memset(devs, 0, sizeof(w1_device_t)*nr);

            int offset = 0;
            for(n = 0; n < nr; n++)
            {
                offset += nc;
                GETVALUE(0, devs[n].serial);
                GETVALUE(1, devs[n].devtype);
                GETVALUE(2, devs[n].s[0].abbrv);
                GETVALUE(3, devs[n].s[0].name);
                GETVALUE(4, devs[n].s[0].units);
                GETVALUE(5, devs[n].s[1].abbrv);
                GETVALUE(6, devs[n].s[1].name);
                GETVALUE(7, devs[n].s[1].units);                
                w1_enumdevs(devs+n);
            }
            sqlite3_free_table(rt);
        }
    }
    else
    {
        if(err)
        {
            fprintf(stderr, "ERR %s\n", err);
            sqlite3_free(err);
        }
    }

    w1->numdev = n;
    w1->devs=devs;

    if(sqlite3_get_table (db, "select name,value,rmin,rmax from ratelimit",
                          &rt, &nr, &nc, &err) == SQLITE_OK)
    {
        float roc,rmin,rmax;

        if(nr > 0 && nc > 0)
        {
            int offset = 0;
            for(n = 0; n < nr; n++)
            {
                char *s = NULL,*sv=NULL;
                short flags = 0;                
                offset += nc;
                GETVALUE(0, s);
                if(s && *s)
                {
                    GETVALUE(1, sv);
                    if(sv)
                    {
                        if(*sv)
                        {
                            roc = strtof(sv, NULL);
                            flags |= W1_ROC;
                        }
                        if (sv) free(sv);                        
                    }
                    GETVALUE(2, sv);
                    if(sv)
                    {
                        if(*sv)
                        {
                            rmin = strtof(sv, NULL);
                            flags |= W1_RMIN;
                        }
                        if (sv) free(sv);                        
                    }
                    GETVALUE(3, sv);
                    if(sv)
                    {
                        if(*sv)
                        {
                            rmax = strtof(sv, NULL);
                            flags |= W1_RMAX;
                        }
                        if (sv) free(sv);                        
                    }
                    if(flags)
                    {
                        w1_sensor_t *sensor;
                        if (NULL != (sensor = w1_find_sensor(w1, (const char *)s)))
                        {
                            sensor->flags = flags;
                            if(flags & W1_ROC)
                                sensor->roc = roc;
                            if(flags & W1_RMIN)
                                sensor->rmin = rmin;
                            if(flags & W1_RMAX)
                                sensor->rmax = rmax;
                        }
                    }
                }
                if (s) free(s);
            }
            sqlite3_free_table(rt);
        }
    }
    else
    {
        if(err)
        {
            sqlite3_free(err);
        }
    }
    
    sqlite3_close(db);
}

static sqlite3 *db;
static sqlite3_stmt *stmt;

void w1_cleanup(void)
{
    if(db)
    {
        if(stmt)
        {
            sqlite3_finalize(stmt);
            stmt = NULL;
        }
        sqlite3_close(db);
        db = NULL;
    }
}

void w1_logger(w1_devlist_t *w1, char *dbnam)
{
    int i;
    w1_device_t *devs;

    if (access("/tmp/.w1retap.lock", F_OK) == 0)
    {
        return;
    }
    
    if(db == NULL)
    {
        db = w1_opendb(dbnam);
    }

    if(stmt == NULL)
    {
        const char *tl;
        static char s[] = "insert into readings values (?,?,?)";    
	sqlite3_prepare(db, s, sizeof(s)-1, &stmt, &tl);
    }

    sqlite3_exec(db,"begin", NULL, NULL,NULL);
    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < 2; j++)
            {
                if(devs->s[j].valid)
                {
                    sqlite3_bind_int(stmt, 1, w1->logtime);
                    sqlite3_bind_text(stmt, 2, devs->s[j].abbrv, -1, SQLITE_STATIC);
                    sqlite3_bind_double(stmt, 3, (double)devs->s[j].value);
                    sqlite3_step(stmt);
                    sqlite3_reset(stmt);
                }
            }
        }
    }
    sqlite3_exec(db,"commit", NULL, NULL,NULL);
}


