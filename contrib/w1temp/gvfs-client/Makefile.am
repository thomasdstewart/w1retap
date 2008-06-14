## Process this file with automake to produce Makefile.in

libexec_PROGRAMS = w1temp

w1temp_CFLAGS = -Wall $(GNOME_APPLETS_CFLAGS) -DGNOME_DISABLE_DEPRECATED $(AM_CFLAGS) $(GIO_MODULE_CFLAGS)  -D _GNU_SOURCE=1

w1temp_SOURCES = w1temp.c
w1temp_LDADD =  $(GNOME_APPLETS_LIBS) $(GIO_MODULE_LIBS)

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

EXTRA_DIST = $(server_in_files) *.png README
