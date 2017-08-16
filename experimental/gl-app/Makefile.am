
SUBDIRS = shaders

bin_PROGRAMS = samplecat_gl

samplecat_gl_SOURCES = \
	main.c \
	application.c application.h \
	glx.c glx.h \
	directory.c directory.h \
	views/dock_v.c views/dock_v.h \
	views/dock_h.c views/dock_h.h \
	views/panel.c views/panel.h \
	views/list.c views/list.h \
	views/files.c views/files.h \
	views/files.impl.c views/files.impl.h \
	views/dirs.c views/dirs.h \
	views/inspector.c views/inspector.h \
	views/search.c views/search.h \
	views/tabs.c views/tabs.h \
	views/scrollbar.c views/scrollbar.h \
	shader.c shader.h \
	layout.c layout.h \
	yaml_utils.c yaml_utils.h \
	keys.h \
	$(top_srcdir)/src/icon_theme.c $(top_srcdir)/src/icon_theme.h

AM_CFLAGS = \
	-g -Wall -O2 \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/experimental \
	-I./gimp \
	$(SAMPLECAT_CFLAGS) \
	$(FFTW3_CFLAGS) \
	$(OPENGL_CFLAGS) \
	$(JACK_CFLAGS) \
	$(SAMPLERATE_CFLAGS) \
	$(SNDFILE_CFLAGS) \
	$(FFMPEG_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(MYSQL_CFLAGS)

AM_CXXFLAGS = $(AM_CFLAGS)

samplecat_gl_LDADD = \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/dir_tree/libdirtree.a \
	$(top_srcdir)/lib/samplecat/libsamplecat.a \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/audio_analysis/libaudioanalysis.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/waveform/waveform/.libs/libwaveform.a \
	$(top_srcdir)/lib/waveform/agl/.libs/libagl.a \
	$(top_srcdir)/lib/waveform/transition/.libs/libtransition.a \
	$(top_srcdir)/lib/waveform/decoder/.libs/libwaveformdecoder.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/xdgmime/libxdgmime.a \
	$(SAMPLECAT_LIBS) \
	$(AYYI_LIBS) \
	$(TRACKER_LIBS) \
	$(SQLITE_LIBS) \
	$(GTK_LIBS) $(MYSQL_LDFLAGS) \
	$(JACK_LIBS) $(SNDFILE_LIBS) ${SAMPLERATE_LIBS} \
	$(GTHREAD_LIBS) \
	$(FFTW3_LIBS) \
	$(OPENGL_LIBS) \
	$(ZLIB_LIBS) \
	$(YAML_LIBS) \
	$(FFMPEG_LIBS) \
	-lstdc++

dist_pkgdata_DATA = samplecat.yaml

