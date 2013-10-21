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
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <libsoup/soup.h>
#include "config.h"

#define DBUS_SERVICE_NM      "org.freedesktop.NetworkManager"
#define DBUS_PATH_NM         "/org/freedesktop/NetworkManager"
#define DBUS_INTERFACE_NM    "org.freedesktop.NetworkManager"

#define INTERVAL 120
#define LSIZE 256
#define MAXSENS 20

static gint timerfunc (gpointer);

enum NM_STATE {NM_STATE_UNKNOWN=0, NM_STATE_ASLEEP = 1, NM_STATE_CONNECTING = 2,
               NM_STATE_CONNECTED = 3, NM_STATE_DISCONNECTED = 4 };

typedef struct MY_STUFF
{
    GtkWidget *label;
    GtkWidget *image;
    PanelApplet *applet;        
    int flag;
    int last;
    char *url;
    char lbl[256];
    int timeout;
    char *pixdir;
    char *key;
    char *auth;
    guint tid;
    SoupSession sess;
} MY_STUFF_t;

#define W1_LABEL_COLOUR "white"

static void mlabel_set_text(MY_STUFF_t * m, char *str)
{
        /* One day theming will be easy again ...... */
#ifdef W1_LABEL_COLOUR
    char *markup;
    markup = g_markup_printf_escaped ("<span color=\"" W1_LABEL_COLOUR "\">%s</span>", str);
    gtk_label_set_markup (GTK_LABEL (m->label), markup);
   g_free (markup);
#else
    gtk_label_set_text (GTK_LABEL(m->label), str);
#endif
}

static void display_about_dialog (GtkAction *action, gpointer data)
{
    static const gchar *authors[] = {
        "Jonathan Hudson <jh+w1retap@daria.co.uk>",
        NULL
    };
    
    gtk_show_about_dialog (NULL,
                           "program-name", "w1retap applet",
                           "title", "About w1retap applet",
                           "authors", authors,
                           "website", "http://www.daria.co.uk/wx",
                           "copyright", "(C) 2008-2011 Jonathan Hudson",
                           "comments", "A soup/dbus based applet for w1retap",
                           NULL);
}


static const GtkActionEntry menu_actions [] = {
  { "w1tempAbout", GTK_STOCK_ABOUT, "_About",
          NULL, NULL, G_CALLBACK (display_about_dialog) }
};

static const char menu_xml [] =
  "<menuitem name=\"About Item\" action=\"w1tempAbout\"/>\n" ;

static void soup_done (SoupSession *sess, SoupMessage *msg, gpointer data)
{
    MY_STUFF_t *m =  (MY_STUFF_t *)data;
    int td;

    if(msg->status_code == SOUP_STATUS_OK)
    {
        double gtemp = -999;
        char *p;

        p =  (char*)msg->response_body->data + msg->response_body->length -1;
        if (*p == '\n')
            *p = 0;

        if(NULL != (p = strcasestr(msg->response_body->data, m->key)))
        {
            p += 1+strlen(m->key);
            gtemp = strtod(p,NULL);
        }
                
        gtk_widget_set_tooltip_text(GTK_WIDGET (m->applet),
                                    msg->response_body->data);
        td = (int)(gtemp+0.5);
        if(td > 39) td = 39;
        if(td < -5) td = -5;
        snprintf(m->lbl, LSIZE, "%.1f°C", gtemp);
                
        if( m->last != td)
        {
            char *pixnam;
            char *fnam;
                    
            asprintf (&pixnam, "w1_thermo_%04d.png", td);
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
    else 
    {
        strcpy(m->lbl, "?");
    }

    mlabel_set_text (m, m->lbl);
    m->tid = g_timeout_add_seconds (m->timeout, timerfunc, m);
    fprintf(stderr, "New tid %d %d\n", m->tid, m->timeout);
}

static gint timerfunc (gpointer data)
{
    SoupSession *session;
    SoupMessage *message;
    session = soup_session_async_new();
    message = soup_message_new("GET",((MY_STUFF_t *)data)->url);
    soup_session_queue_message(session, message, soup_done, data);
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


static gboolean w1temp_fill (PanelApplet *applet, const gchar *iid,
                             gpointer data)
{

    if (strcmp (iid, "w1tempApplet") != 0)
        return FALSE;

    GError *error = NULL;
    DBusGProxy * obj = NULL;
    DBusGConnection * bus = NULL;

    static MY_STUFF_t m = {.timeout=120, .last=-999, .key="temperature",
                           .tid=0};
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

    m.pixdir = "/usr/share/w1temp/pixmaps";

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
                }
            }
            fclose(fp);
        }
    }

    if(m.url == NULL)
    {
        m.url = "http://www-proxy/wx/wx_static.dat";
    }

    m.image = gtk_image_new ();

    char *pixfile = g_build_filename (G_DIR_SEPARATOR_S,
                                      m.pixdir, "w1_thermo_0000.png", NULL);
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (pixfile, NULL);
    g_free(pixfile);
    
    gtk_image_set_from_pixbuf (GTK_IMAGE (m.image), pixbuf);
    g_object_unref(pixbuf);
    m.label = gtk_label_new (NULL);
    mlabel_set_text(&m, "???");

    GtkWidget *hbox1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,0);
    m.applet = applet;
    gtk_widget_set_tooltip_text(GTK_WIDGET (applet),"no data");
    gtk_box_pack_start(GTK_BOX(hbox1), m.image,  TRUE, TRUE, 0);
    gtk_box_pack_end  (GTK_BOX(hbox1), m.label,  FALSE, FALSE, 0);    
    gtk_container_add (GTK_CONTAINER (applet), hbox1);    

    {
       GtkActionGroup *action_group;
       action_group = gtk_action_group_new ("w1temp Applet Actions");
       gtk_action_group_add_actions (action_group,
				     menu_actions,
				     G_N_ELEMENTS (menu_actions),
				     applet);
       panel_applet_setup_menu(applet, menu_xml, action_group);
       g_object_unref (action_group);
    }
    panel_applet_set_background_widget(applet, GTK_WIDGET (applet));
    gtk_widget_show_all (GTK_WIDGET (applet));
    timerfunc(&m);
    return TRUE;
}

PANEL_APPLET_OUT_PROCESS_FACTORY ("w1tempAppletFactory",
                             PANEL_TYPE_APPLET,
                             w1temp_fill,
                             NULL);
