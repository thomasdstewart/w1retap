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
#include <syslog.h>
#include "w1retap.h"


// This forces the bad old way of doing MYSQL inserts, as prepared statements appear to
// fail when loaded dynamically
#if !defined(W1MYSQL_MINVERS)
# define W1MYSQL_MINVERS 9999999
#endif

static void my_params(char *params,
                      char **host, char **dbname, char **user, char **pass)
{
    char *mp = strdup(params);
    char *s0,*s1;

    for(s0 = mp; (s1=strsep(&s0, " "));)
    {
        char t[256],v[256];
        if (2 == sscanf(s1,"%256[^=]=%256s", t, v))
        {
            if(strcmp(t,"dbname") == 0)
            {
                *dbname = strdup(v);
            }
            else if (strcmp(t,"host") == 0)
            {
                *host = strdup(v);
            }
            else if (strcmp(t,"user") == 0)
            {
                *user = strdup(v);
            }
            else if (strcmp(t,"password") == 0)
            {
                *pass = strdup(v);
            }
        }
    }
    free(mp);
}

static MYSQL * w1_opendb(char *params)
{
    MYSQL *conn = NULL;
    char *dbname, *user, *password, *host;
    host = dbname = user = password = NULL;
    my_params(params, &host, &dbname, &user, &password);

    conn = mysql_init(NULL);

    if (0 == mysql_real_connect(conn, host, user, password,
                                dbname, 0, NULL, 0))
    {
        perror(mysql_error(conn));
        conn = NULL;
    }
    if(dbname)
        free(dbname);
    if(user)
        free(user);
    if(host)
        free(host);
    if (password)
        free(password);
    return conn;
}

void  w1_init (w1_devlist_t *w1, char *dbnam)
{
    w1_device_t * devs = NULL;
    char *sql = "select * from w1sensors order by device";
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *field;
    int n = 0;
    int nr = 0;
    int nx = 0;
    int nn = 0;
    int id = -1;
    int it = -1;
    int ni = 0;

    conn = w1_opendb(dbnam);
    if (conn && !mysql_query(conn, sql))
    {
        res = mysql_store_result(conn);
        nr = mysql_num_rows(res);

        devs = malloc(sizeof(w1_device_t)*nr);
        memset(devs, 0, sizeof(w1_device_t)*nr);

        for (n = 0; n < nr; n++)
        {
            int j;
            char *fnam;

            row = mysql_fetch_row(res);
            int nf = mysql_num_fields(res);
            if (n == 0)
            {
                for(j = 0; j < nf; j++)
                {
                    field = mysql_fetch_field_direct(res, j);
                    fnam = field->name;
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
            }

            nn = w1_get_device_index(devs, ni, row[id], row[it]);
            if (nn == -1)
            {
                nx = ni;
                ni++;
            }
            else
            {
                nx = nn;
            }

            for(j = 0; j < nf; j++)
            {
                char *s = row[j];
                char *sv = (s && *s) ? strdup(s) : NULL;
                field = mysql_fetch_field_direct(res, j);
                fnam = field->name;
                if(sv)
                    w1_set_device_data(devs+nx, fnam, sv);
            }
            w1_enumdevs(devs+nx);
        }
        w1->numdev = ni;
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
#if MYSQL_VERSION_ID > W1MYSQL_MINVERS
static MYSQL_STMT *stmt;
#endif

void w1_cleanup(void)
{
    if(conn)
    {
#if MYSQL_VERSION_ID > W1MYSQL_MINVERS
        mysql_stmt_close(stmt);
        stmt = NULL;
#endif
        mysql_close(conn);
        conn = NULL;
    }
}

void w1_logger(w1_devlist_t *w1, char *params)
{
    int i = 0;
    w1_device_t *devs;

    if (access("/tmp/.w1retap.lock", F_OK) == 0)
    {
        return;
    }

    if(conn == NULL)
    {
        if(w1->verbose)
            fprintf(stderr, "mysql version check %d %d\n", MYSQL_VERSION_ID, W1MYSQL_MINVERS);

        conn = w1_opendb(params);
#if MYSQL_VERSION_ID > W1MYSQL_MINVERS
        mysql_autocommit(conn, 0);
#else
        mysql_real_query(conn, "SET AUTOCOMMIT=0",
                         sizeof("SET AUTOCOMMIT=0")-1);
#endif
    }
#if MYSQL_VERSION_ID > W1MYSQL_MINVERS

    if(w1->verbose)
        fprintf(stderr, "Using prepared statement\n");

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
#if MYSQL_VERSION_ID > W1MYSQL_MINVERS
            MYSQL_BIND   bind[3];
#endif
            for (j = 0; j < devs->ns; j++)
            {
                if(devs->s[j].valid)
                {
#if MYSQL_VERSION_ID > W1MYSQL_MINVERS
                    memset(bind, 0, sizeof(bind));
                    struct tm *tm = NULL;
                    if(w1->timestamp)
                    {
                        tm = (w1->force_utc) ? gmtime(&w1->logtime) :
                            localtime(&w1->logtime);
                        MYSQL_TIME mtm = {0};
                        mtm.year = tm->tm_year+1900;
                        mtm.month = tm->tm_mon+1;
                        mtm.day = tm->tm_mday;
                        mtm.hour= tm->tm_hour;
                        mtm.minute= tm->tm_min;
                        mtm.second= tm->tm_sec;
                        mtm.neg = 0;
                        mtm.second_part = 0;
                        mtm.time_type =  MYSQL_TIMESTAMP_DATETIME;
                        bind[0].buffer_type= MYSQL_TYPE_TIMESTAMP;
                        bind[0].buffer= (char *)&mtm;
                        bind[0].is_null= (my_bool*) 0;
                        bind[0].buffer_length = sizeof(MYSQL_TIME);
                    }
                    else
                    {
                        bind[0].buffer_type= MYSQL_TYPE_LONG;
                        bind[0].buffer= (char *)&w1->logtime;
                        bind[0].buffer_length = sizeof(w1->logtime);
                        bind[0].is_null= (my_bool*) 0;
                    }

                    bind[1].buffer_type= MYSQL_TYPE_VAR_STRING;
                    bind[1].buffer= devs->s[j].abbrv;
                    bind[1].is_null= (my_bool*) 0;
                    bind[1].buffer_length= strlen(bind[1].buffer);

                    bind[2].buffer_type= MYSQL_TYPE_DOUBLE;
                    bind[2].buffer= (char *)&devs->s[j].value;
                    bind[2].is_null= (my_bool*) 0;
                    bind[2].buffer_length= 0;

                    if(mysql_stmt_bind_param(stmt, bind))
                    {
                        fputs("bad bind\n", stderr);
                    }

                    if(mysql_stmt_execute(stmt))
                    {
                        fprintf(stderr, "execute:  %s\n", mysql_error(conn));
                        fprintf(stderr, "timestamp = %d\n", w1->timestamp);
                        fprintf(stderr, "logtime = %ld\n", w1->logtime);
                        if(tm != NULL)
                        {
                            char tval[64];
                            strftime(tval, sizeof(tval), "'%F %T'", tm);
                            fprintf(stderr, "timestamp %s %d\n", tval, w1->force_utc);
                        }
                    }
#else
                    char *q;
                    char tval[64];

                    if(w1->timestamp)
                    {
                        struct tm *tm;
                        tm = (w1->force_utc) ? gmtime(&w1->logtime) : localtime(&w1->logtime);
                        strftime(tval, sizeof(tval), "'%F %T'", tm);
//                        printf("timestamp %s %d\n", tval, strlen(tval1));
                    }
                    else
                    {
                        snprintf(tval, sizeof(tval), "%ld", w1->logtime);
//                        printf("time_t %s\n", tval);
                    }
                    asprintf(&q,
                             "INSERT into readings(date,name,value) VALUES(%s,'%s',%g)",
                             tval, devs->s[j].abbrv, (double)devs->s[j].value);

                    if(w1->verbose)
                        printf("SQL:%s\n", q);

                    if(0 != mysql_real_query(conn, q, strlen(q)))
                    {
                        const char *mse;
                        mse = mysql_error(conn);
                        if (mse)
                        {
                            syslog(LOG_ERR, "MySQL error %s", mse);
                            if(w1->verbose)
                                fprintf(stderr, "Err:%s\n", mse);
                        }
                    }
                    free(q);
#endif
                }
            }
        }
    }
#if MYSQL_VERSION_ID > W1MYSQL_MINVERS
    mysql_commit(conn);
#else
    mysql_real_query(conn, "COMMIT", sizeof("COMMIT")-1);
#endif
}
