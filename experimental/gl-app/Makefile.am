
SUBDIRS = shaders views behaviours test

bin_PROGRAMS = samplecat_gl

samplecat_gl_SOURCES = \
	main.c \
	application.c application.h \
	directory.c directory.h \
	materials/icon_ring.c materials/icon_ring.h \
	shader.c shader.h \
	layout.c layout.h \
	utils.c utils.h \
	keys.h \
	action.h

AM_CFLAGS = \
	-no-pie \
	-Wl,-z,noexecstack \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	-I$(top_srcdir)/experimental \
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
	views/libviews.la \
	behaviours/libbehaviours.la \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/dir-tree/libdirtree.la \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/samplecat/.libs/libsamplecat.a \
	$(top_srcdir)/lib/audio_analysis/.libs/libaudioanalysis.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/icon/libicon.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/yaml/libyaml.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformcore.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformui.a \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/gtk/libgtk.la \
	$(top_srcdir)/lib/menu/libmenu.la \
	$(SAMPLECAT_LIBS) \
	$(AYYI_LIBS) \
	$(TRACKER_LIBS) \
	$(SQLITE_LIBS) \
	$(GTK_LDFLAGS) \
	$(GRAPHENE_LIBS) \
	$(MYSQL_LDFLAGS) \
	$(JACK_LIBS) $(SNDFILE_LIBS) ${SAMPLERATE_LIBS} \
	$(GTHREAD_LIBS) \
	$(FFTW3_LIBS) \
	$(GL_LDFLAGS) \
	$(ZLIB_LIBS) \
	$(YAML_LIBS) \
	$(FFMPEG_LIBS) \
	$(LIBASS_LDFLAGS) \
	$(FILEMANAGER_LDFLAGS) \
	$(X11_LDFLAGS) \
	$(XRANDR_LDFLAGS) \
	$(DBUS_LIBS) \
	$(MATH_LDFLAGS) \
	$(FRIBIDI_LDFLAGS) \
	-lstdc++

dist_pkgdata_DATA = samplecat.yaml

