SUBDIRS =

AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Debug\" \
	-D_FILE_OFFSET_BITS=64 \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I$(includedir) \
	-I$(top_srcdir)/libs \
	$(GLIB_CFLAGS)

noinst_LIBRARIES = libdebug.a

if BUILD_PROFILING
PROFILING_SRC = instrument.c
endif

libdebug_a_SOURCES = \
	debug.c debug.h \
	$(PROFILING_SRC)

BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

