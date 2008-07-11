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

#define VALTPQRY "SELECT format_type(a.atttypid, a.atttypmod)" \
    " FROM pg_class c, pg_attribute a WHERE c.relname = 'readings'" \
    " AND a.attname = 'value' AND a.attrelid = c.oid"
#define SENSQRY "select distinct * from w1sensors"
#define RATEQRY "select name,value,rmin,rmax from ratelimit"

static char * valtype;

void  w1_init (w1_devlist_t *w1, char *dbnam)
{
    w1_device_t * devs = NULL;

    PGconn *idb;
    PGresult *res;
    int n = 0, ni = 0;
    
    idb = w1_opendb(dbnam);
    res = PQexec(idb, SENSQRY);

    if (res && PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        int nr = PQntuples(res);
        int nfields;
        int j;
        int nx = 0;
        int nn = 0;
        int id = -1;
        int it = -1;

        devs = malloc(sizeof(w1_device_t)*nr);
        memset(devs, 0, sizeof(w1_device_t)*nr);
        nfields = PQnfields(res);

        for(j = 0; j < nfields; j++)
        {
            char *fnam = PQfname(res, j);
            if(strcmp(fnam, "device") == 0)
            {
                id = j;
            }
            else if (strcmp(fnam, "type") == 0)
            {
                it = j;
            }
            if (it != -1 && id != -1)
                break;
        }

        for (n = 0; n < nr; n++)        
        {
            nn = w1_get_device_index(devs, ni, PQgetvalue(res, n, id),
                                     PQgetvalue(res, n, it));
             if (nn == -1)
             {
                 nx = ni;
                 ni++;
             }
             else
             {
                 nx = nn;
             }
            
            for(j = 0; j < nfields; j++)
            {
                char *fnam = PQfname(res, j);
                char *s = PQgetvalue(res, n, j);
                char *sv = (s && *s) ? strdup(s) : NULL;
                if(sv)
                    w1_set_device_data(devs+nx, fnam, sv);
            }
            w1_enumdevs(devs+nx);
        }
    }
    w1->numdev = ni;
    w1->devs=devs;
    if(res) PQclear(res);

    res = PQexec(idb,VALTPQRY);
    if (res && PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        char *s;
        s = PQgetvalue(res, 0, 0);
        if(s && *s)
        {
            if(valtype)
            {
                free(valtype);
            }
            valtype = strdup(s);
        }
    }

    if (res) PQclear(res);
    
    res = PQexec(idb, RATEQRY);
    if (res && PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        float roc=0,rmin=0,rmax=0;
        int nn = PQntuples(res);

        for (n = 0; n < nn; n++)        
        {
            char *s, *sv;
            short flags = 0;
            s = PQgetvalue(res, n, 0);
            if(s && *s)
            {
                sv = PQgetvalue(res, n, 1);
                if(sv && *sv)
                {
                    roc = strtod(sv, NULL);
                    flags |= W1_ROC;
                }

                sv = PQgetvalue(res, n, 2);
                if(sv && *sv)
                {
                    rmin = strtod(sv, NULL);
                    flags |= W1_RMIN;
                }

                sv = PQgetvalue(res, n, 3);
                if(sv && *sv)
                {
                    rmax = strtod(sv, NULL);
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
            if((qq = strchr(q,'\n')))
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
        syslog(LOG_ERR, "psql: %s", q);
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

void handle_result(PGresult *res)
{
    if(res){
        ExecStatusType status = PQresultStatus(res);
        if((status == PGRES_NONFATAL_ERROR) || (status == PGRES_FATAL_ERROR)){
            syslog(LOG_ERR, "psql: %s", PQresultErrorMessage(res));
        }
        PQclear(res);
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
#define STMTSTR "prepare insrt(%s,text,%s) as insert into readings (date,name,value) values ($1,$2,$3)"

#warning "Pg Version 7"
        char stmtstr[256];
        char *tstr = (w1->timestamp) ? "timestamp with time zone" : "integer";
        snprintf(stmtstr, sizeof(stmtsrr),STMTSTR,tstr,valtype);
        res = PQexec(db, stmtstr);
#elif PGV == 8
        res = PQprepare(db, stmt,
                        "insert into readings (date,name,value) values ($1,$2,$3)", 0, NULL);
#else
#error "Bad PG version"        
#endif
        if(res) PQclear(res);        
    }
    
    res = PQexec(db,"begin");
    handle_result(res);
    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++) // for each device
    {
        if(devs->init) // if the device is initialised
        {
            int j;
            for (j = 0; j < devs->ns; j++) // for each sensor
            {
                if(devs->s[j].valid) // if there's a valid reading
                {
                    char *rval;
                    char tval[64];
                    int n;
                    
                    if(devs->stype == W1_COUNTER || devs->stype == W1_WINDVANE)
                        n=asprintf(&rval, "%.0f", devs->s[j].value); // do not include any decimal places for integer values
                    else
                        n=asprintf(&rval, "%f", devs->s[j].value);   // include default 6 decimal places

                    if(w1->timestamp){
                        struct tm *tm;
                        tm = localtime(&w1->logtime);
                        strftime(tval, sizeof(tval), "%F %T%z", tm);
                    }else{
                        snprintf(tval, sizeof(tval), "%ld", w1->logtime);
                    }

                    if(devs->s[j].abbrv[0] == '>'){
                        // store sensor value in a separate named table
                        char *query;
                        n=asprintf(&query, "INSERT INTO %s (date, value) VALUES ('%s', '%s')", &devs->s[j].abbrv[1], tval, rval);
                        res = PQexec(db, query);
                        handle_result(res);
                        free(query);
                    }else{
                        // store sensor value in the 'readings' table
                        const char * pvals[3];
                    pvals[0] = tval;
                    pvals[1] = devs->s[j].abbrv;
                    pvals[2] = rval;
                    res = PQexecPrepared(db, stmt, 3, pvals, NULL, NULL, 0);
                        handle_result(res);
                    }

                    free(rval);
                }
            }
        }
    }
    res = PQexec(db,"commit");
    handle_result(res);
}

void w1_report(w1_devlist_t *w1, char *dbnam)
{
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
                            "insert into replog(date,message) values ($1,$2)", 0, NULL);
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
    /*
    w1->logtime = 0;
    w1->devs[0].s[0].valid = 1;
    w1->devs[0].init = 1;
    w1->devs[0].s[0].value = 22.22;
    w1_logger(w1, auth);    
*/
     return 0;
}
#endif
