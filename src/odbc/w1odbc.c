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
#include <sql.h>
#include <sqlext.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>
#include "w1retap.h"

void w1_init(w1_devlist_t *w1, char *params)
{
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret; 
    SQLSMALLINT columns; 
    const char *sql =
        "select * from w1sensors";
    SQLLEN rows = 0;
    w1_device_t * devs = NULL;
    int n = 0;
    
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    ret = SQLDriverConnect(dbc, NULL, (unsigned char *)params, SQL_NTS,
                     NULL, 0, NULL, SQL_DRIVER_COMPLETE);

    if (SQL_SUCCEEDED(ret))
    {
        char cnam[256];
        SQLSMALLINT lcnam;
        int i;
        char ** flds, **f;
        int id=-1,it=-1;
        int nx = 0, ni = 0, nn = 0;

        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        SQLExecDirect(stmt, (unsigned char *)sql, SQL_NTS);

        SQLNumResultCols(stmt, &columns);
        SQLRowCount(stmt, &rows);
        devs = malloc(sizeof(w1_device_t)*rows);
        memset(devs, 0, sizeof(w1_device_t)*rows);
        flds = malloc((columns*sizeof(char *)));
                    
        for (f=flds, i = 0; i < columns; i++, f++)
        {
            SQLDescribeCol(stmt,i+1, (SQLCHAR*)cnam, sizeof(cnam), &lcnam,
                           NULL, NULL, NULL, NULL);
            *f = strdup(cnam);
            if(strcmp(cnam,"device") == 0)
            {
                id = i;
            }
            else if (strcmp(cnam, "type") == 0)
            {
                it = i;
            }
        }

        while (SQL_SUCCEEDED(ret = SQLFetch(stmt)))
        {
            int  i;
            char device[32] = {0};
            char type[32] = {0}; 
            SQLLEN indicator;
            
            SQLGetData(stmt, id+1, SQL_C_CHAR, device, sizeof(device),
                                 &indicator);
            SQLGetData(stmt, it+1, SQL_C_CHAR, type, sizeof(type),
                                 &indicator);
            
            nn = w1_get_device_index(devs, ni, device, type);
            if (nn == -1)
            {
                nx = ni;
                ni++;
            }
            else
            {
                nx = nn;
            }

            for (i = 0; i < columns; i++)
            {
                char buf[512];
                
                ret = SQLGetData(stmt, i+1, SQL_C_CHAR, buf, sizeof(buf),
                                 &indicator);
                if (SQL_SUCCEEDED(ret))
                {
                    char *sv;
                    if (indicator == SQL_NULL_DATA)
                    {
                        sv = NULL;
                    }
                    else
                    {
                        sv = strdup(buf);
                    }
                    if(sv)
                        w1_set_device_data (devs+nx, flds[i], sv);
                }
            }
            w1_enumdevs(devs+nx);
            n++;
        }
        w1->numdev = ni;
        w1->devs=devs;
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);    

        for (f=flds, i = 0; i < columns; i++, f++)
        {
            free(*f);
        }
        free(flds);
        
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        ret = SQLExecDirect(stmt, (unsigned char *)"select name,value,rmin,rmax from ratelimit", SQL_NTS);
        SQLNumResultCols(stmt, &columns);
        SQLRowCount(stmt, &rows);
        while (SQL_SUCCEEDED(ret = SQLFetch(stmt)))
        {
            SQLLEN indicator;
            char buf[512];
            char *s;
            short flags = 0;
            float roc=0,rmin=0,rmax=0;
            
            ret = SQLGetData(stmt, 1, SQL_C_CHAR, buf, sizeof(buf),
                             &indicator);
            if (SQL_SUCCEEDED(ret))
            {
                if (indicator == SQL_NULL_DATA)
                {
                    s = NULL;
                }
                else
                {
                    s = strdup(buf);
                    ret = SQLGetData(stmt, 2, SQL_C_CHAR, buf, sizeof(buf),
                                     &indicator);
                    if (SQL_SUCCEEDED(ret))
                    {
                        if (indicator != SQL_NULL_DATA)
                        {
                            roc = strtof(buf, NULL);
                            flags |= W1_ROC;
                        }
                    }
                    ret = SQLGetData(stmt, 3, SQL_C_CHAR, buf, sizeof(buf),
                                     &indicator);
                    if (SQL_SUCCEEDED(ret))
                    {
                        if (indicator != SQL_NULL_DATA)
                        {
                            rmin = strtof(buf, NULL);
                            flags |= W1_RMIN;
                        }
                    }
                    ret = SQLGetData(stmt, 4, SQL_C_CHAR, buf, sizeof(buf),
                                     &indicator);
                    if (SQL_SUCCEEDED(ret))
                    {
                        if (indicator != SQL_NULL_DATA)
                        {
                            rmax = strtof(buf, NULL);
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
                    free(s);
                }
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);    
    }
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}

static SQLHENV env;
static SQLHDBC dbc;
static SQLHSTMT stmt;

void w1_cleanup(void)
{
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    dbc = env = stmt = NULL;
}


void w1_logger (w1_devlist_t *w1, char *params)
{
    w1_device_t * devs = NULL;
    int i = 0;
    
    if(env == NULL)
    {
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
        SQLDriverConnect(dbc, NULL, (unsigned char *)params, SQL_NTS,
                         NULL, 0, NULL, SQL_DRIVER_COMPLETE);
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        SQLPrepare (stmt, (unsigned char *)"insert into readings(date,name,value)  values (?,?,?)", SQL_NTS);
    }

    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < devs->ns; j++)
            {
                if(devs->s[j].valid)
                {
                    SQLLEN psz;
                    SQL_TIMESTAMP_STRUCT sqlts;
                    SQLSMALLINT ValueType;
                    SQLSMALLINT ParameterType;
                    SQLPOINTER ParameterValuePtr;
                    int res;
                    
                    if(w1->timestamp)
                    {
                        struct tm *tm = localtime(&w1->logtime);
                        sqlts.year = tm->tm_year + 1900;
                        sqlts.month = tm->tm_mon + 1;
                        sqlts.day = tm->tm_mday;
                        sqlts.hour = tm->tm_hour;
                        sqlts.minute = tm->tm_min;
                        sqlts.second = tm->tm_sec;
                        sqlts.fraction = 0;
                        ValueType= SQL_C_TIMESTAMP;
                        ParameterType = SQL_TIMESTAMP;
                        ParameterValuePtr = &sqlts;
                        psz = sizeof(sqlts);                       
                    }
                    else
                    {
                        ValueType= SQL_C_LONG;
                        ParameterType = SQL_INTEGER;
                        ParameterValuePtr = &w1->logtime;
                        psz = sizeof(&w1->logtime);
                    }
        
                    res = SQLBindParameter(stmt, 1, SQL_PARAM_INPUT,
                                           ValueType,
                                           ParameterType,
                                           0, 0,
                                           ParameterValuePtr, psz, &psz);
                    psz = strlen(devs->s[j].abbrv);
                    res = SQLBindParameter(stmt, 2, SQL_PARAM_INPUT,
                                           SQL_C_CHAR,
                                           SQL_VARCHAR,
                                           0, 0,
                                           devs->s[j].abbrv, psz, &psz);
                    psz = sizeof(float);
                    res = SQLBindParameter(stmt, 3, SQL_PARAM_INPUT,
                                           SQL_C_FLOAT,
                                           SQL_REAL,
                                           0, 0,
                                           &devs->s[j].value, psz, &psz);
                    res = SQLExecute(stmt);
                }
            }
        }
    }
}

#if defined(TESTBIN)
int main()
{
    w1_devlist_t w = {0},*w1;
    int n;

    w1=&w;
    w1_init(w1, "DSN=w1retap");

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
    w1_logger(w1, "DSN=w1retap");

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
    w1_logger(w1, "DSN=w1retap");
    return 0;
}
#endif

