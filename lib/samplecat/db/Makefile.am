## Process this file with automake to produce Makefile.in

SUBDIRS =

AM_CFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/src \
	$(SAMPLECAT_CFLAGS) \
	$(FFTW3_CFLAGS) \
	$(OPENGL_CFLAGS) \
	$(JACK_CFLAGS) \
	$(SAMPLERATE_CFLAGS) \
	$(SNDFILE_CFLAGS) \
	$(FFMPEG_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(GTK_CFLAGS) $(DBUS_CFLAGS) $(MYSQL_CFLAGS) $(FLAC_CFLAGS) $(TRACKER_CFLAGS)

AM_CXXFLAGS = $(AM_CFLAGS)

noinst_LIBRARIES = libsamplecatdb.a

if ENABLE_MYSQL
MYSQL_SRC = mysql.c mysql.h
endif

if ENABLE_SQLITE
SQLITE_SRC = sqlite.c sqlite.h
endif

if ENABLE_TRACKER
TRACKER_SRC = tracker.c tracker.h
endif

libsamplecatdb_a_SOURCES = \
	db.c db.h \
	$(MYSQL_SRC) \
	$(TRACKER_SRC) \
	$(SQLITE_SRC)

