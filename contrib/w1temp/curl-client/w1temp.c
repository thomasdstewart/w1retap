/*

Copyright (C) 2005 Jonathan Hudson <jh+w1temp@daria.co.uk>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <panel-applet.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <curl/curl.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include "config.h"

#define DBUS_SERVICE_NM      "org.freedesktop.NetworkManager"
#define DBUS_PATH_NM         "/org/freedesktop/NetworkManager"
#define DBUS_INTERFACE_NM    "org.freedesktop.NetworkManager"

#define INTERVAL 120
#define LSIZE 256
#define MAXSENS 20

enum NM_STATE {NM_STATE_UNKNOWN=0, NM_STATE_ASLEEP = 1, NM_STATE_CONNECTING = 2,
               NM_STATE_CONNECTED = 3, NM_STATE_DISCONNECTED = 4 };

typedef struct MY_STUFF
{
    GtkWidget *label;
    GtkWidget *image;
    PanelApplet *applet;        
    int flag;
    int last;
    char lbl[LSIZE];
    GtkTooltips *tips;
    char *ptr;
    size_t len;
    char *url;
    int timeout;
    CURL *c;
    char *pixdir;
    char *key;
    char *auth;
    guint tid;
    short noproxy;
} MY_STUFF_t;

static void display_about_dialog (BonoboUIComponent *uic, gpointer data,
                                  const gchar *verbname)
{
    static const gchar *authors[] = {
        "Jonathan Hudson <jh+w1retap@daria.co.uk>",
        NULL
    };
    
    gtk_show_about_dialog (NULL,
                           "program-name", "w1retap applet",
                           "title", _("About w1retap applet"),
                           "authors", authors,
                           "website", "http://www.daria.co.uk/wx",
                           "copyright", "(C) 2008 Jonathan Hudson",
                           "comments", "A libcurl/dbus based applet for w1retap",
                           NULL);
}

static const BonoboUIVerb example_menu_verbs [] = {
    BONOBO_UI_VERB ("w1tempAbout", display_about_dialog),
    BONOBO_UI_VERB_END
};

static const char Context_menu_xml [] =
"<popup name=\"button3\">\n"
"   <menuitem name=\"About Item\" "
"             verb=\"w1tempAbout\" "
"           _label=\"_About...\"\n"
"          pixtype=\"stock\" "
"          pixname=\"gnome-stock-about\"/>\n"
"</popup>\n";

static size_t  wfunc (void  *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t nb = size*nmemb;
    MY_STUFF_t *p = stream;
    if (nb > 0)
    {
        p->ptr = g_realloc(p->ptr, p->len+nb);
        if(p->ptr)
        {
            memcpy(p->ptr+p->len, ptr, nb);
        }
        p->len += nb;
    }
    return nb;
}


static void clean_curl(MY_STUFF_t *m)
{
    curl_easy_cleanup(m->c);
    curl_global_cleanup();
    m->c = NULL;
}

static void init_curl(MY_STUFF_t *m)
{
    curl_global_init(CURL_GLOBAL_NOTHING);
    atexit(curl_global_cleanup);
    if((m->c = curl_easy_init()) != NULL)
    {
        curl_easy_setopt(m->c, CURLOPT_WRITEFUNCTION, wfunc);
        curl_easy_setopt(m->c, CURLOPT_WRITEDATA, m);
        curl_easy_setopt(m->c, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(m->c, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(m->c, CURLOPT_CONNECTTIMEOUT, 30);
        curl_easy_setopt(m->c, CURLOPT_TIMEOUT, 90);
        
        if (m->noproxy)
        {
            curl_easy_setopt(m->c, CURLOPT_PROXY,"");
        }
        
        if(m->auth)
        {
            curl_easy_setopt(m->c, CURLOPT_USERPWD, m->auth);
            curl_easy_setopt(m->c, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
        }
    }
}

static gint timerfunc (gpointer data)
{
    MY_STUFF_t *m =  (MY_STUFF_t *)data;
    int td;

    m->len = 0;    
    m->ptr = NULL;
    
    if(m->c == NULL)
    {
        init_curl(m);
    }
    
    if(m->c)
    {
        char *turl = NULL;
        time_t t = time(NULL);
        asprintf(&turl, m->url, t);
        curl_easy_setopt(m->c, CURLOPT_URL, turl);        
        puts(turl);
        if(curl_easy_perform(m->c) == 0)
        {
            if(m->len && m->ptr)
            {
                double gtemp = -999;
                char *p;

                if(*(m->ptr+m->len-1) == '\n')
                {
                    *(m->ptr+m->len-1) = '\0';
                }
                else
                {
                    m->ptr = g_realloc(m->ptr, m->len+1);                    
                    *(m->ptr+m->len) = 0;
                }
                if(NULL != (p = strcasestr(m->ptr, m->key)))
                {
                    p += 1+strlen(m->key);
                    gtemp = strtod(p,NULL);
                }
                
                gtk_tooltips_set_tip (m->tips, GTK_WIDGET (m->applet), m->ptr, NULL);
                td = (int)(gtemp+0.5);
                if(td > 39) td = 39;
                if(td < -5) td = -5;
                snprintf(m->lbl, LSIZE, "%.1f°C", gtemp);
                
                if( m->last != td)
                {
                    char *pixnam;
                    char *fnam;
                    
                    asprintf (&pixnam, "thermo_%d.png", td);
                    fnam = g_build_filename (G_DIR_SEPARATOR_S,
                                             m->pixdir, pixnam, NULL);
                    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (fnam, NULL);
                    g_free(fnam);
                    free(pixnam);
                    gtk_image_set_from_pixbuf (GTK_IMAGE (m->image), pixbuf);
                    g_object_unref(pixbuf);
                    m->last = td;
                }
            }
        }
        else
        {
            clean_curl(m);
        }
        free(turl);
    }
    
    if(m->len == 0) 
    {
        strcpy(m->lbl, "?");
    }

    gtk_label_set_text (GTK_LABEL(m->label), m->lbl);
    g_free(m->ptr);
    m->tid = g_timeout_add_seconds (m->timeout, timerfunc, m);
    fprintf(stderr, "New tid %d %d\n", m->tid, m->timeout);
    return FALSE;
}

static void nm_state_cb (DBusGProxy *proxy, int state, gpointer user_data)
{
    MY_STUFF_t *m =  (MY_STUFF_t *)user_data;    
    g_print("Message received %d\n", state);
    if (state == NM_STATE_CONNECTED)
    {
        if(m->tid)
        {
            printf("remove tid %d\n", m->tid);            
            g_source_remove(m->tid);
        }
        printf("Call func\n");
        m->tid = g_timeout_add_seconds (1, timerfunc, m);
        printf("new tid %d\n", m->tid);        
    }
}


/* Verbatim from netspeed applet, thanks */
static void
change_background_cb(PanelApplet *applet_widget,
                     PanelAppletBackgroundType type,
                     GdkColor *color, GdkPixmap *pixmap,
                     MY_STUFF_t *t)
{
    GtkStyle *style;
    GtkRcStyle *rc_style = gtk_rc_style_new ();
    gtk_widget_set_style (GTK_WIDGET (applet_widget), NULL);
    gtk_widget_modify_style (GTK_WIDGET (applet_widget), rc_style);
    gtk_rc_style_unref (rc_style);
    switch (type)
    {
        case PANEL_PIXMAP_BACKGROUND:
            style = gtk_style_copy (GTK_WIDGET (applet_widget)->style);
            if(style->bg_pixmap[GTK_STATE_NORMAL])
                g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);
            style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
            gtk_widget_set_style (GTK_WIDGET(applet_widget), style);
            g_object_unref (style);
            break;
        case PANEL_COLOR_BACKGROUND:
            gtk_widget_modify_bg(GTK_WIDGET(applet_widget), GTK_STATE_NORMAL,
                                 color);
            break;
        case PANEL_NO_BACKGROUND:
            break;
    }
}

static gboolean w1temp_fill (
    PanelApplet *applet,
    const gchar *iid,
    gpointer data)
{

    if (strcmp (iid, "OAFIID:w1tempApplet") != 0)
        return FALSE;

    GError *error = NULL;
    DBusGProxy * obj = NULL;
    DBusGConnection * bus = NULL;

    static MY_STUFF_t m = {.timeout=120, .last=-999, .key="temperature",
                           .noproxy = 0, .tid=0};
    FILE *fp;
    char *p;

    g_type_init ();
    
    bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
    if (bus)
    {
        obj = dbus_g_proxy_new_for_name (bus, DBUS_SERVICE_NM, DBUS_PATH_NM,
                                         DBUS_INTERFACE_NM);
        if(obj)
        {
            dbus_g_proxy_add_signal(obj, "StateChanged", 
                                    G_TYPE_UINT, G_TYPE_INVALID);
            dbus_g_proxy_connect_signal(obj, "StateChanged",
                                        G_CALLBACK(nm_state_cb), &m, NULL);
        }
    }

    m.pixdir = GNOME_PIXMAPSDIR"/w1temp/";

    if((p = getenv("HOME")))
    {
        char fbuf[1024];
        char buf[256];
        strcpy(fbuf, p);
        strcat(fbuf,"/.config/w1retap/applet");
        if((fp = fopen(fbuf, "r")))
        {
                // We only read sizeof(buf).
            while(fgets(fbuf, sizeof(buf),fp))
            {
                if(1 == sscanf(fbuf, "key = %[^\n]", buf))
                {
                    m.key = g_strdup(buf);
                }
                else if(1 == sscanf(fbuf, "uauth = %[^\n]", buf))
                {
                    m.auth = g_strdup(buf);
                }
                else if(sscanf(fbuf, "url = %[^\n]", buf))
                {
                    m.url = g_strdup(buf);
                }
                else
                {
                    sscanf(fbuf, "delay = %d", &m.timeout);
                    sscanf(fbuf, "noproxy = %hd", &m.noproxy);
                }
            }
            fclose(fp);
        }
    }

    if(m.url == NULL)
    {
        m.url = "http://www-proxy/wx/wx_static.dat";
    }

    init_curl (&m);
    m.image = gtk_image_new ();

    char *pixfile = g_build_filename (G_DIR_SEPARATOR_S,
                                      m.pixdir, "thermo_0.png", NULL);
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (pixfile, NULL);
    g_free(pixfile);
    
    gtk_image_set_from_pixbuf (GTK_IMAGE (m.image), pixbuf);
    g_object_unref(pixbuf);
    m.label = gtk_label_new ("???");
    GtkWidget *hbox1 = gtk_hbox_new (FALSE, 0);
    m.tips = gtk_tooltips_new ();
    m.applet = applet;
    gtk_tooltips_set_tip (m.tips, GTK_WIDGET (applet),"no data", NULL);
    gtk_box_pack_start(GTK_BOX(hbox1), m.image,  TRUE, TRUE, 0);
    gtk_box_pack_end  (GTK_BOX(hbox1), m.label,  FALSE, FALSE, 0);    
    gtk_container_add (GTK_CONTAINER (applet), hbox1);    
    g_signal_connect(G_OBJECT(applet), "change_background",
                     G_CALLBACK(change_background_cb),
                     (gpointer)&m);

    panel_applet_setup_menu (PANEL_APPLET (applet),
                             Context_menu_xml,
                             example_menu_verbs,
                             applet);	

    gtk_widget_show_all (GTK_WIDGET (applet));
    timerfunc(&m);
    return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:w1tempApplet_Factory",
                             PANEL_TYPE_APPLET,
                             "The w1temp Applet",
                             "0",
                             w1temp_fill,
                             NULL);
