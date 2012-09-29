/*
 * Copyright (C) 2011 Jonathan Hudson <jh+w1retap@daria.co.uk>
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <inttypes.h>
#include <mongo-client/mongo.h>
#include "w1retap.h"

typedef struct
{
    char *host;
    int port;
} hport_t;

typedef struct 
{
    char *setname;
    char *dname;
    int nhosts;
    hport_t hp[0];
} w1repset_t;

#define MDEF_PORT 27017

static char *dbname;
static mongo_sync_connection *conn;
static char mkid=FALSE;

static void mongo_params(char *params, w1repset_t **rset)
{
    char *mp = strdup(params);
    char *s0,*s1;
    char *repset = NULL;
    char *host = NULL;
    int port = 0;
    w1repset_t *rs = NULL;
    
    for(s0 = mp; (s1=strsep(&s0, " "));)
    {
        char t[256],v[256];
        if (2 == sscanf(s1,"%256[^=]=%256s", t, v))
        {
            if (strcmp(t,"host") == 0)
            {
                host = strdup(v);
            }
            else if (strcmp(t,"port") == 0)
            {
                port = strtol(v,NULL,0);
            }
            else if (strcmp(t,"dbname") == 0)
            {
                dbname = strdup(v);
            }
            else if (strcmp(t,"replica") == 0)
            {
                repset = strdup(v);
            }
        }
    }
    if(repset)
    {
        char *p,*q;
        int nh = 0;
        int req;
        
        for(p = repset; *p ; p++)
        {
            if(*p == ',')
                nh++;
        }
        req = sizeof(w1repset_t) + nh * sizeof(hport_t);
        rs = calloc(1, req);

        nh = -1;
        for(p = repset; (s1=strsep(&p, ",")); nh++)
        {
            if(nh == -1)
            {
                rs->setname = strdup(s1);
            }
            else
            {
                port = 0;
                if(NULL != (q = strchr(s1,'/')))
                {
                    *q++ = 0;
                    port = strtol(q, NULL, 0);
                }
                rs->hp[nh].port = (port) ? port : MDEF_PORT;
                rs->hp[nh].host = strdup(s1);
            }
        }
        rs->nhosts = nh;
        free(host);
    }
    else if (host)
    {
        rs = calloc(1, sizeof(w1repset_t) + sizeof(hport_t));
        rs->nhosts = 1;
        rs->hp[0].host = host;
        rs->hp[0].port = (port) ? port : MDEF_PORT;
    }
    if(dbname == NULL)
    {
        dbname = strdup("wx");
    }
    free(repset);
    *rset = rs;
    free(mp);
}

static void rset_free(w1repset_t *r)
{
    int i;
    free(r->setname);
    for(i = 0; i < r->nhosts; i++)
    {
        free(r->hp[i].host);
    }
    free(r);
}

static mongo_sync_connection * w1_opendb(char *params)
{
    w1repset_t *rset = NULL;
    mongo_params(params, &rset);

#ifdef TESTBIN
    int i;
    if (rset->setname)
        printf("%s\n", rset->setname);
    for(i = 0; i < rset->nhosts; i++)
    {
        printf("%s/%d\n", rset->hp[i].host, rset->hp[i].port);
    }
#endif

        /* Really, we don't care about rep sets with this API */

    if(rset && rset->nhosts > 0)
    {
        if(rset->setname)
        {
            int i;
            conn = mongo_sync_connect(rset->hp[0].host, rset->hp[0].port, TRUE);
            mongo_sync_conn_set_auto_reconnect (conn, TRUE);
            for(i = 1; i < rset->nhosts; i++)
            {
                mongo_sync_conn_seed_add (conn, rset->hp[i].host,
                                       rset->hp[i].port);
            }
        }
        else
        {
            conn = mongo_sync_connect(rset->hp[0].host, rset->hp[0].port, TRUE);
        }
        
        if( conn == NULL )
        {
            perror("Connection");
            exit(1);
        }
    }
    rset_free(rset);
    if(mkid == FALSE)
    {
        mkid=TRUE;
        mongo_util_oid_init(0);
    }

    return conn;
}

void  w1_init (w1_devlist_t *w1, char *params)
{
    w1_device_t * devs = NULL;
    int nx = 0;
    int nn = 0;
    int ni = 0;

    if(conn == NULL)
        conn = w1_opendb(params);

    if(conn)
    {
        gchar *device;
        gchar *dtype;
        const char * key;
        const char *value;
        char buff[32];
        gdouble d;
        gint64 i64;
        gint32 i32;
        char collection[128];
        bson *query;

        int nr =  (int)mongo_sync_cmd_count(conn, dbname,"w1sensors",NULL);
        devs = malloc(sizeof(w1_device_t)*nr);
        memset(devs, 0, sizeof(w1_device_t)*nr);

        bson *nq =bson_new ();
        bson_finish (nq);
        query = bson_build_full (BSON_TYPE_DOCUMENT, "$query", TRUE, nq,
                                 BSON_TYPE_DOCUMENT, "$orderby", TRUE,
                                 bson_build (BSON_TYPE_INT32, "device", 1,
                                             BSON_TYPE_NONE),
                                 BSON_TYPE_NONE);
        bson_finish (query);
        
        snprintf(collection, sizeof(collection), "%s.w1sensors", dbname);
        
        mongo_packet *p;
        mongo_sync_cursor *cursor;
        
        p = mongo_sync_cmd_query (conn, collection, 0, 0, nr, query, NULL);
        cursor = mongo_sync_cursor_new (conn, collection, p);

        while (mongo_sync_cursor_next (cursor))
        {
            bson *result = mongo_sync_cursor_get_data (cursor);
            bson_cursor *c;
            const gchar *ds;
            device = dtype = NULL;
            c = bson_find (result, "device");
            bson_cursor_get_string (c, &ds);
            device=strdup(ds);
            bson_cursor_free (c);
            c = bson_find (result, "type");
            bson_cursor_get_string (c, &ds);
            bson_cursor_free (c);
            dtype=strdup(ds);
            if(device && dtype)
            {
                nn = w1_get_device_index(devs, ni, device, dtype);
                if (nn == -1)
                {
                    nx = ni;
                    ni++;
                }
                else
                {
                    nx = nn;
                }
                bson_type type;
                c= bson_cursor_new(result);
                while(bson_cursor_next(c))
                {
                    type =  bson_cursor_type(c);
                    key = bson_cursor_key(c);
                    value=NULL;
                    switch (type)
                    {
                        case BSON_TYPE_STRING:
                            bson_cursor_get_string(c,&value);
                            break;
                        case BSON_TYPE_INT32:
                            bson_cursor_get_int32(c,&i32);
                            snprintf(buff, sizeof(buff), "%d", i32);
                            value = buff;
                            break;
                        case BSON_TYPE_INT64:
                            bson_cursor_get_int64(c,&i64);
                            snprintf(buff, sizeof(buff), "%"PRId64, i64);
                            value = buff;
                            break;
                        case BSON_TYPE_DOUBLE:
                            bson_cursor_get_double(c,&d);
                            snprintf(buff, sizeof(buff), "%f", d);
                            value = buff;
                            break;
                        default:
                            break;
                    }
                    if(value)
                    {
                        char *sv = strdup(value);
                        w1_set_device_data(devs+nx, key, sv); 
                    }
                }
                w1_enumdevs(devs+nx);
            }
            free(device);
            free(dtype);
            bson_cursor_free (c);
            free(result);
        }
        w1->numdev = ni;
        w1->devs=devs;
        mongo_sync_cursor_free (cursor);
        bson_free(query);
//        bson_free(nq);
//        free(p);
        
        query=bson_new ();
        bson_finish (query);
        fflush(stdout);
        
        nr =  (int)mongo_sync_cmd_count(conn, dbname,"ratelimit",NULL);
        snprintf(collection, sizeof(collection), "%s.ratelimit", dbname);
        p = mongo_sync_cmd_query (conn, collection, 0, 0, nr, query, NULL);
        cursor = mongo_sync_cursor_new (conn, collection, p);
        while (mongo_sync_cursor_next (cursor))
        {
            bson_cursor *c;
            short flags = 0;
            char *name = NULL;
            const char *cname;
            double roc=0,rmin=0,rmax=0;
            bson *result = mongo_sync_cursor_get_data (cursor);

            c = bson_find (result, "name");
            if (bson_cursor_get_string (c, &cname))
            {
                name = strdup(cname);
            }
            bson_cursor_free (c);

            c = bson_find (result, "value");
            if (bson_cursor_get_double (c, &roc))
            {
                flags |= W1_ROC;
            }
            bson_cursor_free (c);

            c = bson_find (result, "rmin");
            if (bson_cursor_get_double (c, &rmin))
            {
                flags |= W1_RMIN;
            }
            bson_cursor_free (c);

            c = bson_find (result, "rmax");
            if (bson_cursor_get_double (c, &rmax))
            {
                flags |= W1_RMAX;
            }
            bson_cursor_free (c);

            if(flags)
            {
                w1_sensor_t *sensor;
                if (NULL != (sensor = w1_find_sensor(w1, (const char *)name)))
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
            free(name);
        }
        mongo_sync_cursor_free (cursor);
        bson_free(query);
    }
    else
    {
        exit(1);
    }
}

void w1_cleanup(void)
{
    free(dbname);
    dbname = NULL;
    mongo_sync_disconnect (conn);
    conn = NULL;
}

static guint8* w1_id_new(time_t *xnow)
{
    static time_t last;
    time_t now;
    static unsigned int  seq;

    now = time(xnow);
    if(now == last)
    {
        seq++;
    }
    else
    {
        seq = 0;
    }
    return mongo_util_oid_new(seq);
}

void w1_logger(w1_devlist_t *w1, char *params)
{
    int i,nv=0;
    w1_device_t *devs;
    bson *doc;
    
    if (access("/tmp/.w1retap.lock", F_OK) == 0)
    {
        return;
    }
    
    if(conn == NULL)
    {
       conn = w1_opendb(params);
    }

    if(conn == NULL)
    {
        syslog(LOG_ERR, "mongo conn error");
        return;
    }
    
    doc = bson_new();
    guint8* id =  w1_id_new(NULL);
    gint64 ts = w1->logtime*1000ULL;
#ifdef TESTBIN
    fprintf(stderr,"ts =  %ld %" PRId64 "\n", w1->logtime, ts);
#endif

    bson_append_oid(doc,"_id",id);
    bson_append_utc_datetime(doc, "date", ts);

    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < devs->ns; j++)
            {
                if(devs->s[j].valid)
                {
                    bson_append_double(doc, devs->s[j].abbrv, devs->s[j].value);
                    nv++;
                }
            }
        }
    }
    bson_finish(doc);
    if(nv > 0)
    {
        char collection[128];
        snprintf(collection, sizeof(collection), "%s.readings", dbname);
#ifdef TESTBIN
        fprintf(stderr,"coll = %s\n", collection);
#endif
        if(!mongo_sync_cmd_insert (conn, collection, doc, NULL))
        {
            perror ("mongo_sync_cmd_insert()");
        }
    }
    bson_free(doc);
    free(id);
}

void w1_report(w1_devlist_t *w1, char *dbnam)
{
    if(w1->lastmsg)
    {
         bson *b;
         time_t now;
         char collection[128];
         snprintf(collection, sizeof(collection), "%s.replog", dbname);
         guint8* id =  w1_id_new(&now);
         gint64 ts = now*1000;
         b = bson_new();
         bson_append_oid(b,"_id",id);
         bson_append_utc_datetime(b, "date", ts);
         bson_append_string(b, "message", w1->lastmsg, -1);
         bson_finish(b);
         mongo_sync_cmd_insert (conn, collection, b, NULL);
         bson_free(b);
         free(id);
    }
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
