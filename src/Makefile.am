SUBDIRS = $(USE_SQLITE) $(USE_MYSQL) $(USE_PGSQL) $(USE_ODBC) libusblinux300
DIST_SUBDIRS = sqlite mysql pgsql odbc libusblinux300
EXTRA_DIST = w1retap.h sensors.sql

bin_PROGRAMS=w1retap

pkglib_LTLIBRARIES = libw1file.la  libw1xml.la  libw1csv.la 

w1retap_SOURCES = w1retap.c w1conf.c w1util.c w1sensors.c
AM_CPPFLAGS = -Wall -D_GNU_SOURCE=1 -I libusblinux300 -DMODULE_DIR=\"$(pkglibdir)\" $(GLIB_CFLAGS)
w1retap_DEPENDENCIES = libw1file.la libusblinux300/libowfat.a
w1retap_LDADD = "-dlopen" libw1file.la  -L./libusblinux300/.libs -L./libusblinux300 -lowfat -lw1common -lm
w1retap_LDFLAGS = -rdynamic  $(GLIB_LIBS) 

libw1file_la_SOURCES = w1file.c
libw1file_la_LDFLAGS = -module $(GLIB_LIBS) 

libw1xml_la_SOURCES = w1xml.c
libw1xml_la_LDFLAGS = -module $(GLIB_LIBS) 

libw1csv_la_SOURCES = w1csv.c
libw1csv_la_LDFLAGS = -module $(GLIB_LIBS) 

install-exec-hook:
	if test -n "$(STRIP_LIBS)" ; then \
	 strip --strip-unneeded $(pkglibdir)/lib*.so.0.0.0 ; \
	fi		
