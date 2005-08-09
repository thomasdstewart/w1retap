pkglib_LTLIBRARIES = libw1mysql.la
AM_CPPFLAGS += -D_GNU_SOURCE=1 -I  ../libusblinux300/ -I../ @MYSQL_CFLAGS@ $(GLIB_CFLAGS)
libw1mysql_la_SOURCES = w1mysql.c
libw1mysql_la_LIBADD =  @MYSQL_LIBS@

