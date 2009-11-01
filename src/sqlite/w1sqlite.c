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
    char *sql = "select distinct * from w1sensors";
        //device,type,abbrv1,name1,units1,abbrv2,name2,units2
    sqlite3 *db;
    char *err;
    int n = 0, nr = 0, nc;
    char **rt;
    int nx = 0, ni = 0;
    
    db = w1_opendb(dbnam);
    if(sqlite3_get_table (db, sql, &rt, &nr, &nc, &err) == SQLITE_OK)
    {
        if(nr > 0 && nc > 0)
        {
            int k;
            int nn = 0;
            int id = -1;
            int it = -1;

            devs = malloc(sizeof(w1_device_t)*nr);
            memset(devs, 0, sizeof(w1_device_t)*nr);

            for(k = 0; k < nc; k++)
            {
                if(strcmp(rt[k],"device") == 0)
                {
                    id = k;
                }
                else if (strcmp(rt[k], "type") == 0)
                {
                    it = k;
                }
                if (it != -1 && id != -1)
                    break;
            }

            for(n = 0; n < nr; n++)
            {
                nn = w1_get_device_index(devs, ni, rt[(n+1)*nc + id],
                                         rt[(n+1)*nc + it]);
//            fprintf(stderr, "Search for %s %s %d\n", rt[(n+1)*nc + id],rt[(n+1)*nc + it]  ,nn);

            if (nn == -1)
                {
                    nx = ni;
                    ni++;
                }
                else
                {
                    nx = nn;
                }
//            fprintf(stderr, "Device %d\n", nx);
            
                for(k = 0; k < nc; k++)
                {
                    char *fnam = rt[k];
                    char *s = rt[(n+1)*nc + k];
                    char *sv = (s && *s) ? strdup(s) : NULL;
//                    fprintf(stderr,"set entry %d:%d %s %s\n", k, (devs+nx)->ns, fnam,sv);
                    if (fnam && sv)
                        w1_set_device_data(devs+nx, fnam, sv);
                }
                w1_enumdevs(devs+nx);
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

    w1->numdev = ni;
    w1->devs=devs;
//    fprintf(stderr, "read = %d, found = %d\n", ni, nr);
    
    if(sqlite3_get_table (db, "select name,value,rmin,rmax from ratelimit",
                          &rt, &nr, &nc, &err) == SQLITE_OK)
    {
        float roc=0,rmin=0,rmax=0;

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
static sqlite3_stmt *stml;

void w1_cleanup(void)
{
    if(db)
    {
        if(stmt)
        {
            sqlite3_finalize(stmt);
            stmt = NULL;
        }
        if(stml)
        {
            sqlite3_finalize(stml);
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
        static char s[] = "insert into readings(date,name,value) values (?,?,?)";    
	sqlite3_prepare(db, s, sizeof(s)-1, &stmt, &tl);
    }

    sqlite3_exec(db,"begin", NULL, NULL,NULL);
    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < devs->ns; j++)
            {
                if(devs->s[j].valid)
                {
                    if(w1->timestamp)
                    {
                        struct tm *tm;
                        char tval[64];
                        tm = localtime(&w1->logtime);
                        strftime(tval, sizeof(tval), "%F %T%z", tm);
                        sqlite3_bind_text(stmt, 1, tval, -1, SQLITE_STATIC);
                    }
                    else
                    {
                        sqlite3_bind_int(stmt, 1, w1->logtime);
                    }
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

void w1_report(w1_devlist_t *w1, char *dbnam)
{
    if(w1->lastmsg)
    {
        if(db == NULL)
        {
            db = w1_opendb(dbnam);
        }
        if(stml == NULL)
        {
            const char *tl;
            static char s[] = "insert into replog(date,message) values (?,?)";
            sqlite3_prepare(db, s, sizeof(s)-1, &stml, &tl);
        }
        sqlite3_exec(db,"begin", NULL, NULL,NULL);
        {
            struct tm *tm;
            char tval[64];
            tm = localtime(&w1->logtime);
            strftime(tval, sizeof(tval), "%F %T%z", tm);
            sqlite3_bind_text(stml, 1, tval, -1, SQLITE_STATIC);
        }
        sqlite3_bind_text(stml, 2, w1->lastmsg, -1, SQLITE_STATIC);
        sqlite3_step(stml);
        sqlite3_reset(stml);
        sqlite3_exec(db,"commit", NULL, NULL,NULL);
    }
}
