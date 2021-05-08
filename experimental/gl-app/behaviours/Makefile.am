
noinst_LTLIBRARIES = libbehaviours.la

libbehaviours_la_SOURCES = \
	style.c style.h \
	state.c state.h \
	panel.c panel.h

AM_CFLAGS = \
	-no-pie \
	-Wl,-z,noexecstack \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	-I$(top_srcdir) \
	$(SAMPLECAT_CFLAGS) \
	$(OPENGL_CFLAGS) \
	$(FFMPEG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(ZLIB_CFLAGS)
