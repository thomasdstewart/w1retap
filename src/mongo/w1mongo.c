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
#include <mongoc/mongo.h>
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

/*
 * As mongo C api (alas) requires IPv4 addressing, we could use
 * gethostbyname, but this is a useful example.
 *
 * This code becomes unnecessary once the mongo C api works with host
 * names and IPV6
 */

#define USE_MY_RESOLVER alas

#ifdef USE_MY_RESOLVER
const char * get_str_host(char *name)
{
    const char *res=NULL;
    static char straddr[INET6_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // reallyAF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(name, NULL, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return NULL;
    }
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        void *addr;
//        if (p->ai_family == AF_INET)
//        {
        addr = &(((struct sockaddr_in *)p->ai_addr)->sin_addr);
//        }
//        else
//        {
//          addr = &(((struct sockaddr_in6 *)p->ai_addr)->sin6_addr);
//        }
        res = inet_ntop(p->ai_family, addr, straddr, sizeof straddr);
        if(res)
            break;
    }
    freeaddrinfo(servinfo);
    return res;
}
#else
#define get_str_host(v) (v)
#endif


static char *dbname;
static mongo conn[1];

static void mongo_params(char *params, w1repset_t **rset)
{
    char *mp = strdup(params);
    char *s0,*s1;
    char *repset = NULL;
    char *host = NULL;
    int port = 0;
    w1repset_t *rs = NULL;
    const char *addr;
    
    for(s0 = mp; (s1=strsep(&s0, " "));)
    {
        char t[256],v[256];
        if (2 == sscanf(s1,"%256[^=]=%256s", t, v))
        {
            if (strcmp(t,"host") == 0)
            {
                addr = get_str_host(v);
                host = (addr) ? strdup(addr) : strdup(v);
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
                addr = get_str_host(s1);
                rs->hp[nh].host = (addr) ? strdup(addr) : strdup(s1);
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

static mongo* w1_opendb(char *params)
{
    mongo *pconn = NULL;
    long status=-1;
    w1repset_t *rset = NULL;
    
    mongo_params(params, &rset);
    if(rset && rset->nhosts > 0)
    {
        if(rset->setname)
        {
            int i;
            
            mongo_replset_init( conn, rset->setname);
            for(i = 0; i < rset->nhosts; i++)
            {
                mongo_replset_add_seed(conn, rset->hp[i].host,
                                       rset->hp[i].port);
            }
            status = mongo_replset_connect( conn );
        }
        else
        {
            status = mongo_connect( conn, rset->hp[0].host, rset->hp[0].port);
        }
        
        if( status != MONGO_OK )
        {
            switch ( conn->err )
            {
                case MONGO_CONN_SUCCESS:
                    fputs("connection succeeded\n", stderr);
                    break;
                case MONGO_CONN_NO_SOCKET:
                    fputs("no socket\n", stderr);
                    break;
                case MONGO_CONN_FAIL:
                    fputs( "connection failed\n",stderr);
                    break;
                case MONGO_CONN_NOT_MASTER:
                    fputs("not master\n",stderr);
                    break;
                default:
                    fprintf(stderr, "Mongo error %d\n", conn->err);
                    break;
            }
        }
        else
        {
            pconn = conn;
        }
    }
    rset_free(rset);
    return pconn;
}

void  w1_init (w1_devlist_t *w1, char *params)
{
    w1_device_t * devs = NULL;
    mongo *pconn;
    int nx = 0;
    int nn = 0;
    int ni = 0;

    pconn = w1_opendb(params);
    if(pconn)
    {
        char *device;
        char *dtype;
        mongo_cursor cursor[1];
        const char * key;
        const char *value;
        char buff[32];
        double d;
        long l;
        char collection[128];
        
        int nr = mongo_count(conn,dbname,"w1sensors",NULL);
        
        devs = malloc(sizeof(w1_device_t)*nr);
        memset(devs, 0, sizeof(w1_device_t)*nr);

        bson query[1];        
        bson_init( query );
        bson_append_start_object( query, "$query" );
        bson_append_finish_object( query );
        bson_append_start_object( query, "$orderby");
        bson_append_int( query, "device", 1);
        bson_append_finish_object( query );
        bson_finish( query );

        snprintf(collection, sizeof(collection), "%s.w1sensors", dbname);
        mongo_cursor_init(cursor, conn, collection);
        mongo_cursor_set_query(cursor, query);

        while( mongo_cursor_next(cursor) == MONGO_OK )
        {
            bson_iterator it[1];
            device = dtype = NULL;
            if ( bson_find( it, mongo_cursor_bson( cursor ), "device" )) {
                device = strdup(bson_iterator_string(it));
            }
            if ( bson_find( it, mongo_cursor_bson( cursor ), "type" )) {
                dtype = strdup(bson_iterator_string(it));
            }
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
                bson_iterator_init( it, &cursor->current);
                while(bson_iterator_more(it))
                {
                    type =  bson_iterator_next(it);
                    key = bson_iterator_key(it);
                    value=NULL;
                    switch (type)
                    {
                        case BSON_STRING:
                            value =  bson_iterator_string(it);
                            break;
                        case BSON_INT:
                            l =  bson_iterator_int(it);
                            snprintf(buff, sizeof(buff), "%ld", l);
                            value = buff;
                            break;
                        case BSON_LONG:
                            l =  bson_iterator_long(it);
                            snprintf(buff, sizeof(buff), "%ld", l);
                            value = buff;
                            break;
                        case BSON_DOUBLE:
                            d =bson_iterator_double(it);
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
        }
        mongo_cursor_destroy( cursor );
        w1->numdev = ni;
        w1->devs=devs;
        snprintf(collection, sizeof(collection), "%s.ratelimit", dbname);
        mongo_cursor_init(cursor, conn, collection);
        while (mongo_cursor_next(cursor) == MONGO_OK)
        {
            bson_iterator it[1];
            short flags = 0;
            char *name = NULL;
            double roc=0,rmin=0,rmax=0;

            if (BSON_STRING == bson_find(it, mongo_cursor_bson(cursor), "name"))
            {
                name = strdup(bson_iterator_string(it));
            }
            if (BSON_DOUBLE == bson_find(it, mongo_cursor_bson(cursor), "value" ))
            {
                flags |= W1_ROC;
                roc =  bson_iterator_double(it);
            }
            if (BSON_DOUBLE == bson_find(it, mongo_cursor_bson(cursor), "rmin"))
            {
                flags |= W1_RMIN;
                rmin = bson_iterator_double(it);
            }
            if (BSON_DOUBLE == bson_find(it, mongo_cursor_bson(cursor), "rmax"))
            {
                flags |= W1_RMAX;
                rmax = bson_iterator_double(it);
            }

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
        mongo_cursor_destroy(cursor);
        mongo_destroy(pconn);
    }
    else
    {
        exit(1);
    }
}

static mongo *mconn;
void w1_cleanup(void)
{
    free(dbname);
    dbname = NULL;
    mongo_destroy(conn);
    mconn = NULL;
}

void w1_logger(w1_devlist_t *w1, char *params)
{
    int i,nv=0;
    w1_device_t *devs;
    bson b[1];
    
    if (access("/tmp/.w1retap.lock", F_OK) == 0)
    {
        return;
    }
    
    if(mconn == NULL)
    {
        mconn = w1_opendb(params);
    }

    if(mconn)
    {
        if(mongo_check_connection (mconn) != MONGO_OK)
        {
            w1_cleanup();
            mconn = w1_opendb(params);
        }
        
    }
    
    if(mconn == NULL)
    {
        syslog(LOG_ERR, "mongo error: %d", conn->err);
        return;
    }
    
    bson_init(b);
    bson_append_new_oid(b, "_id");
    bson_append_time_t(b, "date", w1->logtime);

    for(devs = w1->devs, i = 0; i < w1->numdev; i++, devs++)
    {
        if(devs->init)
        {
            int j;
            for (j = 0; j < devs->ns; j++)
            {
                if(devs->s[j].valid)
                {
                    bson_append_double(b, devs->s[j].abbrv, devs->s[j].value);
                    nv++;
                }
            }
        }
    }
    bson_finish(b);
    if(nv > 0)
    {
        char collection[128];
        snprintf(collection, sizeof(collection), "%s.readings", dbname);
        mongo_insert( conn, collection, b );
    }
    bson_destroy( b );
}

void w1_report(w1_devlist_t *w1, char *dbnam)
{
    if(w1->lastmsg)
    {
         bson b[1];
         time_t now;
         char collection[128];
         snprintf(collection, sizeof(collection), "%s.replog", dbname);
         
         time(&now);
         bson_init(b);
         bson_append_new_oid(b, "_id");
         bson_append_time_t(b, "date", now);
         bson_append_string(b, "message", w1->lastmsg);
         bson_finish(b);
         mongo_insert(conn, collection, b);
         bson_destroy(b);
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
