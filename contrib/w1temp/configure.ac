#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(configure.ac)
PACKAGE=w1temp
VERSION=0.0.5
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_ARG_WITH(xmlrpc, AC_HELP_STRING([--with-xmlrpc], [Force sppport for XMLRPC client]))
AC_ARG_WITH(curl, AC_HELP_STRING([--with-curl], [Force sppport for cURL client]))
AC_ARG_WITH(gio, AC_HELP_STRING([--with-gio], [Force sppport for GVFS client]))

# Checks for libraries.
AC_CHECK_LIB([curl], [curl_easy_init])
AC_CHECK_LIB([expat], [XML_ParserCreate])
AC_CHECK_LIB([xmlrpc], [xmlrpc_array_new],,,[-lexpat])
# But don't set fscking LIBS ....
LIBS=
AC_CHECK_LIB([m], [sin], [])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h memory.h stdlib.h string.h sys/file.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_CHECK_FUNCS([alarm atexit gettimeofday memset sqrt stpcpy strcasecmp strchr strdup strncasecmp strpbrk strrchr strstr strtol])

USE_APPLET=
GLIB_CFLAGS=
GLIB_LIBS=
GIO_CFLAGS=
GIO_LIBS=
DBUS_REQUIRED=0.60

AC_PATH_PROG(PKG_CONFIG, pkg-config,no)

if test "$PKG_CONFIG" = "no" ; then
 AC_MSG_ERROR([Sorry, we need pkg-config])
fi

ACLOCAL_AMFLAGS="\${ACLOCAL_FLAGS}"
AC_SUBST(ACLOCAL_AMFLAGS)

pixmapsdir="${datadir}/pixmaps"
AC_SUBST(pixmapsdir)
AC_DEFINE_DIR(GNOME_PIXMAPSDIR, "${pixmapsdir}", [Gnome Pixmaps Directory])

PKG_CHECK_MODULES(GLIB_MODULE, gmodule-2.0 >= 2.2.0 
			        dbus-1 >= $DBUS_REQUIRED 
				dbus-glib-1 >= $DBUS_REQUIRED)
PKG_CHECK_MODULES(GIO_MODULE, gio-2.0 >= 2.16.0,,false)

GTK_REQUIRED=2.2.0
LIBPANEL_REQUIRED=2.6.0
PKG_CHECK_MODULES(GNOME_APPLETS, gtk+-2.0 >= $GTK_REQUIRED libpanelapplet-2.0 >= $LIBPANEL_REQUIRED,,false)
if test -z "$GNOME_APPLETS_PKG_ERRORS" ; then
 USE_APPLET=udp-client
 if test "$ac_cv_lib_xmlrpc_xmlrpc_array_new" = "yes" -a "$with_xmlrpc" = "yes"
 then	
  USE_RPCAPP=xmlrpc-client
  USE_RPCCGI=rpc-server
  XMLRPC_CFLAGS="$(xmlrpc-c-config client --cflags)"
  XMLRPC_LIBS="$(xmlrpc-c-config client --libs)"
  XMLRPCCGI_CFLAGS="$(xmlrpc-c-config cgi-server --cflags)"
  XMLRPCCGI_LIBS="$(xmlrpc-c-config cgi-server --libs)"
  AC_SUBST(XMLRPC_CFLAGS)
  AC_SUBST(XMLRPC_LIBS)
  AC_SUBST(XMLRPCCGI_CFLAGS)
  AC_SUBST(XMLRPCCGI_LIBS)
 fi	

 if test "x$with_curl" = "xyes" -a "x$ac_cv_lib_curl_curl_easy_init" = "xyes"
 then
  USE_CURLAPP=curl-client
  CURL_CFLAGS="$(curl-config --cflags)"
  CURL_LIBS="$(curl-config --libs)"
  AC_SUBST(CURL_CFLAGS)
  AC_SUBST(CURL_LIBS)
 fi

 if test -z "$GIO_PKG_ERRORS" -a "x$with_gio" = "xyes"
 then
  USE_GIOAPP=gvfs-client
  AC_SUBST(GIO_MODULE_CFLAGS)
  AC_SUBST(GIO_MODULE_LIBS)
 fi
fi

AC_MSG_CHECKING(for sunny days)

CFLAGS=-Werror
GLIB_CFLAGS="$GLIB_MODULE_CFLAGS"
GLIB_LIBS="$GLIB_MODULE_LIBS"
AC_SUBST(USE_APPLET)
AC_SUBST(USE_RPCAPP)
AC_SUBST(USE_CURLAPP)
AC_SUBST(USE_RPCCGI)
AC_SUBST(USE_GIOAPP)
AC_SUBST(GNOME_APPLETS_CFLAGS)
AC_SUBST(GNOME_APPLETS_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_MSG_RESULT(perhaps)

AC_OUTPUT(
Makefile
udp-client/Makefile
curl-client/Makefile
xmlrpc-client/Makefile
rpc-server/Makefile
gvfs-client/Makefile
)

