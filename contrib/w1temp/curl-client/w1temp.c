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
#include <expat.h>

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

static void xml_start(void *data, const char *el, const char **attr)
{
    int i;
    MY_STUFF_t *m = data;

    if(0 == strcasecmp(el, "report"))
    {
        for (i = 0; attr[i]; i += 2)
        {
            if(0 == strcmp(attr[i], "timestamp"))
            {
                m->time = g_strdup(attr[i+1]);
                break;
            }
        }
    }
    else if(0 == strcasecmp(el, "sensor"))
    {
        wdata_t *w = m->w + m->nvals;        
        
        for (i = 0; attr[i]; i += 2)
        {
            if(0 == strcmp(attr[i], "name"))
            {
                w->key = g_strdup (attr[i+1]);
                *w->key = toupper(*w->key);                
                continue;
            }
            if(0 == strcmp(attr[i], "value"))
            {
                w->value = strtod(attr[i+1], NULL);
                continue;
            }
            if(0 == strcmp(attr[i], "units"))
            {
                w->units = g_strdup(attr[i+1]);
                continue;
            }
        }
        m->nvals++;
    }
}   

static void xml_end(void *data, const char *el) {}

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

static gint timerfunc (gpointer data)
{
    MY_STUFF_t *m =  (MY_STUFF_t *)data;
    int td;
    int n = -1;

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
            if(m->len && m->ptr)
            {
                XML_Parser p = XML_ParserCreate(NULL);
                XML_SetElementHandler(p, xml_start, xml_end);
                XML_SetUserData(p, m);
                XML_Parse(p, m->ptr, m->len, 1);
                XML_ParserFree(p);
                
                char tt[1024];
                double gtemp = -999;
                int nc = 0;
                wdata_t *w = m->w;;
                int j;
            
                for(j = 0; j < m->nvals; j++, w++)
                {
                    if(w->units)
                    {
                        if ((0 == strcasecmp(w->key, m->key)))
                        {
                            gtemp = w->value;
                            n = 0;
                        }
                        nc+=sprintf(tt+nc,"%s: %.2f %s\n", w->key, w->value, w->units);
                        g_free(w->units);
                        w->units = NULL;
                        g_free(w->key);
                        w->key = NULL;
                    }
                }
                
                if(m->time)
                {
                    strcpy(tt+nc, m->time);
                }
                gtk_tooltips_set_tip (m->tips, GTK_WIDGET (m->applet), tt, NULL);
            
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
                    g_free(pixnam);
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
    }
    
    if(n == -1) 
    {
        strcpy(m->lbl, "?");
    }

    gtk_label_set_text (GTK_LABEL(m->label), m->lbl);
    g_free(m->ptr);
    g_free(m->time);
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
        strcpy(fbuf, p);
        strcat(fbuf,"/.config/w1retap/applet");
        if((fp = fopen(fbuf, "r")))
        {
            while(fgets(fbuf, sizeof(fbuf),fp))
            {
                sscanf(fbuf, "url = %a[^\n]\n", &m.url);
                sscanf(fbuf, "key = %a[^\n]\n", &m.key);
                sscanf(fbuf, "auth = %a[^\n]\n", &m.auth);
                sscanf(fbuf, "delay = %d", &m.timeout);
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
