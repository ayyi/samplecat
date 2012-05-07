
bin_PROGRAMS = samplecat_webkit

samplecat_webkit_SOURCES = \
	webkit.c \
	util.c util.h \
	table.c table.h \
	../../lib/utils/ayyi_utils.c ../../lib/utils/ayyi_utils.h

AM_CFLAGS = \
	-g -Wall -O2 \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/file_manager \
	-I$(top_srcdir)/xdgmime \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
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
	$(MYSQL_CFLAGS) \
	$(WEBKIT_CFLAGS)

AM_CXXFLAGS = $(AM_CFLAGS)

samplecat_webkit_LDADD = \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/xdgmime/libxdgmime.a \
	$(top_srcdir)/lib/dir_tree/libdirtree.a \
	$(top_srcdir)/src/db/libsamplecatdb.a \
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
	$(FFMPEG_LIBS) \
	$(WEBKIT_LIBS)

