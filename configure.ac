#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(configure.ac)
PACKAGE=w1retap
VERSION=0.0.7
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_ARG_WITH(mysql, AC_HELP_STRING([--without-applet], [Don't build the applet]))
AC_ARG_WITH(mysql, AC_HELP_STRING([--without-mysql], [Don't build for MySQL]))
AC_ARG_WITH(postgres, AC_HELP_STRING([--without-postgres], [Don't build for PostgreSQL]))
AC_ARG_WITH(sqlite, AC_HELP_STRING([--without-sqlite], [Don't build for SQLite]))
AC_ARG_WITH(odbc, AC_HELP_STRING([--without-odbc], [Don't build for ODBC]))

# Checks for libraries.
AC_CHECK_LIB([usb], [usb_control_msg])
AC_CHECK_LIB([sqlite3], [sqlite3_open])
AC_CHECK_LIB([odbc], [SQLAllocHandle])
# But don't set fscking LIBS ....
LIBS=
AC_CHECK_LIB([m], [sin], [])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h memory.h stdlib.h string.h sys/file.h sys/time.h unistd.h])

AC_CHECK_HEADERS(sqlite3.h)
AC_CHECK_HEADERS(sql.h)

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

USE_SQLITE=
USE_ODBC=
USE_MYSQL=
USE_PGSQLE=
USE_APPLET=
GLIB_CFLAGS=
GLIB_LIBS=

if test "$ac_cv_lib_sqlite3_sqlite3_open = yes" -a "$with_sqlite" != "no" ; then
 USE_SQLITE=sqlite
fi

if test "$ac_cv_lib_odbc_SQLAllocHandle = yes" -a "$with_odbc" != "no"; then
 USE_ODBC=odbc
fi

AC_PATH_PROG(MYSQL_CONFIG, mysql_config, no)
if test "$MYSQL_CONFIG" = "no" -o  "$with_mysql" = "no" ; then
 : #AC_MSG_RESULT([Not found])
else
 MYSQL_CFLAGS="$(mysql_config --cflags)"
 MYSQL_LIBS="$(mysql_config --libs)"
 USE_MYSQL=mysql
fi

AC_PATH_PROG(PG_CONFIG, pg_config, no)
if test "$PG_CONFIG" = "no" -o "$with_pgsql" = "no"  ; then
 : # AC_MSG_RESULT([Not found])
else
 save_libs="$LIBS"
 AC_CHECK_LIB([pq], [PQconnectdb])
 LIBS="$save_libs"
 if test $ac_cv_lib_pq_PQconnectdb = yes ; then
  PGSQL_CFLAGS="$(pg_config --includedir)"
  PGSQL_LIBS="$(pg_config --libdir)"
  PGV="$(pg_config --version | cut -f 2 -d ' ' | cut -f 1 -d '.')" 
  PGSQL_CFLAGS="$PGSQL_CFLAGS -DPGV=$PGV"
  USE_PGSQL=pgsql
 fi
fi

AC_SUBST(MYSQL_LIBS)
AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(PGSQL_LIBS)
AC_SUBST(PGSQL_CFLAGS)

AC_SUBST(USE_SQLITE)
AC_SUBST(USE_ODBC)
AC_SUBST(USE_MYSQL)
AC_SUBST(USE_PGSQL)
AC_PATH_PROG(PKG_CONFIG, pkg-config,no)

if test "$PKG_CONFIG" = "no" ; then
 AC_MSG_ERROR([Sorry, we need pkg-config])
else
 PKG_CHECK_MODULES(GLIB_MODULE, gmodule-2.0 >= 2.2.0)
fi


AC_MSG_CHECKING(for fine weather)
GLIB_CFLAGS="$GLIB_MODULE_CFLAGS"
GLIB_LIBS="$GLIB_MODULE_LIBS"
AC_SUBST(GNOME_APPLETS_CFLAGS)
AC_SUBST(GNOME_APPLETS_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_MSG_RESULT(perhaps)

if test "$with_applet" != "no" ; then
 USE_APPLET=w1temp
 AC_CONFIG_SUBDIRS(contrib/w1temp)
fi

AC_SUBST(USE_APPLET)	

AC_OUTPUT(
Makefile
src/Makefile
src/libusblinux300/Makefile
src/sqlite/Makefile
src/mysql/Makefile
src/pgsql/Makefile
src/odbc/Makefile
contrib/Makefile
)


echo
echo "Configuration:"
if test -n "$USE_APPLET"; then echo ' * Gnome applet' ; fi
if test -n "$USE_MYSQL"; then echo ' * MySQL plugin' ; fi
if test -n "$USE_ODBC"; then echo ' * ODBC plugin' ; fi
if test -n "$USE_PGSQL"; then echo ' * PostgreSQL plugin' ; fi
if test -n "$USE_SQLITE"; then echo ' * SQLite plugin' ; fi
echo
