
bin_PROGRAMS = samplecat_webkit

samplecat_webkit_SOURCES = \
	webkit.c \
	util.c util.h \
	table.c table.h \
	$(top_srcdir)/src/icon_theme.c $(top_srcdir)/src/icon_theme.h

if !ENABLE_SYSTEM_GTKGLEXT
LOCAL_GTK_LIBS = \
	$(top_srcdir)/lib/waveform/gtkglext-1.0/libgtkglext.la
endif

AM_CFLAGS = \
	-g -Wall -O2 \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I./gimp \
	$(SAMPLECAT_CFLAGS) \
	$(FFTW3_CFLAGS) \
	$(OPENGL_CFLAGS) \
	$(GMODULE_LIBS) \
	$(JACK_CFLAGS) \
	$(SAMPLERATE_CFLAGS) \
	$(SNDFILE_CFLAGS) \
	$(FFMPEG_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(MYSQL_CFLAGS) \
	$(WEBKIT_CFLAGS)

AM_CXXFLAGS = $(AM_CFLAGS)

samplecat_webkit_LDADD = \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/dir_tree/libdirtree.a \
	$(top_srcdir)/lib/samplecat/libsamplecat.a \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/audio_analysis/libaudioanalysis.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/waveform/libwaveform.la \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(LOCAL_GTK_LIBS) \
	$(AYYI_LIBS) \
	$(TRACKER_LIBS) \
	$(SQLITE_LIBS) \
	$(GTK_LIBS) $(MYSQL_LDFLAGS) \
	$(SNDFILE_LIBS) ${SAMPLERATE_LIBS} \
	$(GTHREAD_LIBS) \
	$(FFTW3_LIBS) \
	$(GL_LDFLAGS) \
	$(ZLIB_LIBS) \
	$(FFMPEG_LIBS) \
	$(WEBKIT_LIBS) \
	$(X11_LDFLAGS) \
	$(LIBASS_LDFLAGS) \
	-lstdc++

