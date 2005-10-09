# Makefile.in generated automatically by automake 1.4-p6 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999, 2001 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ../..

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/w1retap
pkglibdir = $(libdir)/w1retap
pkgincludedir = $(includedir)/w1retap

top_builddir = ../..

ACLOCAL = aclocal-1.4
AUTOCONF = autoconf
AUTOMAKE = automake-1.4
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = 
host_triplet = i686-pc-linux-gnu
AR = ar
AS = @AS@
CC = gcc
CURL_CFLAGS = 
CURL_LIBS = -L/usr/lib -lcurl -L/usr/lib -lgnutls -lgcrypt -lgpg-error -lz -lidn -ldl -lz
CXX = g++
CXXCPP = g++ -E
DLLTOOL = @DLLTOOL@
ECHO = echo
EGREP = grep -E
EXEEXT = 
F77 = g77
GCJ = @GCJ@
GCJFLAGS = @GCJFLAGS@
GLIB_CFLAGS = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include  
GLIB_LIBS = -Wl,--export-dynamic -lgmodule-2.0 -ldl -lglib-2.0  
GNOME_APPLETS_CFLAGS = -DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API -DORBIT2=1 -pthread -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/pango-1.0 -I/usr/include/freetype2 -I/usr/include/freetype2/config -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/panel-2.0 -I/usr/include/libgnomeui-2.0 -I/usr/include/libbonoboui-2.0 -I/usr/include/libgnome-2.0 -I/usr/include/libgnomecanvas-2.0 -I/usr/include/libart-2.0 -I/usr/include/gconf/2 -I/usr/include/orbit-2.0 -I/usr/include/libbonobo-2.0 -I/usr/include/gnome-vfs-2.0 -I/usr/lib/gnome-vfs-2.0/include -I/usr/include/bonobo-activation-2.0 -I/usr/include/libxml2  
GNOME_APPLETS_LIBS = -Wl,--export-dynamic -pthread -lpanel-applet-2 -lgnomeui-2 -lSM -lICE -lbonoboui-2 -lxml2 -lz -lgnomecanvas-2 -lgnome-2 -lpopt -lart_lgpl_2 -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lpangoxft-1.0 -lpangox-1.0 -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lgnomevfs-2 -lbonobo-2 -lgconf-2 -lbonobo-activation -lORBit-2 -lm -lgmodule-2.0 -ldl -lgthread-2.0 -lglib-2.0  
HAVE_LIB = @HAVE_LIB@
LIB = @LIB@
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
LTLIB = @LTLIB@
MAKEINFO = makeinfo
MYSQL_CFLAGS = 
MYSQL_CONFIG = no
MYSQL_LIBS = 
OBJDUMP = @OBJDUMP@
OBJEXT = o
PACKAGE = w1retap
PGSQL_CFLAGS = /usr/include -DPGV=8
PGSQL_LIBS = /usr/lib
PG_CONFIG = /usr/bin/pg_config
PKG_CONFIG = /usr/bin/pkg-config
RANLIB = ranlib
RC = @RC@
STRIP = strip
USE_APPLET = w1temp
USE_MYSQL = 
USE_ODBC = odbc
USE_PGSQL = pgsql
USE_RPC = 
USE_RPCCGI = 
USE_SQLITE = sqlite
VERSION = 0.0.4
XMLRPCCGI_CFLAGS = 
XMLRPCCGI_LIBS = 
XMLRPC_CFLAGS = 
XMLRPC_LIBS = 

libexec_PROGRAMS = w1temp
INCLUDES = $(GNOME_APPLETS_CFLAGS) $(CURL_FLAGS) -DGNOME_DISABLE_DEPRECATED

w1temp_SOURCES = w1temp.c
w1temp_LDADD = $(GNOME_APPLETS_LIBS) $(CURL_LIBS) -lexpat

serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_W1tempApplet.server.in
server_DATA = $(server_in_files:.server.in=.server)

PIXIES = $(wildcard *.png)

pixmapdir = $(datadir)/pixmaps/w1temp/
pixmap_DATA = $(PIXIES)

icon_DATA = w1temp.png
icondir = $(datadir)/pixmaps

CLEANFILES = $(server_DATA) *.bak

EXTRA_DIST = $(server_in_files) *.png README COPYING
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../../config.h
CONFIG_CLEAN_FILES = 
libexec_PROGRAMS =  w1temp$(EXEEXT)
PROGRAMS =  $(libexec_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I../..
CPPFLAGS = 
LDFLAGS = 
LIBS = -lm 
w1temp_OBJECTS =  w1temp.$(OBJEXT)
w1temp_DEPENDENCIES = 
w1temp_LDFLAGS = 
CFLAGS = -g -O2
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
DATA =  $(icon_DATA) $(pixmap_DATA) $(server_DATA)

DIST_COMMON =  README COPYING Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
DEP_FILES =  .deps/w1temp.P
SOURCES = $(w1temp_SOURCES)
OBJECTS = $(w1temp_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .lo .o .obj .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.ac $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu contrib/w1temp/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-libexecPROGRAMS:

clean-libexecPROGRAMS:
	-test -z "$(libexec_PROGRAMS)" || rm -f $(libexec_PROGRAMS)

distclean-libexecPROGRAMS:

maintainer-clean-libexecPROGRAMS:

install-libexecPROGRAMS: $(libexec_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libexecdir)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-libexecPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

# FIXME: We should only use cygpath when building on Windows,
# and only if it is available.
.c.obj:
	$(COMPILE) -c `cygpath -w $<`

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core
	-rm -f *.$(OBJEXT)

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.s.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.S.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:

maintainer-clean-libtool:

w1temp$(EXEEXT): $(w1temp_OBJECTS) $(w1temp_DEPENDENCIES)
	@rm -f w1temp$(EXEEXT)
	$(LINK) $(w1temp_LDFLAGS) $(w1temp_OBJECTS) $(w1temp_LDADD) $(LIBS)

install-iconDATA: $(icon_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(icondir)
	@list='$(icon_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(icondir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(icondir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(icondir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(icondir)/$$p; \
	  fi; fi; \
	done

uninstall-iconDATA:
	@$(NORMAL_UNINSTALL)
	list='$(icon_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(icondir)/$$p; \
	done

install-pixmapDATA: $(pixmap_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pixmapdir)
	@list='$(pixmap_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(pixmapdir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(pixmapdir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pixmapdir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(pixmapdir)/$$p; \
	  fi; fi; \
	done

uninstall-pixmapDATA:
	@$(NORMAL_UNINSTALL)
	list='$(pixmap_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(pixmapdir)/$$p; \
	done

install-serverDATA: $(server_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(serverdir)
	@list='$(server_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(serverdir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(serverdir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(serverdir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(serverdir)/$$p; \
	  fi; fi; \
	done

uninstall-serverDATA:
	@$(NORMAL_UNINSTALL)
	list='$(server_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(serverdir)/$$p; \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = contrib/w1temp

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu contrib/w1temp/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:
	-rm -rf .deps

maintainer-clean-depend:

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) -Wp,-MD,.deps/$(*F).pp -c $<
	@-cp .deps/$(*F).pp .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm .deps/$(*F).pp

%.lo: %.c
	@echo '$(LTCOMPILE) -c $<'; \
	$(LTCOMPILE) -Wp,-MD,.deps/$(*F).pp -c $<
	@-sed -e 's/^\([^:]*\)\.o[ 	]*:/\1.lo \1.o :/' \
	  < .deps/$(*F).pp > .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm -f .deps/$(*F).pp
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am: install-libexecPROGRAMS
install-exec: install-exec-am

install-data-am: install-iconDATA install-pixmapDATA install-serverDATA
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-libexecPROGRAMS uninstall-iconDATA \
		uninstall-pixmapDATA uninstall-serverDATA
uninstall: uninstall-am
all-am: Makefile $(PROGRAMS) $(DATA)
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(libexecdir) $(DESTDIR)$(icondir) \
		$(DESTDIR)$(pixmapdir) $(DESTDIR)$(serverdir)


mostlyclean-generic:

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-libexecPROGRAMS mostlyclean-compile \
		mostlyclean-libtool mostlyclean-tags mostlyclean-depend \
		mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-libexecPROGRAMS clean-compile clean-libtool clean-tags \
		clean-depend clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-libexecPROGRAMS distclean-compile \
		distclean-libtool distclean-tags distclean-depend \
		distclean-generic clean-am
	-rm -f libtool

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-libexecPROGRAMS \
		maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-tags maintainer-clean-depend \
		maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: mostlyclean-libexecPROGRAMS distclean-libexecPROGRAMS \
clean-libexecPROGRAMS maintainer-clean-libexecPROGRAMS \
uninstall-libexecPROGRAMS install-libexecPROGRAMS mostlyclean-compile \
distclean-compile clean-compile maintainer-clean-compile \
mostlyclean-libtool distclean-libtool clean-libtool \
maintainer-clean-libtool uninstall-iconDATA install-iconDATA \
uninstall-pixmapDATA install-pixmapDATA uninstall-serverDATA \
install-serverDATA tags mostlyclean-tags distclean-tags clean-tags \
maintainer-clean-tags distdir mostlyclean-depend distclean-depend \
clean-depend maintainer-clean-depend info-am info dvi-am dvi check \
check-am installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


$(server_DATA): $(server_in_files)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
