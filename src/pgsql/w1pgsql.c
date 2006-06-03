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
#include <syslog.h>

#include "libpq-fe.h"
#include "w1retap.h"

static PGconn * w1_opendb(char *params)
{
    PGconn *mydb = NULL;
    mydb = PQconnectdb(params);
    if (PQstatus(mydb) != CONNECTION_OK)
    {
        PQfinish(mydb);
        mydb = NULL;
    }
    return mydb;
}

void  w1_init (w1_devlist_t *w1, char *dbnam)
{
    w1_device_t * devs = NULL;
    char *sql = "select device,type,abbrv1,name1,units1,abbrv2,name2,units2 from w1sensors";
    PGconn *idb;
    PGresult *res;
    int n = 0;

    idb = w1_opendb(dbnam);
    res = PQexec(idb, sql);

    if (res && PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        int nn = PQntuples(res);
        devs = malloc(sizeof(w1_device_t)*nn);
        memset(devs, 0, sizeof(w1_device_t)*nn);
        
        for (n = 0; n < nn; n++)        
        {
            int j;
            for(j = 0; j < 8; j++)
            {
                char *s = PQgetvalue(res, n, j);
                char *sv = (s && *s) ? strdup(s) : NULL;
                switch (j)
                {
                    case 0:
                        devs[n].serial = sv;
                        break;
                    case 1:
                        devs[n].devtype = sv;
                        break;
                    case 2:
                        devs[n].s[0].abbrv = sv;
                        break;
                    case 3:
                        devs[n].s[0].name = sv;
                        break;
                    case 4:
                        devs[n].s[0].units = sv;
                        break;
                    case 5:
                        devs[n].s[1].abbrv = sv;
                        break;
                    case 6:
                        devs[n].s[1].name = sv;
                        break;
                    case 7:
                        devs[n].s[1].units = sv;
                        break;
                }
            }
            w1_enumdevs(devs+n);
        }
    }
    w1->numdev = n;
    w1->devs=devs;
    if(res) PQclear(res);
    
    res = PQexec(idb, "select name,value,rmin,rmax from ratelimit");
    if (res && PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        float roc,rmin,rmax;
        int nn = PQntuples(res);

        for (n = 0; n < nn; n++)        
        {
            char *s, *sv;
            int i;
            short flags = 0;
            s = PQgetvalue(res, n, 0);
            if(s && *s)
            {
                sv = PQgetvalue(res, n, 1);
                if(sv && *sv)
                {
                    roc = strtof(sv, NULL);
                    flags |= W1_ROC;
                }

                sv = PQgetvalue(res, n, 2);
                if(sv && *sv)
                {
                    rmin = strtof(sv, NULL);
                    flags |= W1_RMIN;
                }

                sv = PQgetvalue(res, n, 3);
                if(sv && *sv)
                {
                    rmax = strtof(sv, NULL);
                    flags |= W1_RMAX;
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
        }
    }
    if (res) PQclear(res);
    PQfinish(idb);
}

static PGconn *db;
static char *stmt;
static char *stml;
static int retry;

static void db_syslog(char *p)
{
    char *q = NULL,*qq;
    int qa = 0;
    
    if (p)
    {
        if ((q = strdup(p)))
        {
            qa = 1;
            if(qq = strchr(q,'\n'))
            {
                *qq = 0;
            }
        }
    }
    
    if (q == NULL)
    {
        q = "retry";
    }
    
    if(q)
    {
        openlog("w1retap", LOG_PID, LOG_USER);
        syslog(LOG_ERR, "psql: %s", q);
        closelog();
    }
    if(qa) free(q);
}


static void db_status(char *dbnam)
{
    if(db == NULL)
    {
        db = w1_opendb(dbnam);
    }
    else if (PQstatus(db) == CONNECTION_BAD)
    {
        PQreset(db);
        stmt = NULL;
        stml = NULL;
        retry++;
    }
    else
    {
        retry = 0;
    }

    if ((retry % 10) == 1)
    {
        db_syslog(PQerrorMessage(db));
    }
}


void w1_cleanup(void)
{
    if(db)
    {
        stmt = NULL;
        stml = NULL;
        PQfinish(db);
        db = NULL;
        retry = 0;
    }
}

void w1_logger(w1_devlist_t *w1, char *dbnam)
{
    int i;
    w1_device_t *devs;
    PGresult *res = NULL;
    
    if (access("/tmp/.w1retap.lock", F_OK) == 0)
    {
        return;
    }
    
    db_status(dbnam);
    
    if(stmt == NULL)
    {
        stmt = "insrt";

#if PGV == 7
#warning "Pg Version 7"
        res = PQexec(db, "prepare insrt(integer,text,real) as "
               "insert into readings values ($1,$2,$3)");
#elif PGV == 8
        res = PQprepare(db, stmt,
                        "insert into readings values ($1,$2,$3)", 0, NULL);
#else
#error "Bad PG version"        
#endif
        if(res) PQclear(res);        
    }
    
    res = PQexec(db,"begin");
    if(res) PQclear(res);
    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < 2; j++)
            {
                if(devs->s[j].valid)
                {
                    char tval[16];
                    char *rval;
                    const char * pvals[3];
                    
                    snprintf(tval, sizeof(tval), "%ld", w1->logtime);
                    asprintf(&rval, "%g", devs->s[j].value);
                    pvals[0] = tval;
                    pvals[1] = devs->s[j].abbrv;
                    pvals[2] = rval;
                    res = PQexecPrepared(db, stmt, 3, pvals, NULL, NULL, 0);
                    if(res) PQclear(res);                    
                    free(rval);
                }
            }
        }
    }
    res = PQexec(db,"commit");
    if(res) PQclear(res);    
}

void w1_report(w1_devlist_t *w1, char *dbnam)
{
    int i;
    w1_device_t *devs;
    PGresult *res;

    if(w1->lastmsg)
    {
        db_status(dbnam);
        if(stml == NULL)
        {
            stml = "insrl";

#if PGV == 7
#warning "Pg Version 7"
            res = PQexec(db, "prepare insrl(timestamp with time zone,text) as "
                   "insert into replog values ($1,$2)");
#elif PGV == 8
            res = PQprepare(db, stml,
                            "insert into replog values ($1,$2)", 0, NULL);
#else
#error "Bad PG version"        
#endif
            if(res) PQclear(res);                        
        }

        res = PQexec(db,"begin");
        if(res) PQclear(res);
        {
            const char * pvals[2];
            char tstr[64];
            logtimes(w1->logtime, tstr);
            pvals[0] = tstr;
            pvals[1] = w1->lastmsg;
            res = PQexecPrepared(db, stml, 2, pvals, NULL, NULL, 0);
            if (res) PQclear(res);            
        }
        res = PQexec(db,"commit");
        if(res) PQclear(res);        
    }
}

#if defined(TESTBIN)
int main(int argc, char **argv)
{
    char *auth = "dbname=w1retap user=postgres";
    w1_devlist_t w = {0},*w1;
    
    if(argc == 2)
    {
        auth = argv[1];
    }

    w1=&w;
    w1_init(w1, auth);
    w1->logtime = 0;
    w1->devs[0].s[0].valid = 1;
    w1->devs[0].init = 1;
    w1->devs[0].s[0].value = 22.22;
    w1_logger(w1, auth);    
    return 0;
}
#endif
