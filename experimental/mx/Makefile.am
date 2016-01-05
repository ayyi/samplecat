NULL =

AM_CFLAGS = -g -Wall $(MX_CFLAGS) $(MX_MAINTAINER_CFLAGS)

LDADD = \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/samplecat/libsamplecat.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/waveform/waveform/.libs/libwaveform.a \
	$(top_srcdir)/lib/waveform/agl/.libs/libagl.a \
	$(top_srcdir)/lib/waveform/transition/.libs/libtransition.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/xdgmime/libxdgmime.a \
	$(MX_LDFLAGS) $(MX_LIBS) \
	$(GTK_LIBS) \
	$(OPENGL_LIBS) \
	$(SNDFILE_LIBS) \
	$(FFMPEG_LIBS) \
	$(MYSQL_LDFLAGS) \
	$(SQLITE_LIBS) \
	-lz

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/src \
	-I$(top_builddir) \
	$(GTK_CFLAGS)

noinst_PROGRAMS = \
	mx_test \
	$(NULL)

AUDIOANALYSERS_SRC = \
	$(FFT_ANANALYSERS_SRC) \
	$(top_srcdir)/src/audio_analysis/meter/peak.c $(top_srcdir)/src/audio_analysis/meter/peak.h \
	$(top_srcdir)/src/audio_analysis/waveform/waveform.c $(top_srcdir)/src/audio_analysis/waveform/waveform.h \
	$(top_srcdir)/src/audio_analysis/ebumeter/ebur128.cc $(top_srcdir)/src/audio_analysis/ebumeter/ebur128.h \
	$(top_srcdir)/src/audio_analysis/ebumeter/ebu_r128_proc.cc $(top_srcdir)/src/audio_analysis/ebumeter/ebu_r128_proc.h

AD_SRC = ../../src/audio_decoder/ad.h \
	../../src/audio_decoder/ad_plugin.h ../../src/audio_decoder/ad_plugin.c \
	../../src/audio_decoder/ad_soundfile.c \
	../../src/audio_decoder/ad_flac.c \
	../../src/audio_decoder/ad_ffmpeg.c \
	../../src/audio_decoder/ffcompat.h

mx_test_SOURCES = main.c \
	list_model.c list_model.h \
	list_view.c list_view.h \
	list_item.c list_item.h \
	$(top_srcdir)/src/list_store.c $(top_srcdir)/src/list_store.h \
	$(top_srcdir)/src/worker.c $(top_srcdir)/src/worker.h \
	$(top_srcdir)/src/icon_theme.c $(top_srcdir)/src/icon_theme.h \
	$(AUDIOANALYSERS_SRC) \
	$(AD_SRC)

###$(top_srcdir)/lib/utils/ayyi_utils.c $(top_srcdir)/lib/utils/ayyi_utils.h

GLIB_GENMARSHAL = glib-genmarshal

marshal.h: stamp-marshal.h
	@true
stamp-marshal.h: Makefile marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) \
		--prefix=_samplecat_marshal \
		--header \
	$(srcdir)/marshal.list > xgen-tmh && \
	(cmp -s xgen-tmh marshal.h || cp -f xgen-tmh marshal.h) && \
	rm -f xgen-tmh && \
	echo timestamp > $(@F)

marshal.c: Makefile marshal.list
	$(AM_V_GEN)(echo "#include \"marshal.h\"" ; \
	 $(GLIB_GENMARSHAL) \
		--prefix=_samplecat_marshal \
		--body \
	 $(srcdir)/marshal.list ) > xgen-tmc && \
	cp -f xgen-tmc marshal.c && \
	rm -f xgen-tmc

BUILT_SOURCES = \
	marshal.h

EXTRA_DIST = \
	marshal.list \
	style.css

#-include $(top_srcdir)/git.mk
