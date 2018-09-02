
AM_CFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform \
	$(FFTW3_CFLAGS) \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libspectrogram.la

if ENABLE_FFTW3
FFT_ANALYSERS_SRC = \
	spectrogram.c spectrogram.h \
	sndfile_window.c sndfile_window.h
endif

libspectrogram_la_SOURCES = \
	$(FFT_ANALYSERS_SRC)
