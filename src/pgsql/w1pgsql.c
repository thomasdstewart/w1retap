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
    PGconn *db;
    PGresult *res;
    int n = 0;

    db = w1_opendb(dbnam);
    res = PQexec(db, sql);

    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        int nn = PQntuples(res);
        devs = malloc(sizeof(w1_device_t)*nn);
        memset(devs, 0, sizeof(w1_device_t)*nn);
        
        for (n = 0; n < nn; n++)        
        {
            int j;
            
            for(j = 0; j < 6; j++)
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
    PQclear(res);
    PQfinish(db);
}

static PGconn *db;
static char *stmt;

void w1_cleanup(void)
{
    if(db)
    {
        stmt = NULL;
        PQfinish(db);
        db = NULL;
    }
}

void w1_logger(w1_devlist_t *w1, char *dbnam)
{
    int i;
    w1_device_t *devs;
    PGresult *res;
    
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
        stmt = "insrt";

#if PGV == 7
#warning "Pg Version 7"
        PQexec(db, "prepare insrt(integer,text,real) as "
               "insert into readings values ($1,$2,$3)");
#elif PGV == 8
        res = PQprepare(db, stmt,
                        "insert into readings values ($1,$2,$3)", 0, NULL);
#else
#error "Bad PG version"        
#endif
    }

    res = PQexec(db,"begin");
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
                    free(rval);
                }
            }
        }
    }
    res = PQexec(db,"commit");
}

#if defined(TESTBIN)
int main()
{
    w1_devlist_t w = {0},*w1;
    w1=&w;
    w1_init(w1, "dbname=w1retap user=postgres");
    w1->logtime = time(0);
    w1->devs[0].s[0].valid = 1;
    w1->devs[0].init = 1;
    w1->devs[0].s[0].value = 22.22;
    w1_logger(w1, "dbname=w1retap user=postgres");    
    return 0;
}
#endif
