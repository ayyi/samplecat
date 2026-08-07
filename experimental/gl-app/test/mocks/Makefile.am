local_libs = $(top_srcdir)/lib

AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Mock\" \
	-I$(includedir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib \
	-I$(top_srcdir)/lib/ayyi \
	-I$(top_srcdir)/lib/test \
	$(GTK_CFLAGS)

if BUILD_TESTS

noinst_LIBRARIES = libmock.a

libmock_a_SOURCES = \
	icon_ring.c

endif
