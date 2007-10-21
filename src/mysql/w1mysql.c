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
#include <mysql.h>
#include "w1retap.h"

// If you know how to make this stmt/prepare work, the fix this
// to a suitable version (and fix timestamp stuff too).

#define MINVERS 99999

static void my_params(char *params,
                      char **host, char **dbname, char **user, char **pass)
{
    char *mp = strdup(params);
    char *s0,*s1;

    *host = *dbname = *user = *pass = NULL;

    for(s0 = mp; (s1=strsep(&s0, " "));)
    {
        char *t,*v;
        sscanf(s1,"%a[^=]=%as", &t, &v);

        if(strcmp(t,"dbname") == 0)
        {
            *dbname = v;
        }
        else if (strcmp(t,"host") == 0)
        {
            *host = v;
        }
        else if (strcmp(t,"user") == 0)
        {
            *user = v;
        }
        else if (strcmp(t,"password") == 0)
        {
            *pass = v;
        }
        else
        {
            free(v);
        }
        free(t);
    }
    free(mp);
}

static MYSQL * w1_opendb(char *params)
{
    MYSQL *conn = NULL;
    char *dbname, *user, *password, *host;

    my_params(params, &host, &dbname, &user, &password);

    conn = mysql_init(NULL);
    
    if (0 == mysql_real_connect(conn, host, user, password,
                                dbname, 0, NULL, 0))
    {
        perror(mysql_error(conn));
        conn = NULL;
    }
    return conn;
}

void  w1_init (w1_devlist_t *w1, char *dbnam)
{
    w1_device_t * devs = NULL;
    char *sql = "select * from w1sensors";
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *field;
    int n = 0;

    conn = w1_opendb(dbnam);
    if (!mysql_query(conn, sql))
    {
        res = mysql_store_result(conn);        
        int nn = mysql_num_rows(res);
        
        devs = malloc(sizeof(w1_device_t)*nn);
        memset(devs, 0, sizeof(w1_device_t)*nn);
        
        for (n = 0; n < nn; n++)        
        {
            int j;
            row = mysql_fetch_row(res);
            int nf = mysql_num_fields(res);            
            for(j = 0; j < nf; j++)
            {
                char *s = row[j];
                char *sv = (s && *s) ? strdup(s) : NULL;
                field = mysql_fetch_field_direct(res, j);
                char *fnam = field->name;
                w1_set_device_data(devs+n, fnam, sv);
            }
            w1_enumdevs(devs+n);
        }
        w1->numdev = n;
        w1->devs=devs;
        mysql_free_result(res); 

        if (!mysql_query(conn, "select name,value,rmin,rmax from ratelimit"))
        {
            res = mysql_store_result(conn);        
            int nn = mysql_num_rows(res);
            for (n = 0; n < nn; n++)        
            {
                row = mysql_fetch_row(res);
                char *s = row[0];
                short flags = 0;
                float roc=0,rmin=0,rmax=0;

                if(s && *s)
                {
                    char *sv = row[1];
                    if(sv && *sv)
                    {
                        roc = strtof(sv, NULL);
                        flags |= W1_ROC;
                    }
                    sv = row[2];
                    if(sv && *sv)
                    {
                        rmin = strtof(sv, NULL);
                        flags |= W1_RMIN;
                    }
                    sv = row[3];
                    if(sv && *sv)
                    {
                        rmax = strtof(sv, NULL);
                        flags |= W1_RMAX;
                    }
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
            mysql_free_result(res); 
        }
        mysql_close(conn);
    }
}

static MYSQL *conn;
#if MYSQL_VERSION_ID > MINVERS
static MYSQL_STMT *stmt;
#endif

void w1_cleanup(void)
{
    if(conn)
    {
#if MYSQL_VERSION_ID > MINVERS        
        mysql_stmt_close(stmt);
        stmt = NULL;
#endif
        mysql_close(conn);
        conn = NULL;
    }
}

void w1_logger(w1_devlist_t *w1, char *params)
{
    int i;
    w1_device_t *devs;
    
    if (access("/tmp/.w1retap.lock", F_OK) == 0)
    {
        return;
    }
    
    if(conn == NULL)
    {
        conn = w1_opendb(params);
#if MYSQL_VERSION_ID > MINVERS
        mysql_autocommit(conn, 0);
#else
        mysql_real_query(conn, "SET AUTOCOMMIT=0",
                         sizeof("SET AUTOCOMMIT=0")-1);        
#endif
    }
#if MYSQL_VERSION_ID > MINVERS
    if(stmt == NULL)
    {
        const char s[] =
            "insert into readings(date,name,value) values (?,?,?)";
        stmt = mysql_stmt_init(conn);
        mysql_stmt_prepare(stmt, s, sizeof(s) - 1 );
    }
#else
    mysql_real_query(conn, "BEGIN", sizeof("BEGIN")-1);        
#endif
    
    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
#if MYSQL_VERSION_ID > MINVERS
            MYSQL_BIND   bind[3];
            my_bool is_null = 0;
            unsigned int length[3];
#endif
            for (j = 0; j < 2; j++)
            {

                if(devs->s[j].valid)
                {
#if MYSQL_VERSION_ID > MINVERS
                    bind[0].buffer_type= MYSQL_TYPE_LONG;
                    bind[0].buffer= (char *)&w1->logtime;
                    bind[1].buffer_length= length[0] = sizeof(int);
                    bind[0].is_null= &is_null;
                    bind[0].length= length;

                    bind[1].buffer_type= MYSQL_TYPE_VAR_STRING;
                    bind[1].buffer= devs->s[j].abbrv;
                    bind[1].buffer_length=
                        length[1] = strlen(devs->s[j].abbrv);
                    bind[1].is_null= &is_null;
                    bind[1].length= length+1;

                    bind[2].buffer_type= MYSQL_TYPE_FLOAT;
                    bind[2].buffer= (char *)&devs->s[j].value;
                    bind[2].buffer_length= length[2] = sizeof(float);
                    bind[2].is_null= &is_null;
                    bind[2].length= length + 2;

                    if(mysql_stmt_bind_param(stmt, bind))
                    {
                        fputs("bad bind\n", stderr);
                    }
                    
                    if(mysql_stmt_execute(stmt))
                    {
                        fprintf(stderr, "execute:  %s\n", mysql_error(conn));
                    }
                    
#else
                    char *q;
                    char tval[64];

                    if(w1->timestamp)
                    {
                        struct tm *tm;
                        tm = localtime(&w1->logtime);
                        strftime(tval, sizeof(tval), "'%F %T%z'", tm);
                    }
                    else
                    {
                        snprintf(tval, sizeof(tval), "%ld", w1->logtime);
                    }
                    asprintf(&q,
                             "INSERT into readings(date,name,value) VALUES(%s,'%s',%g)",
                             tval, devs->s[j].abbrv, devs->s[j].value);
		    if(w1->verbose) puts(q);
                    mysql_real_query(conn, q, strlen(q));
                    free(q);
#endif
                }
            }
        }
    }
#if MYSQL_VERSION_ID > MINVERS
    mysql_commit(conn);
#else
    mysql_real_query(conn, "COMMIT", sizeof("COMMIT")-1);            
#endif
}


#if defined(TESTBIN)
int main(int argc, char **argv)
{
    int n;
    w1_devlist_t w = {0},*w1;
    w1=&w;

    if(argc < 2)
    {
        fputs("Need dbparams as argv[1] please\n", stderr);
        exit(1);
    }
    
    w1_init(w1, argv[1]);
    for(n = 0; n < w1->numdev; n++)
    {
        fprintf(stderr, "%s %s\n",
                w1->devs[n].serial, w1->devs[n].devtype);
        fprintf(stderr, "\t0: %s %s\n",
                w1->devs[n].s[0].abbrv, w1->devs[n].s[0].name);
        fprintf(stderr, "\t1: %s %s\n",
                w1->devs[n].s[1].abbrv, w1->devs[n].s[1].name);
    }


    w1->logtime = time(0);
    w1->devs[0].init = 1;
    w1->devs[0].s[0].valid = 1;
    w1->devs[0].s[0].value = 22.22;

    w1->devs[1].init = 1;
    w1->devs[1].s[0].valid = 1;
    w1->devs[1].s[0].value = 69;
    w1->devs[1].s[1].valid = 99.0;
    w1->devs[1].s[1].value = 18.88;

    w1->devs[2].init = 1;
    w1->devs[2].s[0].valid = 1;
    w1->devs[2].s[0].value = 1001.45;
    w1_logger(w1, argv[1]);

    sleep(5);
    w1->logtime = time(0);
    w1->devs[0].init = 1;
    w1->devs[0].s[0].valid = 1;
    w1->devs[0].s[0].value = 25.77;

    w1->devs[1].init = 1;
    w1->devs[1].s[0].valid = 1;
    w1->devs[1].s[0].value = 66;
    w1->devs[1].s[1].valid = 1;
    w1->devs[1].s[1].value = 12.565;

    w1->devs[2].init = 0;

    w1_logger(w1, argv[1]);
    return 0;
}
#endif

