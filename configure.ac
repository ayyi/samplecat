AC_PREREQ(2.59)
AC_INIT([Samplecat],[0.3.2],[tim@orford.org])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([no-dist-gzip dist-xz])
AM_PROG_AR

# LT_REVISION=1

AC_PROG_CXX
AC_PROG_CC_C99
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

LT_INIT

AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

AC_SYS_LARGEFILE

AC_CONFIG_SUBDIRS([lib/waveform])

# At least level 2 optimisation is needed to allow GCC to
# determine that not all nested functions require trampolines.
CFLAGS="$CFLAGS -O2"

# Check for debugging flag
debug="no"
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging (false)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
	CFLAGS="$CFLAGS -g -DDEBUG"
	CXXFLAGS="$CFLAGS"
else
	CFLAGS="$CFLAGS -DNDEBUG"
	CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi

# default 'auto' configutaion
AC_ARG_ENABLE(mysql,    AC_HELP_STRING([--disable-mysql], [disable use of Mysql as backend]),enable_mysql=$enableval, enable_mysql="auto")
AC_ARG_ENABLE(ffmpeg,   AC_HELP_STRING([--disable-ffmpeg], [disable use of FFMPEG]), enable_ffmpeg=$enableval, enable_ffmpeg="auto")
AC_ARG_ENABLE(dnotify,  AS_HELP_STRING([--disable-dnotify], [build with dnotify support]), enable_dnotify=$enableval, enable_dnotify="auto")
AC_ARG_ENABLE(ayyi,     AS_HELP_STRING([--enable-ayyi],[build with ayyi support]),enable_ayyi=$enableval, enable_ayyi="no")
AC_ARG_ENABLE(ayyidbus, AS_HELP_STRING([--disable-ayyidbus],[disable use of dbus]),enable_ayyidbus=$enableval, enable_ayyidbus="auto")
AC_ARG_ENABLE(jack,     AS_HELP_STRING([--disable-jack],[build with JACK support]),enable_jack=$enableval, enable_jack="auto")
AC_ARG_ENABLE(gplayer,  AS_HELP_STRING([--disable-gplayer],[build with external player]),enable_gplayer=$enableval, enable_gplayer="auto")
AC_ARG_ENABLE(nks,      AS_HELP_STRING([--enable-nks],[build with nks support]),enable_nks=$enableval,enable_nks=auto)
AC_ARG_ENABLE(sf2,      AC_HELP_STRING([--enable-sf2], [build with sf2 support. Currently this option doesnt do anything useful.]),enable_sf2="$enableval", enable_sf2="no")
AC_ARG_ENABLE(tracker,  AS_HELP_STRING([--enable-tracker],[build with tracker support]),enable_tracker=$enableval, enable_tracker="auto")
AC_ARG_ENABLE(sqlite,   AS_HELP_STRING([--enable-sqlite],[build with sqlite support]),enable_sqlite=$enableval, enable_sqlite="auto")
AC_ARG_ENABLE(mx,       AS_HELP_STRING([--enable-mx], [build with mx support -- experimental]), enable_mx=$enableval, enable_mx="auto")
AC_ARG_ENABLE(webkit,   AS_HELP_STRING([--enable-webkit], [build with webkit support -- experimental]), enable_webkit=$enableval, enable_webkit="auto")
AC_ARG_ENABLE(gl-app,   AS_HELP_STRING([--enable-gl_app], [build the gl application -- experimental]), enable_gl_app=$enableval, enable_gl_app="auto")
AC_ARG_ENABLE(opengl,   AC_HELP_STRING([--disable-opengl], [disable use of OpenGL]),enable_opengl=$enableval,enable_opengl="auto")
AC_ARG_ENABLE(fftw,     AC_HELP_STRING([--disable-fftw], [disable usage of FFTW ]),enable_fftw=$enableval,enable_fftw="auto")
AC_ARG_ENABLE(libass,   AC_HELP_STRING([--disable-libass], [disable usage of libass ]),enable_libass=$enableval,enable_libass="auto")
AC_ARG_ENABLE(dock,     AC_HELP_STRING([--disable-dock], [disable usage of Gdl-dock ]), enable_dock=$enableval, enable_dock="auto")
AC_ARG_ENABLE(aubio,    AC_HELP_STRING([--enable-aubio], [enable use of aubio for bpm detection]), enable_aubio=$enableval, enable_aubio="auto")
AC_ARG_ENABLE(system-gtkglext, AC_HELP_STRING([--enable-system-gtkglext], [disable local opengl extension for gtk]), enable_system_gtkglext=$enableval, enable_system_gtkglext="no")

case $target_os in
	*darwin*)
		CFLAGS="$CFLAGS -fnested-functions"
		SAMPLECAT_LIBS="-ligemacintegration"
		SAMPLECAT_LIBS="$SAMPLECAT_LIBS -Wl,-flat_namespace,-undefined,dynamic_lookup -framework CoreAudio -framework CoreServices -framework AudioUnit -weak_framework Jackmp"
		enable_ayyidbus="no"
		enable_jack="no"
		enable_jackosx="yes"
		enable_sqlite="yes"
		enable_gplayer="yes"
		enable_opengl="no"
		;;
	*linux*)
		if test "$enable_dnotify" = "auto"; then enable_dnotify="yes"; fi
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

AC_MSG_CHECKING([if -Wl,--as-needed works])
LDFLAGS_save=$LDFLAGS
LDFLAGS="$LDFLAGS -Wl,--as-needed"
AC_TRY_LINK([], [],
            [
             AC_MSG_RESULT([yes])
             LDFLAGS="$LIBOAUTH_LDFLAGS -Wl,--as-needed"
            ], 
            [AC_MSG_RESULT([no])])
LDFLAGS=$LDFLAGS_save

dnl -----------------------------------------------------------------------------


GTK_CFLAGS=`pkg-config gtk+-2.0 --cflags`
GTK_LIBS=`pkg-config gtk+-2.0 --libs`
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

if `pkg-config gtk+-2.0 --atleast-version=2.12`; then
  HAVE_GTK_2_12="yes"
  AC_DEFINE(HAVE_GTK_2_12, 1, [We have at least gtk version 2.12])
else
  HAVE_GTK_2_12="no"
fi;

AC_DEFINE(USE_GTK,1,[Enable GTK in libwaveform])

GMODULE_LIBS=`pkg-config gmodule-2.0 --libs`
AC_SUBST(GMODULE_LIBS)

dnl -----------------------------------------------------------------------------

dnl added for suse-42
X11_LDFLAGS=`pkg-config --libs x11`
AC_SUBST(X11_LDFLAGS)

dnl -----------------------------------------------------------------------------

FILEMANAGER_LDFLAGS=`pkg-config --libs-only-l gmodule-2.0`
AC_SUBST(FILEMANAGER_LDFLAGS)

dnl -----------------------------------------------------------------------------

GLIB_CFLAGS=`pkg-config --cflags glib-2.0`
AC_SUBST(GLIB_CFLAGS)

if `pkg-config glib-2.0 --atleast-version=2.58`; then
  HAVE_GLIB_2_58="yes"
  AC_DEFINE(HAVE_GLIB_2_58, 1, [We have at least glib version 2.58])
else
  HAVE_GLIB_2_58="no"
fi;

if `pkg-config glib-2.0 --atleast-version=2.67`; then
  HAVE_GLIB_2_67="yes"
  AC_DEFINE(HAVE_GLIB_2_67, 1, [We have at least glib version 2.67])
else
  HAVE_GLIB_2_67="no"
fi;

GDK_CFLAGS=`pkg-config --cflags gdk-2.0`
AC_SUBST(GDK_CFLAGS)

PKG_CHECK_MODULES(GTHREAD, gthread-2.0,, [true])
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

PKG_CHECK_MODULES(GRAPHENE, graphene-1.0,, [true])
AC_SUBST(GRAPHENE_CFLAGS)
AC_SUBST(GRAPHENE_LIBS)

dnl -----------------------------------------------------------------------------

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

AC_SUBST(MYSQL_LDFLAGS)
AC_SUBST(MYSQL_CFLAGS)
if test x$enable_mysql = xyes; then
	AC_DEFINE(USE_MYSQL,1,[Enable Mysql as backend])
else
	enable_mysql="no"
fi
AM_CONDITIONAL(ENABLE_MYSQL, test "$enable_mysql" = "yes")

dnl This test is used to determine whether to use API
dnl that is removed in mysql version 8.
dnl It is currently always set because it is hopefully
dnl safe to do so.
HAVE_MYSQL_8="yes"
AC_DEFINE(HAVE_MYSQL_8, 1, [We have at least mysql version 8])

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_sqlite" != "no"],[
	PKG_CHECK_MODULES(SQLITE, sqlite3, AC_DEFINE(USE_SQLITE, 1, Use sqlite backend) enable_sqlite="yes", enable_sqlite="no")
])
AM_CONDITIONAL(ENABLE_SQLITE, test "$enable_sqlite" = "yes")
AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LIBS)

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_tracker" != "no"], [
	 PKG_CHECK_MODULES(TRACKER, tracker-client-0.10, [AC_DEFINE(USE_TRACKER, 1, Use Tracker metadata daemon) enable_tracker="yes"],[
		 PKG_CHECK_MODULES(TRACKER, tracker-client-0.9, [AC_DEFINE(USE_TRACKER, 1, Use Tracker metadata daemon) enable_tracker="yes"],
	   enable_tracker="no")]
	 ) ])
AM_CONDITIONAL(ENABLE_TRACKER, test "$enable_tracker" = "yes")
AC_SUBST(TRACKER_CFLAGS)
AC_SUBST(TRACKER_LIBS)

dnl -----------------------------------------------------------------------------

if test "$enable_mysql" = "no" && test "$enable_sqlite" = "no" && test "$enable_tracker" = "no"; then
	AC_MSG_ERROR([Please ensure one of either mysql, sqlite, or tracker is enabled])
fi

dnl -----------------------------------------------------------------------------

dnl TODO: sndfile is optional if FFMPEG is avail
PKG_CHECK_MODULES(SNDFILE, sndfile)
AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_ffmpeg" != "no"], [
	PKG_CHECK_MODULES(FFMPEG, [libavformat >= 51.0.0 libavcodec >= 51.0.0 libavutil >= 50.0.0 libswscale >= 5.0.0], enable_ffmpeg=yes, enable_ffmpeg=no)
	])

if test "$enable_ffmpeg" = "yes"; then
	AC_DEFINE(HAVE_FFMPEG,1,[Use ffmpeg audio decoder])
fi
AM_CONDITIONAL(ENABLE_FFMPEG, test "$enable_ffmpeg" = "yes")
AC_SUBST(FFMPEG_CFLAGS)
AC_SUBST(FFMPEG_LIBS)

dnl -----------------------------------------------------------------------------

if test "$enable_dnotify" = "yes"; then
	AC_DEFINE(USE_DNOTIFY,1,[Use dnotify on Linux])
	dnl AC_DEFINE(USE_NOTIFY,1,[Use notification]) dnl is set in dir.h
fi

AC_CHECK_HEADERS(sys/inotify.h)

dnl -----------------------------------------------------------------------------

PKG_CHECK_MODULES(ZLIB, zlib, enable_zlib="yes", enable_zlib="no")
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)
if test "$enable_zlib" = "yes"; then
	AC_DEFINE(HAVE_ZLIB,1,[Enable zlib compression])
fi

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_ayyidbus" != "no"], [
	PKG_CHECK_MODULES(DBUS, dbus-glib-1, enable_ayyidbus="yes", enable_ayyidbus="no")
	])
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)
if test "$enable_ayyidbus" = "yes"; then
	AC_DEFINE(HAVE_AYYIDBUS, 1, Use AYYI/Dbus auditioner.)
fi
AM_CONDITIONAL(ENABLE_AYYIDBUS, [test "$enable_ayyidbus" = "yes"])

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_ayyi" = "yes"], [
	PKG_CHECK_MODULES(AYYI, libayyi, AC_DEFINE(USE_AYYI,1,[Use ayyi]), enable_ayyi="no")
	])
AC_SUBST(AYYI_LIBS)
AM_CONDITIONAL(ENABLE_AYYI, [test "$enable_ayyi" = "yes"])

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_jack" != "no"], [
	PKG_CHECK_MODULES(JACK, jack,enable_jack=yes, enable_jack=no)
	], [true])

AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)
if test "$enable_jack" = "yes" -o "$enable_jackosx" = "yes"; then
	AC_DEFINE(HAVE_JACK,1,[have JACK audio connection kit])
fi
AM_CONDITIONAL(ENABLE_JACK, [test "$enable_jack" = "yes" -o "$enable_jackosx" = "yes"])

#JACK player can re-sample and use LADSPA plugins
enable_ladspa="no"
enable_resampling="no"
AS_IF([test "$enable_jack" = "yes" -o "$enable_jackosx" = "yes"], [
	PKG_CHECK_MODULES(SAMPLERATE, samplerate,
		[AC_DEFINE(ENABLE_RESAMPLING,1,[JACK player resampling])
		enable_resampling="yes"
		 dnl only use LADSPA/rubberband if libsamplerate is also available.
		 AC_CHECK_HEADER(ladspa.h, enable_ladspa="yes", AC_MSG_ERROR([LADSPA is required  see http://www.ladspa.org/]))
		], [true])
	])
AC_SUBST(SAMPLERATE_CFLAGS)
AC_SUBST(SAMPLERATE_LIBS)

if test "$enable_ladspa" = "yes"; then
	AC_DEFINE(ENABLE_LADSPA,1,[LADSPA plugin support])
fi

dnl -----------------------------------------------------------------------------

dnl TODO: check for executables: gst-launch | afplay | totem-audio-preview
AM_CONDITIONAL(ENABLE_GPLAYER, test "$enable_gplayer" != "no")
if test "$enable_gplayer" != "no"; then
	enable_gplayer="yes"
	AC_DEFINE(HAVE_GPLAYER,1,[Use gtk/gnome auditioner])
fi
AM_CONDITIONAL(ENABLE_GPLAYER, [test "$enable_gplayer" = "yes"])

dnl -----------------------------------------------------------------------------

AC_CHECK_LIB(yaml, yaml_parser_scan,
	[
		YAML_LIBS="-lyaml"
		AC_CHECK_HEADERS(yaml.h)
	],
	[],
	[-lyaml])
AC_SUBST(YAML_LIBS)
AM_CONDITIONAL(ENABLE_YAML, [test "$YAML_LIBS" = "-lyaml"])

dnl -----------------------------------------------------------------------------

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
AM_CONDITIONAL(ENABLE_NKS, test "$nks_found" = "yes")
AC_SUBST(NKS_CFLAGS)
AC_SUBST(NKS_LDFLAGS)

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_sf2" = "yes"], [
	 PKG_CHECK_MODULES(SF2, fluidsynth, [AC_DEFINE(USE_SF2, 1,[enable support for SF2 plugins]) enable_sf2="yes"],[ enable_sf2="no"])
	 ])
AM_CONDITIONAL(ENABLE_SF2, test "$enable_sf2" = "yes")
AC_SUBST(SF2_LDFLAGS)
AC_SUBST(SF2_CFLAGS)

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_mx" = "yes"],
	[PKG_CHECK_MODULES(MX, mx-1.0, AC_DEFINE(USE_MX, 1, Use mx widget toolkit) enable_mx="yes", enable_mx="no")],
	[enable_mx="no"]
)
AM_CONDITIONAL(ENABLE_MX, test "$enable_mx" = "yes")
AC_SUBST(MX_CFLAGS)
AC_SUBST(MX_LDFLAGS)

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_webkit" = "yes"],[
	PKG_CHECK_MODULES(WEBKIT, webkit-1.0, AC_DEFINE(USE_WEBKIT, 1, Use experimental Webkit interface) enable_webkit="yes", enable_webkit="no")
	], [enable_webkit="no"])
AM_CONDITIONAL(ENABLE_WEBKIT, test "$enable_webkit" = "yes")
AC_SUBST(WEBKIT_CFLAGS)
AC_SUBST(WEBKIT_LDFLAGS)

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_gl_app" = "yes"],[
	dnl PKG_CHECK_MODULES(WEBKIT, gtkglext-1.0, AC_DEFINE(USE_GL_APP, 1, Build the experimental GL application) enable_gl_app="yes", enable_gl_app="no")
	enable_gl_app="yes"
	], [enable_gl_app="no"])
AM_CONDITIONAL(ENABLE_GL_APP, test "$enable_gl_app" = "yes")

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_opengl" != "no"], [
	enable_opengl="yes"
	AC_DEFINE(USE_OPENGL, 1, Gtk Opengl features are enabled)
])

# Local version of Gtkglext only works with X11.
# For systems other than X11, the system gtkglext must be used.
AS_IF([test "$enable_system_gtkglext" = "yes"], [
	AC_DEFINE(USE_SYSTEM_GTKGLEXT, 1, [do not use the internal Gtkglext library])
	PKG_CHECK_MODULES(OPENGL, gtkglext-1.0, [AC_DEFINE(USE_OPENGL, 1, Opengl features are enabled)])
	GL_LDFLAGS=`pkg-config --libs gtkglext-1.0`
],[
	GL_LDFLAGS=`pkg-config --libs gl`
])

AC_SUBST(OPENGL_CFLAGS)
AC_SUBST(GL_LDFLAGS)
AM_CONDITIONAL(ENABLE_OPENGL, test "$enable_opengl" = "yes")
AM_CONDITIONAL(ENABLE_SYSTEM_GTKGLEXT, test "$enable_system_gtkglext" = "yes")

dnl -----------------------------------------------------------------------------

XRANDR_LDFLAGS=`pkg-config --libs xrandr`
AC_SUBST(XRANDR_LDFLAGS)

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_fftw" != "no"], [
	PKG_CHECK_MODULES(FFTW3, fftw3 >= 0.15.0, [AC_DEFINE([HAVE_FFTW3], 1, [Set to 1 if you have libfftw3.]) enable_fftw=yes], enable_fftw=no)
	])
AC_SUBST(FFTW3_CFLAGS)
AC_SUBST(FFTW3_LIBS)
AM_CONDITIONAL(ENABLE_FFTW3, test "$enable_fftw" = "yes")

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_libass" != "no"], [
	PKG_CHECK_MODULES(LIBASS, libass, [AC_DEFINE(USE_LIBASS, 1, Use libass for text display) enable_libass="yes"],[
	enable_libass="no"])
])
AM_CONDITIONAL(ENABLE_LIBASS, test "$enable_libass" = "yes")
AC_SUBST(LIBASS_CFLAGS)
dnl the libass pc file seems to be missing one dir:
if test "$enable_libass" = "yes"; then
	LIBASS_CFLAGS=${LIBASS_CFLAGS}" -I/usr/include/ass"
	LIBASS_LDFLAGS=${LIBASS_LIBS}
	AC_SUBST(LIBASS_LDFLAGS)
fi

dnl -----------------------------------------------------------------------------

if test "$enable_dock" = "auto"; then
	enable_dock="yes"
fi
if test "$enable_dock" = "yes"; then
	AC_DEFINE(USE_GDL, 1, Use Gdl-dock)
fi
AM_CONDITIONAL(ENABLE_GDL, test "$enable_dock" = "yes")
AC_SUBST(GDL_CFLAGS)

PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.2.8)
AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)

dnl -----------------------------------------------------------------------------

AS_IF([test "$enable_aubio" = "yes"],[
	PKG_CHECK_MODULES(AUBIO, aubio, AC_DEFINE(USE_AUBIO, 1, Use aubio for bpm detection) enable_aubio="yes", enable_aubio="no")
	], [enable_aubio="no"])
AM_CONDITIONAL(ENABLE_AUBIO, test "$enable_aubio" = "yes")
AC_SUBST(AUBIO_CFLAGS)
AC_SUBST(AUBIO_LDFLAGS)

dnl -----------------------------------------------------------------------------

dnl cannot use the pc file because it is not yet generated
WAVEFORM_LDFLAGS=`pwd`"/lib/waveform/.libs/libwaveformcore.a "`pwd`"/lib/waveform/.libs/libwaveformui.a "
AC_SUBST(WAVEFORM_LDFLAGS)

dnl -----------------------------------------------------------------------------

FRIBIDI_CFLAGS=`pkg-config --cflags fribidi`
FRIBIDI_LDFLAGS=`pkg-config --libs fribidi`
AC_SUBST(FRIBIDI_CFLAGS)
AC_SUBST(FRIBIDI_LDFLAGS)

dnl -----------------------------------------------------------------------------

MATH_LDFLAGS=-lm
AC_SUBST(MATH_LDFLAGS)

AC_ARG_ENABLE(test, AS_HELP_STRING([--enable-test],[ compile the test programs]), enable_test=yes, enable_test=no)
AM_CONDITIONAL(BUILD_TESTS, test "$enable_test" = "yes")

AC_ARG_ENABLE(valgrind,
	[  --enable-valgrind            Enable features to help with memory reporting.]
)
if test "$enable_valgrind" = "yes"; then
	AC_DEFINE(WITH_VALGRIND, 1, [Enable features to help with memory reporting])
else
	enable_valgrind=no
fi;


CXXFLAGS="$CXXFLAGS -pipe -fmessage-length=200 -fdiagnostics-show-location=every-line"
CFLAGS="$CFLAGS -pipe -fmessage-length=200 -fdiagnostics-show-location=every-line"

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([lib/Makefile])
AC_CONFIG_FILES([lib/utils/Makefile])
AC_CONFIG_FILES([lib/debug/Makefile])
AC_CONFIG_FILES([lib/file_manager/Makefile])
AC_CONFIG_FILES([lib/file_manager/filetypes/Makefile])
AC_CONFIG_FILES([lib/icon/Makefile])
AC_CONFIG_FILES([lib/dir_tree/Makefile])
AC_CONFIG_FILES([lib/samplecat/Makefile])
AC_CONFIG_FILES([lib/samplecat/db/Makefile])
AC_CONFIG_FILES([lib/samplecat/player/Makefile])
AC_CONFIG_FILES([lib/audio_analysis/Makefile])
AC_CONFIG_FILES([lib/audio_analysis/ebumeter/Makefile])
AC_CONFIG_FILES([lib/audio_analysis/meter/Makefile])
AC_CONFIG_FILES([lib/audio_analysis/spectrogram/Makefile])
AC_CONFIG_FILES([lib/audio_analysis/waveform/Makefile])
AC_CONFIG_FILES([lib/gdl/Makefile])
AC_CONFIG_FILES([lib/gtk/Makefile])
AC_CONFIG_FILES([lib/test/Makefile])
AC_CONFIG_FILES([lib/yaml/Makefile])
AC_CONFIG_FILES([layouts/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/panels/Makefile])
AC_CONFIG_FILES([test/ui/Makefile])
AC_CONFIG_FILES([test/unit/Makefile])
AC_CONFIG_FILES([experimental/Makefile])
AC_CONFIG_FILES([experimental/webkit/Makefile])
AC_CONFIG_FILES([experimental/mx/Makefile])
AC_CONFIG_FILES([experimental/gl-app/Makefile])
AC_CONFIG_FILES([experimental/gl-app/shaders/Makefile])
AC_CONFIG_FILES([experimental/gl-app/views/Makefile])
AC_CONFIG_FILES([experimental/gl-app/behaviours/Makefile])
AC_OUTPUT

jplayer="no"
if test "$enable_jack" = "yes" -o "$enable_jackosx" = "yes"; then
	jplayer="yes"
	if test "$enable_ladspa" = "yes" -a "$enable_resampling" = "yes"; then
		jplayer="${jplayer} w/ LADSPA support and resampling"
	else if test "$enable_resampling" = "yes"; then
		jplayer="${jplayer} w/ resampling (no LADSPA)"
	else if test "$enable_ladspa" = "yes"; then
		jplayer="${jplayer} w/ LADSPA support (no resampling)"
	fi;fi;fi
fi

AC_MSG_RESULT([])
AC_MSG_RESULT([Database backend:])
AC_MSG_RESULT([  mysql support:              $enable_mysql])
AC_MSG_RESULT([  sqlite support:             $enable_sqlite])
AC_MSG_RESULT([Audio decoders:])
AC_MSG_RESULT([  libsndfile:                 yes])
AC_MSG_RESULT([  ffmpeg decoder:             $enable_ffmpeg])
AC_MSG_RESULT([Audio players:])
AC_MSG_RESULT([  built-in JACK:              $jplayer])
AC_MSG_RESULT([  AYYI/Dbus:                  $enable_ayyidbus])
AC_MSG_RESULT([  CLI: gst,afplay,totem:      $enable_gplayer])
AC_MSG_RESULT([Building zlib:                $enable_zlib])
AC_MSG_RESULT([Building dnotify support:     $enable_dnotify])
AC_MSG_RESULT([Building inotify support:     $ac_cv_header_sys_inotify_h])
AC_MSG_RESULT([Building opengl support:      $enable_opengl])
AC_MSG_RESULT([Building libass support:      $enable_libass])
AC_MSG_RESULT([Building fftw support:        $enable_fftw])
AC_MSG_RESULT([Building ayyi support:        $enable_ayyi])
AC_MSG_RESULT([Building dock support:        $enable_dock])
AC_MSG_RESULT([Building valgrind:            $enable_valgrind])
AC_MSG_RESULT([Extra debugging:              $enable_debug])
AC_MSG_RESULT([Use system Gtk OpenGL:        $enable_system_gtkglext])
AC_MSG_RESULT([])

if test "$enable_mx" = "yes" -o "$enable_webkit" = "yes" -o "$enable_gl_app" = "yes" -o "$enable_sf2" = "yes" -o "$enable_nks" = "yes" -o "$enable_tracker" = "yes"; then
AC_MSG_RESULT([*** You have experimental options enabled. These are of no use other than to developers])
AC_MSG_RESULT([  tracker support:            $enable_tracker])
AC_MSG_RESULT([  soundfont support:          $enable_sf2])
AC_MSG_RESULT([  nks support:                $enable_nks])
AC_MSG_RESULT([  mx frontend:                $enable_mx])
AC_MSG_RESULT([  webkit frontend:            $enable_webkit])
AC_MSG_RESULT([  gl application:             $enable_gl_app])
AC_MSG_RESULT([])
fi
