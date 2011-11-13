
EXTRA_DIST = soundfont.c \
	filetype_plugin.h

#if ENABLE_SF2
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/libs $(GTK_CFLAGS) $(SF2_CFLAGS)

pkglib_LTLIBRARIES = libsf2.la

libsf2_la_SOURCES = soundfont.c 
libsf2_la_LIBADD = $(PACKAGE_LIBS) $(SF2_LDFLAGS) $(INTLLIBS)
libsf2_la_CFLAGS = $(PACKAGE_CFLAGS) $(SF2_CFLAGS)
libsf2_la_LDFLAGS = -avoid-version -module -export-dynamic
#endif

