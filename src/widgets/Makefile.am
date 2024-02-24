AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libwidgets.la

if ENABLE_FFTW3
if ENABLE_OPENGL
else
FFT_SRC = spectrogram_widget.c spectrogram_widget.h
endif
endif

libwidgets_la_SOURCES = \
	dh_tree.c dh_tree.h \
	suggestion_entry.c suggestion_entry.h \
	colour-box.c colour-box.h \
	tagged-entry.c tagged-entry.h \
	$(FFT_SRC)

