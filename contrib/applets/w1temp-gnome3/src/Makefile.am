## Process this file with automake to produce Makefile.in

libexec_PROGRAMS = w1temp
INCLUDES = $(GLIB_MODULE_CFLAGS) $(GNOME_APPLETS_CFLAGS) $(LIBSOUP_CFLAGS) -DGNOME_DISABLE_DEPRECATED -Wall

w1temp_SOURCES = w1temp.c
w1temp_LDADD = $(GLIB_MODULE_LIBS) $(GNOME_APPLETS_LIBS) $(LIBSOUP_LIBS)

appletdir       = $(LIBPANEL_APPLET_DIR)
applet_in_files = org.gnome.panel.w1tempApplet.panel-applet.in
applet_DATA     = $(applet_in_files:.panel-applet.in=.panel-applet)
$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@LOCATION\@|$(libexecdir)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

%.panel-applet: %.panel-applet.in 
	cp $< $@


servicedir          = $(datadir)/dbus-1/services
service_in_files    = org.gnome.panel.applet.w1tempAppletFactory.service.in
service_DATA        = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files)
	sed -e "s|\@LOCATION\@|$(libexecdir)|" $< > $@

PIXIES = $(wildcard ../../share/w1temp/pixmaps/*.png)

pixmapdir = $(prefix)/share/w1temp/pixmaps
pixmap_DATA = $(PIXIES)

icon_DATA = w1temp.png
icondir = $(datadir)/pixmaps

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) 

EXTRA_DIST =  *.png README COPYING \
	$(service_in_files) org.gnome.panel.w1temp.panel-applet.in.in
