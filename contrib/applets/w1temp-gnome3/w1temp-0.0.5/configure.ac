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

# Checks for libraries.
# But don't set fscking LIBS ....

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

GTK_REQUIRED=0.20
LIBPANEL_REQUIRED=0.20
PKG_CHECK_MODULES(GNOME_APPLETS, gtk+-3.0 >= $GTK_REQUIRED  libpanelapplet-4.0 >= $LIBPANEL_REQUIRED,,false)

PKG_CHECK_MODULES(LIBSOUP, [libsoup-2.4 >= 2.26])
AC_SUBST(LIBSOUP_CFLAGS)
AC_SUBST(LIBSOUP_LIBS)

LIBPANEL_APPLET_DIR=`$PKG_CONFIG --variable=libpanel_applet_dir libpanelapplet-4.0`

AC_SUBST(LIBPANEL_APPLET_DIR)
AC_MSG_CHECKING(for sunny days)

CFLAGS=-Werror
GLIB_CFLAGS="$GLIB_MODULE_CFLAGS"
GLIB_LIBS="$GLIB_MODULE_LIBS"
AC_SUBST(USE_APPLET)
AC_SUBST(USE_CURLAPP)
AC_SUBST(GNOME_APPLETS_CFLAGS)
AC_SUBST(GNOME_APPLETS_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_MSG_RESULT(perhaps)

AC_OUTPUT(
Makefile
src/Makefile
)

