#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(configure.ac)
PACKAGE=w1temp
VERSION=0.0.5
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_ARG_WITH(xmlrpc, AC_HELP_STRING([--with-xmlrpc], [Force sppport for XMLRPC client]))
AC_ARG_WITH(xmlrpc, AC_HELP_STRING([--with-curl], [Force sppport for cURL client]))


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

AC_PATH_PROG(PKG_CONFIG, pkg-config,no)

if test "$PKG_CONFIG" = "no" ; then
 AC_MSG_ERROR([Sorry, we need pkg-config])
fi

PKG_CHECK_MODULES(GLIB_MODULE, gmodule-2.0 >= 2.2.0)
#AC_MSG_RESULT(OK)

dnl need expat for starters for the applet
if test "$ac_cv_lib_expat_XML_ParserCreate" = "yes"
then
 GTK_REQUIRED=2.2.0
 LIBPANEL_REQUIRED=2.6.0
 PKG_CHECK_MODULES(GNOME_APPLETS, gtk+-2.0 >= $GTK_REQUIRED libpanelapplet-2.0 >= $LIBPANEL_REQUIRED,,false)
 if test -z "$GNOME_APPLETS_PKG_ERRORS" ; then
  if test "$with_xmlrpc" != "yes" -a "$with_curl" != "yes"
  then
    USE_APPLET=udp-client
  elif test "$ac_cv_lib_curl_curl_easy_init" = "yes"
  then
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
   else
    AC_MSG_ERROR([Sorry, we need XMLRPC])
   fi	

   if "$with_curl" = "yes"
   then
    USE_CURLAPP=curl-client
    CURL_CFLAGS="$(curl-config --cflags)"
    CURL_LIBS="$(curl-config --libs)"
    AC_SUBST(CURL_CFLAGS)
    AC_SUBST(CURL_LIBS)
   else
    AC_MSG_ERROR([Sorry, we need cURL])
   fi
  fi
 else
  : #echo "************ FAILS **************"
 fi
fi
AC_MSG_CHECKING(for sunny days)

GLIB_CFLAGS="$GLIB_MODULE_CFLAGS"
GLIB_LIBS="$GLIB_MODULE_LIBS"
AC_SUBST(USE_APPLET)
AC_SUBST(USE_RPCAPP)
AC_SUBST(USE_CURLAPP)
AC_SUBST(USE_RPCCGI)
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
)

