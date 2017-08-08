
AM_CFLAGS = \
	-g -Wall \
	-I../ \
	-I$(top_srcdir)/lib \
	$(FFTW3_CFLAGS) \
	$(GTK_CFLAGS)

AM_CXXFLAGS = $(AM_CFLAGS)

noinst_LIBRARIES = libaudioanalysis.a

if ENABLE_FFTW3
FFT_ANALYSERS_SRC = \
	spectrogram/spectrogram.c spectrogram/spectrogram.h \
	spectrogram/sndfile_window.c spectrogram/sndfile_window.h
endif

libaudioanalysis_a_SOURCES = \
	$(FFT_ANALYSERS_SRC) \
	meter/peak.c meter/peak.h \
	waveform/waveform.c waveform/waveform.h \
	ebumeter/ebur128.cc ebumeter/ebur128.h \
	ebumeter/ebu_r128_proc.cc ebumeter/ebu_r128_proc.h
