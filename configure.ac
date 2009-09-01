AC_PREREQ(2.59)
AC_INIT([Samplecat],[0.0.7],[tim@orford.org])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# LT_REVISION=1

AC_PROG_CXX
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Library building stuff
AC_PROG_RANLIB

# Check for debugging flag
debug="yes"
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging (false)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  CFLAGS="-O0 -g -DDEBUG"
  CXXFLAGS="$CFLAGS"
else
  CFLAGS="$CFLAGS -DNDEBUG"
  CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi

# Check for strict flag
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (false)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  CFLAGS="$CFLAGS -ansi -Wall -Wextra -Wno-unused-parameter -Wconversion -Winit-self"
  CXXFLAGS="$CXXFLAGS -ansi -Wall -Wextra -Wno-unused-parameter -Wconversion -Winit-self -Woverloaded-virtual -Wsign-promo"
fi

dnl -----------------------------------------------------------------------------


GTK_CFLAGS=`pkg-config gtk+-2.0 --cflags`
GTK_LIBS=`pkg-config gtk+-2.0 --libs`
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

if `pkg-config gtk+-2.0 --atleast-version=2.12`; then
  HAVE_GTK_2_12="yes"
  AC_DEFINE(HAVE_GTK_2_12, 1, "we have at least gtk version 2.12")
else
  HAVE_GTK_2_12="no"
fi;

if `pkg-config gtk+-2.0 --atleast-version=2.10`; then
  HAVE_GTK_2_10="yes"
  AC_DEFINE(HAVE_GTK_2_10, 1, "we have at least gtk version 2.10")
else
  HAVE_GTK_2_10="no"
fi;

DB_CFLAGS="-I/usr/include/mysql"
DB_LIBS=`/usr/bin/mysql_config --libs`
AC_SUBST(DB_CFLAGS)
AC_SUBST(DB_LIBS)

VFS_CFLAGS=`pkg-config gnome-vfs-2.0 --cflags`
VFS_LIBS="-lgnomevfs-2"
AC_SUBST(VFS_CFLAGS)
AC_SUBST(VFS_LIBS)

dnl AC_CHECK_LIB(art_lgpl_2, art_rgba_run_alpha, [libart_found=yes], AC_MSG_RESULT(**** Cannot find LIBART ****))
LIBART_CFLAGS=`pkg-config libart-2.0 --cflags`
LIBART_LIBS=`pkg-config libart-2.0 --libs`
AC_SUBST(LIBART_CFLAGS)
AC_SUBST(LIBART_LIBS)

XML_CFLAGS=`pkg-config libxml --cflags`
AC_SUBST(XML_CFLAGS)

JACK_LIBS=`pkg-config jack --cflags --libs`
AC_SUBST(JACK_LIBS)

SNDFILE_LIBS=`pkg-config sndfile --cflags --libs`
AC_SUBST(SNDFILE_LIBS)

dnl -----------------------------------------------------------------------------

dnl flac doesnt use pkg-config it seems...

AC_ARG_ENABLE(flac, AC_HELP_STRING([--disable-flac], [disable use of FLAC]))

FLAC_LIBS=""

if test x$enable_flac != xno ; then
    AC_CHECK_HEADERS(FLAC/all.h)
    if test x$ac_cv_header_FLAC_all_h = xyes ; then
        AC_CHECK_LIB(FLAC, FLAC__seekable_stream_encoder_set_tell_callback, HAVE_FLAC_1_1_1="yes")
        if test "x$HAVE_FLAC_1_1_1" = xyes ; then
            AC_DEFINE(HAVE_FLAC_1_1_1, [1], [Define to 1 if you have libflac 1.1.1])
        fi
        FLAC_LIBS="-lFLAC"
    fi
	enable_flac="yes"
fi

AC_SUBST(FLAC_LIBS)

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(dnotify, AS_HELP_STRING([--enable-dnotify],[build with dnotify support]),enable_dnotify=$enableval,enable_dnotify=auto)

if test x$enable_dnotify = xno ; then
    have_dnotify=no;
else
    have_dnotify=yes;	
fi

if test x$have_dnotify = xyes; then
   AC_DEFINE(USE_DNOTIFY,1,[Use dnotify on Linux])
   AC_DEFINE(USE_NOTIFY,1,[Use notification])
fi

AM_CONDITIONAL(ENABLE_DNOTIFY, test x$have_dnotify = xyes)


dnl -----------------------------------------------------------------------------

dbus="yes"
AC_ARG_ENABLE(dbus,
[  --disable-dbus          disable use of dbus],[
  case "$enableval" in
    "yes")
      echo "dbus YES"
      ;;
    "no")
      echo "dbus NO"
      dbus="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-dbus(=yes/no) or --disable-dbus])
      ;;
  esac
])
if `test "$dbus" = "yes"`; then
    AC_CHECK_LIB(dbus-glib-1, dbus_g_bus_get, [DBUS_FOUND=yes], [echo "*** dbus not found!"])
	if test "$DBUS_FOUND" = "yes"; then
		AM_CONDITIONAL(ENABLE_DBUS, test "yes" = "yes")
		AC_DEFINE(USE_DBUS, 1, Dbus is used for messaging.)
	else
		AC_MSG_ERROR([Dbus was configured as the comms system but was not found on your machine.])
	fi;
	DBUS_CFLAGS=`pkg-config --cflags-only-I dbus-glib-1`
	DBUS_LDFLAGS=`pkg-config --libs-only-l dbus-glib-1`
else
	AM_CONDITIONAL(ENABLE_DBUS, test "yes" = "no")
	DBUS_CFLAGS=
	DBUS_LDFLAGS=
fi;
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LDFLAGS)


dnl -----------------------------------------------------------------------------

AC_CHECK_LIB(trackerclient, tracker_connect, [tracker_found=yes])
AM_CONDITIONAL(ENABLE_TRACKER, test "$tracker_found" = "yes")
if `test "$tracker_found" = "yes"`; then
    AC_DEFINE(USE_TRACKER, 1, Use Tracker metadata daemon)
    enable_tracker="yes"
    TRACKER_CFLAGS=`pkg-config --cflags-only-I tracker`
    TRACKER_LDFLAGS=`pkg-config --libs-only-l tracker`
else
    enable_tracker="no"
fi;
AC_SUBST(TRACKER_CFLAGS)
AC_SUBST(TRACKER_LDFLAGS)


dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(sqlite, AS_HELP_STRING([--enable-sqlite],[build with sqlite support]),enable_sqlite=$enableval,enable_sqlite=auto)
AM_CONDITIONAL(ENABLE_SQLITE, test "$enable_sqlite" = "yes")
if `test "$enable_sqlite" = "yes"`; then
    AC_DEFINE(USE_SQLITE, 1, Use sqlite backend)
fi;

SQLITE_CFLAGS=`pkg-config sqlite3 --cflags`
SQLITE_LDFLAGS=`pkg-config sqlite3 --libs`
AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LDFLAGS)

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(ayyi, AS_HELP_STRING([--enable-ayyi],[build with ayyi support]),enable_ayyi=$enableval,enable_ayyi=auto)
AM_CONDITIONAL(ENABLE_AYYI, test "$enable_ayyi" = "yes")
if test "$enable_ayyi" = "yes"; then
   AC_DEFINE(USE_AYYI,1,[Use ayyi])
fi
AM_CONDITIONAL(ENABLE_AYYI, test "$enable_ayyi" = "yes")


dnl -----------------------------------------------------------------------------

SNDFILE_LIBS=`pkg-config --cflags --libs sndfile`
AC_SUBST(SNDFILE_LIBS)

GTHREAD_LIBS=`pkg-config --libs gthread-2.0`
AC_SUBST(GTHREAD_LIBS)
#LIBART_CFLAGS=`pkg-config --cflags libart-2.0`
#AC_SUBST(LIBART_CFLAGS)
#LIBART_LIBS=`pkg-config --libs libart-2.0`
#AC_SUBST(LIBART_LIBS)

# Bolt on a few specific flags to CXXFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -pipe -fmessage-length=139 -fdiagnostics-show-location=every-line"
CFLAGS="$CFLAGS -pipe  -fmessage-length=139 -fdiagnostics-show-location=every-line"

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/file_manager/Makefile])
AC_CONFIG_FILES([src/gimp/Makefile])
AC_CONFIG_FILES([src/xdgmime/Makefile])
AC_CONFIG_FILES([ayyi/Makefile])
AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([Building direct flac support:     $enable_flac])
AC_MSG_RESULT([Building dnotify support:         $have_dnotify])
AC_MSG_RESULT([Building ayyi support:            $enable_ayyi])
AC_MSG_RESULT([Building tracker support:         $enable_tracker])
AC_MSG_RESULT([Building sqlite support:          $enable_sqlite])
AC_MSG_RESULT([])