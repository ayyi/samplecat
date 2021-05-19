
AM_CFLAGS = \
	-I$(top_srcdir)/lib/waveform \
	$(GLIB_CFLAGS)

AM_CXXFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES = libebumeter.la

libebumeter_la_SOURCES = \
	ebur128.cc ebur128.h \
	ebu_r128_proc.cc ebu_r128_proc.h
