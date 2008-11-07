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

#define INTERVAL (120*1000)
#define LSIZE 256
#define MAXSENS 20

typedef struct
{
    char *key;
    double value;
    char *units;
} wdata_t;

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
    char *time;
    int nvals;
    wdata_t *w;
    int timeout;
    CURL *c;
    char *key;
    char *auth;
    char *pixdir;
} MY_STUFF_t;


static size_t  wfunc (void  *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t nb = size*nmemb;
    MY_STUFF_t *p = stream;
    p->ptr = g_realloc(p->ptr, p->len+nb);
    if(p->ptr)
    {
        memcpy(p->ptr+p->len, ptr, nb);
    }
    p->len += nb;
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
        curl_easy_setopt(m->c, CURLOPT_URL, m->url);
        curl_easy_setopt(m->c, CURLOPT_WRITEFUNCTION, wfunc);
        curl_easy_setopt(m->c, CURLOPT_WRITEDATA, m);
        curl_easy_setopt(m->c, CURLOPT_FOLLOWLOCATION, 1);
        if(m->auth)
        {
            curl_easy_setopt(m->c, CURLOPT_USERPWD, m->auth);
            curl_easy_setopt(m->c, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
        }
    }
}

void update_display(MY_STUFF_t *m)
{
    char *tt = m->ptr;
    
    double gtemp = -999;
    char *p;
    int len;


    if((len = m->len) > 0)
    {
        if(*(tt+len-1) == '\n')
        {
            *(tt+len-1) = '\0';
        }
        else
        {
            *(tt+len) = 0;
        }
        
        if(NULL != (p = strcasestr(tt, m->key)))
        {
            p += 1+strlen(m->key);
            gtemp = strtod(p,NULL);
        }
        
        gtk_tooltips_set_tip (m->tips, GTK_WIDGET (m->applet), tt, NULL);
        
        int td = (int)(gtemp+0.5);
        if(td > 39) td = 39;
        if(td < -5) td = -5;
        if(gtemp == -999.0)
        {
            strcpy(m->lbl, "??");
        }
        else
        {
            snprintf(m->lbl, LSIZE, "%.1f°C", gtemp);
        }
        
        if( m->last != td)
        {
            char *pixnam;
            char *fnam;
            
            asprintf (&pixnam, "thermo_%d.png", td);
            fnam = g_build_filename (G_DIR_SEPARATOR_S,
                                     m->pixdir, pixnam, NULL);
            GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (fnam, NULL);
            g_free(fnam);
            g_free(pixnam);
            gtk_image_set_from_pixbuf (GTK_IMAGE (m->image), pixbuf);
            g_object_unref(pixbuf);
            m->last = td;
        }
    }
    else 
    {
        strcpy(m->lbl, "?");
    }
    gtk_label_set_text (GTK_LABEL(m->label), m->lbl);            
}


static gint timerfunc (gpointer data)
{
    MY_STUFF_t *m =  (MY_STUFF_t *)data;

    m->len = 0;    
    m->nvals = 0;

    if(m->c == NULL)
    {
        init_curl(m);
    }
    
    if(m->c)
    {
        if(curl_easy_perform(m->c) == 0)
        {
            if(m->ptr)
            {
                update_display(m);
            }
        }
        else
        {
            clean_curl(m);
        }
    }
    
    g_free(m->ptr);
    m->ptr = NULL;
    g_timeout_add (m->timeout, timerfunc, m);
    return FALSE;
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

    static wdata_t wdata[MAXSENS];
    static MY_STUFF_t m = {.timeout=120, .last=-999,
                           .w=wdata,.key="temperature"};
    FILE *fp;
    char *p;

    m.pixdir = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP,
                        "w1temp/", FALSE, NULL);

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
                else if(sscanf(fbuf, "gurl = %[^\n]", buf))
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
        m.url = "http://www-proxy/wx/wx_static.xml";
    }

    m.timeout *= 1000;
    
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
