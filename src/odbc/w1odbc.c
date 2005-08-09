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
        "select device,type,abbrv1,name1,units1,abbrv2,name2,units2 from w1sensors";
    SQLINTEGER rows = 0;
    w1_device_t * devs = NULL;
    int n = 0;
    
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    SQLDriverConnect(dbc, NULL, params, SQL_NTS,
                     NULL, 0, NULL, SQL_DRIVER_COMPLETE);

    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    SQLExecDirect(stmt, (char *)sql, SQL_NTS);

    SQLNumResultCols(stmt, &columns);
    SQLRowCount(stmt, &rows);
    devs = malloc(sizeof(w1_device_t)*rows);
    memset(devs, 0, sizeof(w1_device_t)*rows);

    while (SQL_SUCCEEDED(ret = SQLFetch(stmt)))
    {
        int  i;
        for (i = 0; i < columns; i++)
        {
            SQLINTEGER indicator;
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

                switch (i)
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
            
        }
        n++;
    }
    w1->numdev = n;
    w1->devs=devs;
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
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
        SQLDriverConnect(dbc, NULL, params, SQL_NTS,
                         NULL, 0, NULL, SQL_DRIVER_COMPLETE);
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        SQLPrepare (stmt, "insert into readings values (?,?,?)", SQL_NTS);
    }

    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < 2; j++)
            {
                if(devs->s[j].valid)
                {
                   SQLINTEGER psz;
                   int res;
                   psz = sizeof(int);
                   res = SQLBindParameter(stmt, 1, SQL_PARAM_INPUT,
                                          SQL_C_LONG,
                                          SQL_INTEGER,
                                          0, 0,
                                          &w1->logtime, sizeof(int), &psz);
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
                                           &devs->s[j].value, sizeof(float),
                                           &psz);
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

