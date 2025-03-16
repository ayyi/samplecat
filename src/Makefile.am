## Process this file with automake to produce Makefile.in

VALA_DIR =
SUBDIRS = widgets panels $(VALA_DIR)

AM_CFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/file_manager \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	-I./panels \
	$(SAMPLECAT_CFLAGS) \
	$(FFTW3_CFLAGS) \
	$(OPENGL_CFLAGS) \
	$(JACK_CFLAGS) \
	$(SAMPLERATE_CFLAGS) \
	$(SNDFILE_CFLAGS) \
	$(FFMPEG_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(DBUS_CFLAGS) $(MYSQL_CFLAGS) $(TRACKER_CFLAGS)

AM_CXXFLAGS = $(AM_CFLAGS)

bin_PROGRAMS = samplecat

if ENABLE_AYYI
AYYI_SRC = ayyi.c ayyi.h ayyi_model.c ayyi_model.h
endif

if ENABLE_OPENGL
if ENABLE_FFTW3
OPENGL_FFTW_SRC = gl_spectrogram_view.c gl_spectrogram_view.h
endif
OPENGL_SRC = \
	$(OPENGL_FFTW_SRC) \
	rotator.c rotator.h
OPENGL_LDFLAGS = \
	$(GL_LDFLAGS)
endif

DOCK_LDFLAGS = $(top_srcdir)/lib/gdl/.libs/libgdl-4.a $(XML_LIBS)

if ENABLE_YAML
YAML_LDFLAGS = \
	$(top_srcdir)/lib/yaml/libyaml.a \
	$(YAML_LIBS)
endif

samplecat_SOURCES = \
	typedefs.h \
	types.c types.h \
	main.c \
	resources.c \
	application.c application.h \
	application-nogui.c application-nogui.h \
	window.c window.h \
	progress_dialog.c progress_dialog.h \
	support.c support.h \
	$(OPENGL_SRC) \
	$(AYYI_SRC)

samplecat_LDADD = \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/samplecat/.libs/libsamplecat.a \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(DOCK_LDFLAGS) \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/gtk/libgtk.la \
	$(top_srcdir)/src/panels/libpanels.la \
	$(SAMPLECAT_LIBS) \
	$(AYYI_LIBS) \
	$(DBUS_LIBS) \
	$(top_srcdir)/lib/audio_analysis/libaudioanalysis.la \
	$(top_srcdir)/src/widgets/.libs/libwidgets.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformcore.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformui.a \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a \
	$(top_srcdir)/lib/gtk/.libs/libgtk.a \
	$(top_srcdir)/lib/menu/libmenu.la \
	$(top_srcdir)/lib/dir-tree/libdirtree.la \
	$(TRACKER_LIBS) \
	$(SQLITE_LIBS) \
	$(MYSQL_LDFLAGS) \
	$(JACK_LIBS) \
	$(SNDFILE_LIBS) \
	${SAMPLERATE_LIBS} \
	$(GTHREAD_LIBS) \
	$(FFTW3_LIBS) \
	$(OPENGL_LDFLAGS) \
	$(GTK_LDFLAGS) \
	$(LIBASS_LDFLAGS) \
	$(ZLIB_LIBS) \
	$(GMODULE_LIBS) \
	$(X11_LDFLAGS) \
	$(FFMPEG_LIBS) \
	$(MATH_LDFLAGS) \
	$(YAML_LDFLAGS) \
	$(GRAPHENE_LIBS) \
	-lstdc++ \
	-ldl

EXTRA_DIST = \
	menu.c
