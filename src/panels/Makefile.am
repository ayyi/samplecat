AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	-I$(top_srcdir)/lib/waveform/gtkglext-1.0 \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libpanels.la

libpanels_la_SOURCES = \
	library.c library.h \
	directories.c directories.h \
	inspector.c inspector.h \
	player_control.c player_control.h \
	spectrogram.c \
	search.c \
	tags.c \
	waveform.c

