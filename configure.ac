#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT(w1retap,1.4.0,jh+w1retap@daria.co.uk,w1retap)

AC_PREREQ(2.60)
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([dist-xz tar-pax no-dist-gzip])
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_ARG_WITH(mysql, AC_HELP_STRING([--without-mysql], [Don't build for MySQL]))
AC_ARG_WITH(postgres, AC_HELP_STRING([--without-postgres], [Don't build for PostgreSQL]))
AC_ARG_WITH(sqlite, AC_HELP_STRING([--without-sqlite], [Don't build for SQLite]))
AC_ARG_WITH(odbc, AC_HELP_STRING([--without-odbc], [Don't build for ODBC]))
AC_ARG_WITH(mongo, AC_HELP_STRING([--without-mongo], [Don't build for Mongodb]))
AC_ARG_WITH(libxml, AC_HELP_STRING([--without-libxml], [Don't build for libXML]))

# Checks for libraries.

AC_CHECK_LIB([mongoc], [mongo_connect])
AC_CHECK_LIB([odbc], [SQLAllocHandle])
# But don't set fscking LIBS ....
LIBS= 
AC_CHECK_LIB([m], [sin], [])
AC_CHECK_LIB([rt], [clock_nanosleep])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
CPPFLAGS="$CPPFLAGS -DMONGO_HAVE_STDINT=1"
AC_CHECK_HEADERS([fcntl.h limits.h memory.h stdlib.h string.h sys/file.h sys/time.h unistd.h sql.h mongoc/mongo.h])

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
AC_CHECK_FUNCS([alarm atexit gettimeofday memset sqrt stpcpy strcasecmp strchr strdup strncasecmp strpbrk strrchr strstr strtol clock_nanosleep])

USE_SQLITE=
USE_ODBC=
USE_MONGO=
USE_MYSQL=
USE_PGSQLE=
USE_APPLET=
GLIB_CFLAGS=
GLIB_LIBS=

PKG_CHECK_MODULES(SQLITE,sqlite3,[],[USE_SQLITE=])

if test "x${SQLITE_LIBS}" = "x" ; then 
   with_sqlite="no"
fi

AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LIBS)    

if test "$with_sqlite" != "no" ; then
 USE_SQLITE=sqlite
fi

if test "$ac_cv_header_mongo_h" = "no" -o "$ac_cv_lib_mongoc_mongo_connect" = "no" ; then
 with_mongo="no"
fi

if test "$with_mongo" != "no"; then
 USE_MONGO=mongo
fi

if test "$ac_cv_header_sql_h" = "no" -o "$ac_cv_lib_odbc_SQLAllocHandle" = "no" ; then
 with_odbc="no"
fi

if test "$with_odbc" != "no"; then
 USE_ODBC=odbc
fi


if test "$with_libxml" = "no" ; then
 : #AC_MSG_RESULT([not checking for libXML]) 
else
AC_PATH_PROGS([TOOL_XML2_CONFIG], [xml2-config])
    AS_IF([test -n "$TOOL_XML2_CONFIG"],
          [XML2_CFLAGS=`$TOOL_XML2_CONFIG --cflags`
           XML2_LIBS=`$TOOL_XML2_CONFIG --libs`
           LIBXML_VERSION=`$TOOL_XML2_CONFIG --version`])
AC_CHECK_LIB([xml2],[xmlNewTextWriterMemory]) 
AC_SUBST([XML2_CFLAGS])
AC_SUBST([XML2_LIBS]) 
fi

AC_PATH_PROG(MYSQL_CONFIG, mysql_config, no)
if test "$MYSQL_CONFIG" = "no" -o  "$with_mysql" = "no" ; then
 : #AC_MSG_RESULT([Not found])
else
 MYSQL_CFLAGS="$(mysql_config --cflags)"
 MYSQL_LIBS="$(mysql_config --libs)"
# Now test that we can actually build with MySQL
 save_CPPFLAGS="$CPPFLAGS"
 CPPFLAGS="$CPPFLAGS $MYSQL_CFLAGS"
 AC_CHECK_HEADER(mysql.h)
 if test "$ac_cv_header_mysql_h" = "yes" ;  then
   save_LIBS="$LIBS"
   LIBS="$LIBS $MYSQL_LIBS"
   AC_CHECK_LIB([mysqlclient], [mysql_init])
   if test "$ac_cv_lib_mysqlclient_mysql_init" = "yes" ; then
    USE_MYSQL=mysql
   fi
   LIBS="$save_LIBS"
 fi	
 CPPFLAGS="$save_CPPFLAGS"
fi

AC_PATH_PROG(PG_CONFIG, pg_config, no)
if test "$PG_CONFIG" = "no" -o "$with_pgsql" = "no"  ; then
 : # AC_MSG_RESULT([Not found])
else
 PGSQL_CFLAGS="$(pg_config --includedir)"
 PGSQL_LIBS="$(pg_config --libdir)"
 save_flags="$CPPFLAGS"
 CPPFLAGS="$CPPFLAGS -I$PGSQL_CFLAGS"
 AC_CHECK_HEADER(libpq-fe.h)
 if test "$ac_cv_header_libpq_fe_h" = "yes" ;  then
  save_libs="$LIBS"
  LIBS="$LIBS -L$PGSQL_LIBS"
  AC_CHECK_LIB([pq], [PQconnectdb])
  if test $ac_cv_lib_pq_PQconnectdb = yes ; then
   PGV="$(pg_config --version | cut -f 2 -d ' ' | cut -f 1 -d '.')" 
   PGSQL_CFLAGS="$PGSQL_CFLAGS -DPGV=$PGV"
   USE_PGSQL=pgsql
  fi
  LIBS="$save_libs"
 fi
 CPPFLAGS="$save_flags"
fi

CPPFLAGS="$save_flags -Werror -Wno-unused-but-set-variable -Wno-unused-result"

AC_SUBST(MYSQL_LIBS)
AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(PGSQL_LIBS)
AC_SUBST(PGSQL_CFLAGS)

AC_SUBST(USE_SQLITE)
AC_SUBST(USE_ODBC)
AC_SUBST(USE_MONGO)
AC_SUBST(USE_MYSQL)
AC_SUBST(USE_PGSQL)
AC_PATH_PROG(PKG_CONFIG, pkg-config,no)

if test "$PKG_CONFIG" = "no" ; then
 AC_MSG_ERROR([Sorry, we need pkg-config])
else
 PKG_CHECK_MODULES(GLIB_MODULE, gmodule-2.0 >= 2.8.0)
fi

AC_MSG_CHECKING(for fine weather)
GLIB_CFLAGS="$GLIB_MODULE_CFLAGS"
GLIB_LIBS="$GLIB_MODULE_LIBS"
AC_SUBST(GNOME_APPLETS_CFLAGS)
AC_SUBST(GNOME_APPLETS_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_MSG_RESULT(perhaps)

PKG_CHECK_MODULES(LIBUSB, libusb)
AC_SUBST(LIBUSB_CFLAGS)
AC_SUBST(LIBUSB_LIBS)

AC_OUTPUT(
Makefile
src/Makefile
src/libusblinux300/Makefile
src/sqlite/Makefile
src/mysql/Makefile
src/pgsql/Makefile
src/odbc/Makefile
src/mongo/Makefile
)

echo
echo "Configuration:"
if test -n "$USE_APPLET"; then echo ' * Gnome applet' ; fi
if test -n "$USE_MYSQL"; then echo ' * MySQL plugin' ; fi
if test -n "$USE_ODBC"; then echo ' * ODBC plugin' ; fi
if test -n "$USE_MONGO"; then echo ' * Mongodb plugin' ; fi
if test -n "$USE_PGSQL"; then echo ' * PostgreSQL plugin' ; fi
if test -n "$USE_SQLITE"; then echo ' * SQLite plugin' ; fi
echo
