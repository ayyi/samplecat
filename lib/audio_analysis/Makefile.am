
SUBDIRS = ebumeter meter spectrogram waveform

AM_CFLAGS = \
	-g -Wall \
	-I../ \
	-I$(top_srcdir)/lib \
	$(FFTW3_CFLAGS) \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libaudioanalysis.la

libaudioanalysis_la_SOURCES =

libaudioanalysis_la_LIBADD = \
	ebumeter/libebumeter.la \
	meter/libmeter.la \
	spectrogram/libspectrogram.la \
	waveform/libwaveform.la
