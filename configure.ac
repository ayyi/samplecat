AC_PREREQ(2.59)
AC_INIT([Samplecat],[0.1.2],[tim@orford.org])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

# LT_REVISION=1

AC_PROG_CXX
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

LT_INIT

AC_LIBTOOL_DLOPEN                                                                                                                                                             
AC_PROG_LIBTOOL

AC_SYS_LARGEFILE

# Check for debugging flag
debug="no"
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

# default 'auto' configutaion

case $target_os in
	*darwin*)
		CFLAGS="$CFLAGS -fnested-functions"
		SAMPLECAT_LIBS="-ligemacintegration"
		enable_dbus="no"
		enable_sqlite="yes"
		enable_gplayer="yes"
		enable_opengl="no"
		;;
	*linux*)
		enable_dnotify="yes"
		;;
	*)	
		;;
esac
AC_SUBST(SAMPLECAT_LIBS)
AC_SUBST(SAMPLECAT_CFLAGS)

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

dnl -----------------------------------------------------------------------------

dnl test for mysql

AC_ARG_ENABLE(mysql, [AC_HELP_STRING([--disable-mysql], [disable use of Mysql as backend])],enable_mysql=$enableval, enable_mysql="auto")

if test x$enable_mysql = xno ; then
	:
else
	AC_MSG_CHECKING(for mysql)
	AC_ARG_WITH(
		[mysql],
		[  --with-mysql=PFX        base of MySQL installation],
		[AC_MSG_RESULT("$with_mysql")
			if test "$with_mysql" != "no"
			then
				if test "$with_mysql" != "yes"
				then
					LDFLAGS="-L$with_mysql/lib -L$with_mysql/lib/mysql $LDFLAGS $ZLIBS"
					CPPFLAGS="-I$with_mysql/include $CPPFLAGS"
				fi
			fi
			AC_CHECK_HEADERS(
				[mysql/mysql.h],
				[
					AC_CHECK_LIB(
						[mysqlclient],
						[mysql_init],
						[MYSQL_LDFLAGS=`/usr/bin/mysql_config --libs`
						MYSQL_CFLAGS="-I$with_mysql/include"
						enable_mysql=yes]
					)
				]
			)

		],
		[
			AC_MSG_RESULT([--with-mysql not specified])
			LDFLAGS="-L/usr/lib/mysql $LDFLAGS $ZLIBS"
			AC_CHECK_LIB([mysqlclient],
				[mysql_init],
				[
					AC_CHECK_HEADERS(
						[mysql/mysql.h],
						[MYSQL_LDFLAGS=`/usr/bin/mysql_config --libs`
						enable_mysql=yes]
					)
				]
			)
		]
	)
fi

# SAVE_LDFLAGS=$LDFLAGS
# SAVE_CPPFLAGS=$CPPFLAGS

AC_SUBST(MYSQL_LDFLAGS)
AC_SUBST(MYSQL_CFLAGS)
if test x$enable_mysql = xyes; then
	AC_DEFINE(USE_MYSQL,1,[Enable Mysql as backend])
else
	enable_mysql="no"
fi
AM_CONDITIONAL(ENABLE_MYSQL, test "$enable_mysql" = "yes")

dnl -----------------------------------------------------------------------------

PKG_CHECK_MODULES(VFS, gnome-vfs-2.0,, [true])
AC_SUBST(VFS_CFLAGS)
AC_SUBST(VFS_LIBS)

PKG_CHECK_MODULES(LIBART, libart-2.0)
AC_SUBST(LIBART_CFLAGS)
AC_SUBST(LIBART_LIBS)

dnl XML_CFLAGS=`pkg-config libxml --cflags`
dnl AC_SUBST(XML_CFLAGS)

dnl TODO: sndfile is optional if FFMPEG is avail
PKG_CHECK_MODULES(SNDFILE, sndfile)
AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(ffmpeg, AC_HELP_STRING([--disable-ffmpeg], [disable use of FFMPEG]), enable_ffmpeg=$enableval, enable_ffmpeg="auto")

AS_IF([test "$enable_ffmpeg" != "no"], [
	PKG_CHECK_MODULES(FFMPEG, [libavformat libavcodec libavutil], enable_ffmpeg=yes, enable_ffmpeg=no)
       ])

if test "$enable_ffmpeg" = "yes"; then
	AC_DEFINE(HAVE_FFMPEG,1,[Use ffmpeg audio decoder])
fi
AM_CONDITIONAL(ENABLE_FFMPEG, test "$enable_ffmpeg" = "yes")
AC_SUBST(FFMPEG_CFLAGS)
AC_SUBST(FFMPEG_LIBS)

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

AC_ARG_ENABLE(dnotify, AS_HELP_STRING([--enable-dnotify],[build with dnotify support]),enable_dnotify=$enableval, enable_dnotify="no")

if test "$enable_dnotify" = "yes"; then
	AC_DEFINE(USE_DNOTIFY,1,[Use dnotify on Linux])
	AC_DEFINE(USE_NOTIFY,1,[Use notification])
fi
AM_CONDITIONAL(ENABLE_DNOTIFY, test "$enable_dnotify" = "yes")


dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(dbus, AS_HELP_STRING([--disable-dbus],[disable use of dbus]),enable_dbus=$enableval, enable_dbus="auto")
AS_IF([test "$enable_dbus" != "no"], [
       PKG_CHECK_MODULES(DBUS, dbus-glib-1, enable_dbus=yes, enable_dbus=no)
       ])
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)
if test "$enable_dbus" = "yes"; then
	AC_DEFINE(USE_DBUS, 1, Use Dbus is for messaging.)
fi
AM_CONDITIONAL(ENABLE_DBUS, [test "$enable_dbus" = "yes"])

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(ayyi, AS_HELP_STRING([--enable-ayyi],[build with ayyi support]),enable_ayyi=$enableval, enable_ayyi="no")

if test "$enable_ayyi" = "yes"; then
	AC_DEFINE(USE_AYYI,1,[Use ayyi])
fi
AM_CONDITIONAL(ENABLE_AYYI, [test "$enable_ayyi" = "yes"])

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_ayyi" = "no" -a "$enable_dbus" = "no" -a "$enable_gplayer" = "no"], [
	    PKG_CHECK_MODULES(JACK, jack,enable_builtin_player=yes, enable_builtin_player=no)
	], [
	    enable_builtin_player=no
	])

AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)
if test "$enable_builtin_player" = "yes"; then
	AC_DEFINE(HAVE_JACK,1,[have JACK audio connection kit])
fi

#JACK player can re-sample:
AS_IF([test "$enable_builtin_player" != "no"], [
	PKG_CHECK_MODULES(SAMPLERATE, samplerate,
	  [AC_DEFINE(ENABLE_RESAMPLING,1,[JACK player resampling])], [true])
	])
AC_SUBST(SAMPLERATE_CFLAGS)
AC_SUBST(SAMPLERATE_LIBS)

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(nks, AS_HELP_STRING([--enable-nks],[build with nks support]),enable_nks=$enableval,enable_nks=auto)
if `test "$enable_nks" = "yes"`; then
	AC_CHECK_LIB(nks, nks_open, [nks_found=yes])
	if `test "$nks_found" = "yes"`; then
		AC_DEFINE(USE_NKS, 1, Use nks library)
		enable_nks="yes"
		NKS_CFLAGS=""
		NKS_LDFLAGS="-lnks"
	else
		enable_nks="no"
	fi;
else
	enable_nks="no"
fi;
AM_CONDITIONAL(ENABLE_TRACKER, test "$nks_found" = "yes")
AC_SUBST(NKS_CFLAGS)
AC_SUBST(NKS_LDFLAGS)


dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(
	[sf2],
	[  --enable-sf2            build with sf2 support (default=no). Currently this option doesnt do anything useful.],
	[
		enable_sf2="$enableval"
		if test "$enableval" == "yes" && `pkg-config fluidsynth`; then
			SF2_CFLAGS=`pkg-config --cflags-only-I fluidsynth`
			SF2_LDFLAGS=`pkg-config --libs fluidsynth`
			AC_DEFINE(USE_SF2, 1, [enable support for SF2 plugins])
		else 
			SF2_CFLAGS=
			enable_sf2="no"
		fi;
	],
	[
		enable_sf2="no"
	]
)
AM_CONDITIONAL(ENABLE_SF2, test "$enable_sf2" != "no")
AC_SUBST(SF2_LDFLAGS)
AC_SUBST(SF2_CFLAGS)

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(tracker, AS_HELP_STRING([--enable-tracker],[build with tracker support]),enable_tracker=$enableval, enable_tracker="no")
if test "$enable_tracker" = "yes"; then
	dnl AC_CHECK_LIB(tracker-client-0.7, tracker_client_new, [tracker_found=yes])
	AC_CHECK_LIB(tracker-client-0.9, tracker_client_new, [tracker_found=yes])
	if `test "$tracker_found" = "yes"`; then
		AC_DEFINE(USE_TRACKER, 1, Use Tracker metadata daemon)
		enable_tracker="yes"
		dnl TRACKER_CFLAGS=`pkg-config --cflags-only-I tracker-client-0.7`
		dnl TRACKER_LDFLAGS=`pkg-config --libs-only-l tracker-client-0.7`
		TRACKER_CFLAGS=`pkg-config --cflags-only-I tracker-client-0.9`
		TRACKER_LDFLAGS=`pkg-config --libs-only-l tracker-client-0.9`
	else
		enable_tracker="no"
	fi;
else
	enable_tracker="no"
fi;
AM_CONDITIONAL(ENABLE_TRACKER, test "$tracker_found" = "yes")
AC_SUBST(TRACKER_CFLAGS)
AC_SUBST(TRACKER_LDFLAGS)


dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(sqlite, AS_HELP_STRING([--enable-sqlite],[build with sqlite support]),enable_sqlite=$enableval, enable_sqlite="no")
AS_IF([test "$enable_sqlite" = "yes"],[
       PKG_CHECK_MODULES(SQLITE, sqlite3, AC_DEFINE(USE_SQLITE, 1, Use sqlite backend), enable_sqlite=no)
       ])
AM_CONDITIONAL(ENABLE_SQLITE, test "$enable_sqlite" = "yes")
AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LIBS)

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(gplayer, AS_HELP_STRING([--enable-gplayer],[build with external player]),enable_gplayer=$enableval, enable_gplayer="no")
if test "$enable_ayyi" = "yes" -o "$enable_dbus" = "yes"; then
	AC_MSG_WARN([*** gplayer is exclusive with ayii and dbus support. ***])
	enable_gplayer=no
fi
AM_CONDITIONAL(ENABLE_GAUDITION, test "$enable_gplayer" = "yes")
if test "$enable_gplayer" = "yes"; then
	AC_DEFINE(USE_GAUDITION,1,[Use gtk/gnome auditioner])
fi

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(opengl, [AC_HELP_STRING([--disable-opengl], [disable use of OpenGL])],enable_opengl=$enableval,enable_opengl="no")
AS_IF([test "$enable_opengl" = "yes"], [
       PKG_CHECK_MODULES(OPENGL, gtkglext-1.0, [AC_DEFINE(USE_OPENGL, 1, Opengl features are enabled)])
       ])
AC_SUBST(OPENGL_CFLAGS)
AC_SUBST(OPENGL_LDIBS)
AM_CONDITIONAL(ENABLE_OPENGL, test "$enable_opengl" = "yes")

dnl -----------------------------------------------------------------------------

AC_ARG_ENABLE(fftw, AC_HELP_STRING([--disable-fftw], [disable usage of FFTW ]),enable_fftw=$enableval,enable_fftw="auto")
AS_IF([test "$enable_fftw" != "no"], [
	PKG_CHECK_MODULES(FFTW3, fftw3 >= 0.15.0, [AC_DEFINE([HAVE_FFTW3], 1, [Set to 1 if you have libfftw3.]) enable_fftw=yes], enable_fftw=no)
	])
AC_SUBST(FFTW3_CFLAGS)
AC_SUBST(FFTW3_LIBS)
AM_CONDITIONAL(ENABLE_FFTW3, test "$enable_fftw" = "yes")


dnl -----------------------------------------------------------------------------

PKG_CHECK_MODULES(GTHREAD, gthread-2.0,, [true])
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

dnl -----------------------------------------------------------------------------

#AC_DEFINE(NO_USE_DEVHELP_DIRTREE, 1, [Use the new directory tree])

# Bolt on a few specific flags to CXXFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -pipe -fmessage-length=139 -fdiagnostics-show-location=every-line"
CFLAGS="$CFLAGS -pipe  -fmessage-length=139 -fdiagnostics-show-location=every-line"

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/file_manager/Makefile])
AC_CONFIG_FILES([src/file_manager/filetypes/Makefile])
AC_CONFIG_FILES([src/gimp/Makefile])
AC_CONFIG_FILES([src/xdgmime/Makefile])
dnl AC_CONFIG_FILES([src/vala/Makefile])
AC_CONFIG_FILES([ayyi/Makefile])
AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([Building mysql support:           $enable_mysql])
AC_MSG_RESULT([Building sqlite support:          $enable_sqlite])
AC_MSG_RESULT([Building tracker support:         $enable_tracker])
AC_MSG_RESULT([Building direct flac support:     $enable_flac])
AC_MSG_RESULT([Building ffmpeg decoder:          $enable_ffmpeg])
AC_MSG_RESULT([Building dnotify support:         $enable_dnotify])
AC_MSG_RESULT([Building dbus support:            $enable_dbus])
AC_MSG_RESULT([Building ayyi support:            $enable_ayyi])
AC_MSG_RESULT([Building gplayer support:         $enable_gplayer])
AC_MSG_RESULT([Building opengl support:          $enable_opengl])
AC_MSG_RESULT([Building builtin JACK player:     $enable_builtin_player])
AC_MSG_RESULT([Building soundfont support:       $enable_sf2])
AC_MSG_RESULT([Building nks support:             $enable_nks])
AC_MSG_RESULT([])
