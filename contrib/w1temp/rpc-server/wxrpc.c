#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmlrpc.h>
#include <xmlrpc_cgi.h>

#define STATICFILE "/tmp/.wx_static.dat"

xmlrpc_value * getWeather (xmlrpc_env *env, xmlrpc_value *param_array,
                           void *user_data)
{
    FILE *fp;
    xmlrpc_value * arry, *x, *r = NULL;
    if(NULL != (fp = fopen(STATICFILE, "r")))
    {
        char buf[128];
        if(NULL != (arry = xmlrpc_array_new(env)))
        {
            while(fgets(buf, sizeof(buf), fp))
            {
                char *n = 0, *v = 0, *u = 0;
                sscanf(buf, "%as %as %as", &n, &v, &u);
                if(n && v && u)
                {
                    if(0 == strcmp(n, "Date"))
                    {
                        x = xmlrpc_build_value(env,"{s:s,s:s}",
                                               "name", n, "value", v);
                    }
                    else
                    {
                        double d = strtod(v, NULL);
                        x = xmlrpc_build_value(env,"{s:s,s:d,s:s}",
                                               "name", n, "value", d, "units", u);
                    }
                    if(x)
                    {
                        xmlrpc_array_append_item (env, arry, x);
                    }
                    free(n);
                    free(v);
                    free(u);
                }
            }
        }
        fclose(fp);
        r = xmlrpc_build_value(env, "A", arry);
    }
    return r;
}

int main (int argc, char **argv)
{
    /* Set up our CGI library. */
    xmlrpc_cgi_init(XMLRPC_CGI_NO_FLAGS);

    /* Install our only method (with a method signature and a help string). */
    xmlrpc_cgi_add_method_w_doc("w1retap.currentWeather",
                                &getWeather, NULL, "",
                                "Get the weather.");

    /* Call the appropriate method. */
    xmlrpc_cgi_process_call();

    /* Clean up our CGI library. */
    xmlrpc_cgi_cleanup();
    return 0;
}
