
noinst_LTLIBRARIES = libviews.la

if ENABLE_FFTW3
FFT_SRC = spectrogram.c spectrogram.h
endif

libviews_la_SOURCES = \
	dock_v.c dock_v.h \
	dock_h.c dock_h.h \
	panel.c panel.h \
	overlay.c overlay.h \
	scrollable.c scrollable.h \
	list.c list.h \
	files.c files.h \
	files.impl.c files.impl.h \
	files.with_wav.c files.with_wav.h \
	dirs.c dirs.h \
	inspector.c inspector.h \
	search.c search.h \
	filters.c filters.h \
	player.c player.h \
	tabs.c tabs.h \
	scrollbar.c scrollbar.h \
	context_menu.c context_menu.h \
	button.c button.h \
	graph_debug.c graph_debug.h \
	$(FFT_SRC)

AM_CFLAGS = \
	-no-pie \
	-Wl,-z,noexecstack \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform/ \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	-I$(top_srcdir)/lib/waveform/gtkglext-1.0 \
	-I$(top_srcdir)/experimental/gl-app \
	-I$(top_srcdir) \
	$(SAMPLECAT_CFLAGS) \
	$(OPENGL_CFLAGS) \
	$(SAMPLERATE_CFLAGS) \
	$(FFMPEG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GRAPHENE_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(MYSQL_CFLAGS)
