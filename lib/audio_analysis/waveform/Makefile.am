
AM_CFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	$(FFTW3_CFLAGS) \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libwaveform.la

libwaveform_la_SOURCES = \
	waveform.c waveform.h
