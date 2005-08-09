pkglib_LTLIBRARIES = libw1pgsql.la
AM_CPPFLAGS += -D_GNU_SOURCE=1 -I  ../libusblinux300/ -I../ -I @PGSQL_CFLAGS@ $(GLIB_CFLAGS)
libw1pgsql_la_SOURCES = w1pgsql.c
libw1pgsql_la_LIBADD = @PGSQL_LIBS@
