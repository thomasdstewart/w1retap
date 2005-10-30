#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "w1temp-rpc.h"

#define NAME  "XML-RPC wxrpc Client"
#define VERSION "0.01"

int get_xmlrpc_wx(W1RPC_t *w, char *tbuf, double *d)
{
    xmlrpc_value *items, *item;
    size_t size, i;

    *d = -999;
    if(w->srv == NULL)
    {
        init_xmlrpc(w);
    }
    if(w->srv)
    {
        items = xmlrpc_client_call_server((xmlrpc_env *)w->env,
                                          (xmlrpc_server_info *)w->srv,
                                          "w1retap.currentWeather",
                                          "()","");
        if(((xmlrpc_env *)w->env)->fault_occurred == 0)
        {
            size = xmlrpc_array_size((xmlrpc_env *)w->env, items);
            if(((xmlrpc_env *)w->env)->fault_occurred == 0)
            {
                *tbuf = 0;
                int nc = 0;
                
                for (i = 0; i < size; i++)
                {
                    char * name;
                    item = xmlrpc_array_get_item((xmlrpc_env *)w->env,
                                                 items, i);
                    if(xmlrpc_struct_has_key((xmlrpc_env *)w->env,
                                             item, "units"))
                    {
                        double v;
                        char * units;
                        xmlrpc_decompose_value((xmlrpc_env *)w->env,
                                               item, "{s:s,s:d,s:s,*}",
                                               "name", &name,
                                               "value", &v,
                                               "units", &units);
                        if ((0 == strcasecmp(w->key, name)))
                        {
                            *d = v;
                        }
                        nc+=sprintf(tbuf+nc,"%s: %.2f %s\n", name, v, units);
                        free(name);
                        free(units);
                    }
                    else
                    {
                        char *dstr;
                        xmlrpc_decompose_value((xmlrpc_env *)w->env,
                                               item, "{s:s,s:s,*}",
                                               "name", &name, "value", &dstr);
                        strcpy(tbuf+nc, dstr);
                        nc += strlen(dstr);
                        free(name);
                        free(dstr);
                    }
                }
            }
        }
        else
        {
            fprintf(stderr, "XML-RPC Fault #%d: %s\n",
                    ((xmlrpc_env *)w->env)->fault_code,
                    ((xmlrpc_env *)w->env)->fault_string);
        }
        xmlrpc_DECREF(items);
    }
    int rv = ((xmlrpc_env *)w->env)->fault_occurred;
    if(rv != 0)
    {
        close_xmlrpc(w);
    }
    return rv;
}

int init_xmlrpc(W1RPC_t *w)
{
    static struct xmlrpc_clientparms clientParms;
    static struct xmlrpc_curl_xportparms curlParms;
    int rv;
    
    w->env = malloc(sizeof(xmlrpc_env));
    xmlrpc_env_init((xmlrpc_env *)w->env);

    curlParms.network_interface = NULL;
    curlParms.no_ssl_verifypeer = 1;
    curlParms.no_ssl_verifyhost = 1;
    curlParms.user_agent        = "wxtemp/1.0";

    clientParms.transport = "curl";
    clientParms.transportparmsP = (struct xmlrpc_xportparms *) &curlParms;
    clientParms.transportparm_size = XMLRPC_CXPSIZE(user_agent);

    w->srv = (xmlrpc_server_info *)xmlrpc_server_info_new(
        (xmlrpc_env *)w->env, w->url);

    if((rv = ((xmlrpc_env *)w->env)->fault_occurred) == 0)
    {
        if(w->user && w->pass)
        {
            xmlrpc_server_info_set_basic_auth((xmlrpc_env *)w->env,
                                              (xmlrpc_server_info *)w->srv,
                                              w->user, w->pass);
        }

        xmlrpc_client_init2((xmlrpc_env *)w->env, XMLRPC_CLIENT_NO_FLAGS,
                            NAME, VERSION, 
                            &clientParms, XMLRPC_CPSIZE(transportparm_size));

    }
    if((rv = ((xmlrpc_env *)w->env)->fault_occurred))
    {
        close_xmlrpc(w);
    }
    return rv;
}

void close_xmlrpc(W1RPC_t *w)
{
    xmlrpc_server_info_free((xmlrpc_server_info *)w->srv);
    w->srv = NULL;
    xmlrpc_env_clean((xmlrpc_env *)w->env);
    xmlrpc_client_cleanup();
    free(w->env);
    w->env = NULL;
}
