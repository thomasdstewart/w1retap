## Process this file with automake to produce Makefile.in

libexec_PROGRAMS = w1temp
INCLUDES = $(GLIB_MODULE_CFLAGS) $(GNOME_APPLETS_CFLAGS) $(CURL_FLAGS) -DGNOME_DISABLE_DEPRECATED -Wall

w1temp_SOURCES = w1temp.c
w1temp_LDADD = $(GLIB_MODULE_LIBS) $(GNOME_APPLETS_LIBS) $(CURL_LIBS) -lexpat

serverdir          = $(libdir)/bonobo/servers
server_in_files    = GNOME_W1tempApplet.server.in
server_DATA        = $(server_in_files:.server.in=.server)

$(server_DATA): $(server_in_files)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

PIXIES = $(wildcard *.png)

pixmapdir = $(datadir)/pixmaps/w1temp/
pixmap_DATA = $(PIXIES)

icon_DATA = w1temp.png
icondir = $(datadir)/pixmaps

CLEANFILES = $(server_DATA) *.bak

EXTRA_DIST = $(server_in_files) *.png README COPYING
