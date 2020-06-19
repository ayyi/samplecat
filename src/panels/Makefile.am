AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libpanels.la

libpanels_la_SOURCES = \
	directories.c directories.h \
	player_control.c player_control.h \
	search.c \
	tags.c

