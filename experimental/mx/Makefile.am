NULL =

AM_CFLAGS = -g -Wall \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/src \
	-I$(top_builddir) \
	$(MX_CFLAGS) \
	$(MX_MAINTAINER_CFLAGS)
	$(GTK_CFLAGS)


LDADD = \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/samplecat/libsamplecat.a \
	$(top_srcdir)/lib/audio_analysis/libaudioanalysis.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/waveform/waveform/.libs/libwaveform.a \
	$(top_srcdir)/lib/waveform/agl/.libs/libagl.a \
	$(top_srcdir)/lib/waveform/transition/.libs/libtransition.a \
	$(top_srcdir)/lib/waveform/decoder/.libs/libwaveformdecoder.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(MX_LDFLAGS) $(MX_LIBS) \
	$(GTK_LIBS) \
	$(OPENGL_LIBS) \
	$(SNDFILE_LIBS) \
	$(FFMPEG_LIBS) \
	$(MYSQL_LDFLAGS) \
	$(SQLITE_LIBS) \
	-lstdc++ \
	-lz

noinst_PROGRAMS = \
	mx_test \
	$(NULL)

mx_test_SOURCES = main.c \
	list_model.c list_model.h \
	list_view.c list_view.h \
	list_item.c list_item.h \
	$(top_srcdir)/src/icon_theme.c $(top_srcdir)/src/icon_theme.h

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
