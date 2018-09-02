
SUBDIRS = shaders views

bin_PROGRAMS = samplecat_gl

samplecat_gl_SOURCES = \
	main.c \
	application.c application.h \
	glx.c glx.h \
	directory.c directory.h \
	materials/icon_ring.c materials/icon_ring.h \
	shader.c shader.h \
	layout.c layout.h \
	style.c style.h \
	yaml_utils.c yaml_utils.h \
	keys.h \
	$(top_srcdir)/src/icon_theme.c $(top_srcdir)/src/icon_theme.h

AM_CFLAGS = \
	-no-pie \
	-Wl,-z,noexecstack \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/gtkglext-1.0 \
	-I$(top_srcdir)/experimental \
	-I$(top_srcdir) \
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

if !ENABLE_SYSTEM_GTKGLEXT
LOCAL_GTK_LIBS = \
	$(top_srcdir)/lib/waveform/gtkglext-1.0/libgtkglext.la
endif

samplecat_gl_LDADD = \
	views/libviews.la \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/dir_tree/libdirtree.a \
	$(top_srcdir)/lib/samplecat/libsamplecat.a \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/audio_analysis/libaudioanalysis.la \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/waveform/libwaveform.la \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(LOCAL_GTK_LIBS) \
	$(SAMPLECAT_LIBS) \
	$(AYYI_LIBS) \
	$(TRACKER_LIBS) \
	$(SQLITE_LIBS) \
	$(GTK_LIBS) $(MYSQL_LDFLAGS) \
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
	-lstdc++

dist_pkgdata_DATA = samplecat.yaml

