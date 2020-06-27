
if BUILD_TESTS

AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libtest.la

libtest_la_SOURCES = \
	runner.c runner.h

endif

