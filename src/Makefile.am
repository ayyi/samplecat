## Process this file with automake to produce Makefile.in

#SUBDIRS = file_manager gimp xdgmime vala
SUBDIRS = file_manager gimp xdgmime

AM_CFLAGS = \
	-g -Wall -O2 \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I./gqview2 \
	-I./file_manager \
	-I./gimp \
	-I./xdgmime \
	$(FFTW3_CFLAGS) \
	$(OPENGL_CFLAGS) \
	$(JACK_CFLAGS) \
	$(SNDFILE_CFLAGS) \
	$(GTK_CFLAGS) $(DBUS_CFLAGS) $(MYSQL_CFLAGS) $(FLAC_CFLAGS) $(TRACKER_CFLAGS)

#$(XML_CFLAGS)

bin_PROGRAMS = samplecat

GQVIEW1_SRC = gqview_view_dir_tree.c gqview_view_dir_tree.h gqview2/gqview.h gqview2/typedefs.h gqview2/ui_fileops.c gqview2/ui_fileops.h gqview2/ui_tree_edit.c gqview2/ui_tree_edit.h gqview2/layout_util.c gqview2/layout_util.h gqview2/filelist.c gqview2/filelist.h gqview2/pixbuf_util.c gqview2/pixbuf_util.h \
	gqview2/ui_menu.c gqview2/ui_menu.h \
	gqview2/utilops.c gqview2/utilops.h
GQVIEW2_SRC = 

if ENABLE_AYYI
AYYI_SRC = ayyi.c ayyi.h ayyi_model.c ayyi_model.h
AYYI_LIBS = ../ayyi/libayyi.a
else
AYYI_SRC =
AYYI_LIBS =
endif

if ENABLE_MYSQL
MYSQL_SRC = db/mysql.c db/mysql.h
endif

if ENABLE_SQLITE
SQLITE_SRC = db/sqlite.c db/sqlite.h
else
SQLITE_SRC =
endif

if ENABLE_TRACKER
TRACKER_SRC = db/tracker.c db/tracker.h
endif

if ENABLE_FFTW3
FFT_SRC = spectrogram_widget.c spectrogram_widget.h
endif

if ENABLE_OPENGL
OPENGL_SRC = gl_spectrogram_view.c gl_spectrogram_view.h
endif

if ENABLE_DBUS
DBUS_SRC = auditioner.c auditioner.h
endif

if ENABLE_GAUDITION
DBUS_SRC = gauditioner.c auditioner.h
endif

samplecat_SOURCES = \
	typedefs.h types.h \
	main.c main.h \
	window.c window.h \
	listmodel.c listmodel.h \
	listview.c listview.h \
	inspector.c inspector.h \
	sample.c sample.h \
	audio.c audio.h \
	overview.c overview.h \
	pixmaps.c pixmaps.h \
	dh_link.c dh_link.h \
	dh_tree.c dh_tree.h \
	cellrenderer_hypertext.c cellrenderer_hypertext.h \
	colour_box.c colour_box.h \
	dnd.c dnd.h \
	observer.c observer.h \
	fscache.c fscache.h \
	support.c support.h \
	console_view.c console_view.h \
	spectrogram.c spectrogram.h \
	sndfile_common.c sndfile_common.h \
	sndfile_window.c sndfile_window.h \
	$(MYSQL_SRC) \
	$(TRACKER_SRC) \
	$(SQLITE_SRC) \
	$(FFT_SRC) \
	$(OPENGL_SRC) \
	$(AYYI_SRC) \
	$(DBUS_SRC) \
	$(GQVIEW1_SRC)

samplecat_LDADD = \
	file_manager/libfilemanager.a \
	gimp/libgimpwidgets.a \
	xdgmime/libxdgmime.a \
	$(AYYI_LIBS) \
	$(DBUS_LIBS) \
	$(TRACKER_LDFLAGS) \
	$(SQLITE_LIBS) \
	$(GTK_LIBS) $(MYSQL_LDFLAGS) $(JACK_LIBS) $(SNDFILE_LIBS) $(FLAC_LIBS) $(GTHREAD_LIBS) \
	$(FFTW3_LIBS) \
	$(OPENGL_LIBS)

EXTRA_DIST = db/sqlite.c db/sqlite.h db/tracker.c db/tracker.h \
	gl_spectrogram_view.c gl_spectrogram_view.h

