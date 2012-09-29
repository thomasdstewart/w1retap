EXTRA_DIST = mktest.sh

pkglib_LTLIBRARIES = libw1sqlite.la
AM_CPPFLAGS = -Wall -D_GNU_SOURCE=1 -I  ../libusblinux300/ -I../ $(GLIB_CFLAGS) $(SQLITE_CFLAGS)
libw1sqlite_la_SOURCES = w1sqlite.c
libw1sqlite_la_LIBADD = $(SQLITE_LIBS)
