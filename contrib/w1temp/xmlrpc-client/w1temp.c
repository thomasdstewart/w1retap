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
#include "config.h"

#define INTERVAL (120*1000)
#define LSIZE 256
#define MAXSENS 20

#include "w1temp-rpc.h"

typedef struct MY_STUFF
{
    GtkWidget *label;
    GtkWidget *image;
    PanelApplet *applet;        
    int flag;
    int last;
    char lbl[LSIZE];
    GtkTooltips *tips;
    char *url;
    char *time;
    int timeout;
    char *pixdir;
    W1RPC_t x;
} MY_STUFF_t;

static gint timerfunc (gpointer data)
{
    MY_STUFF_t *m =  (MY_STUFF_t *)data;
    int td;
    char tt[1024];
    double gtemp;    

    if(0 == get_xmlrpc_wx(&m->x, tt, &gtemp))
    {
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
            if(pixbuf)
            {
                gtk_image_set_from_pixbuf (GTK_IMAGE (m->image), pixbuf);
                g_object_unref(pixbuf);
                m->last = td;
            }
            g_free(fnam);
            g_free(pixnam);
        }
        free(tt);
    }
    else
    {
        strcpy(m->lbl, "?");
    }

    gtk_label_set_text (GTK_LABEL(m->label), m->lbl);
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

    static MY_STUFF_t m = {.timeout=120, .last=-999,
                           .x.key="temperature"};
    FILE *fp;
    char *p;

    m.pixdir = GNOME_PIXMAPSDIR"/w1temp/";

    if((p = getenv("HOME")))
    {
        char fbuf[PATH_MAX];
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
                else if(sscanf(fbuf, "host = %[^\n]", buf))
                {
                    m.host = g_strdup(buf);
                }
                else
                {
                    (void)(sscanf(fbuf, "delay = %d", &m.timeout) ||
                           sscanf(fbuf, "port = %hd", &m.port));
                }
            }
            fclose(fp);
        }
    }

    if(m.x.url == NULL)
    {
        return FALSE;
    }

    m.timeout *= 1000;
    m.image = gtk_image_new ();

    char *pixfile = g_build_filename (G_DIR_SEPARATOR_S,
                                      m.pixdir, "thermo_0.png", NULL);
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (pixfile, NULL);
    gtk_image_set_from_pixbuf (GTK_IMAGE (m.image), pixbuf);
    g_free(pixfile);
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
