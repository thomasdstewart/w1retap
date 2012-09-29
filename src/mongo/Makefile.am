EXTRA_DIST = mktest.sh

pkglib_LTLIBRARIES = libw1mongo.la
AM_CPPFLAGS = -Wall -D_GNU_SOURCE=1 -I  ../libusblinux300/ -I../ $(GLIB_CFLAGS) $(MONGO_CFLAGS)
libw1mongo_la_SOURCES = w1mongo.c
libw1mongo_la_LIBADD = $(MONGO_LIBS)
