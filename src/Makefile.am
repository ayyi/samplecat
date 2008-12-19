## Process this file with automake to produce Makefile.in

SUBDIRS = file_manager gimp

AM_CFLAGS = \
	-g -Wall -O2 \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I./gqview2 \
	-I./file_manager \
	-I./gimp \
	$(GTK_CFLAGS) $(DBUS_CFLAGS) $(DB_CFLAGS) $(FLAC_CFLAGS) $(TRACKER_CFLAGS) $(XML_CFLAGS)

bin_PROGRAMS = samplecat

GQVIEW1_SRC = gqview_view_dir_tree.c gqview_view_dir_tree.h gqview2/gqview.h gqview2/typedefs.h gqview2/icons/icons_inline.h gqview2/ui_fileops.c gqview2/ui_fileops.h gqview2/ui_tree_edit.c gqview2/ui_tree_edit.h gqview2/layout_util.c gqview2/layout_util.h gqview2/filelist.c gqview2/filelist.h gqview2/pixbuf_util.c gqview2/pixbuf_util.h \
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

samplecat_SOURCES = \
	typedefs.h \
	main.c main.h file_manager/rox/rox_global.h \
	window.c window.h \
	listview.c listview.h \
	inspector.c inspector.h \
	sample.c sample.h \
	audio.c audio.h \
	overview.c overview.h \
	pixmaps.c pixmaps.h \
	diritem.c diritem.h \
	dh-link.c dh-link.h \
	tree.c tree.h \
	cellrenderer_hypertext.c cellrenderer_hypertext.h \
	dnd.c dnd.h \
	db.c db.h \
	observer.c observer.h \
	fscache.c fscache.h \
	xdgmime.c xdgmime.h xdgmimecache.c xdgmimecache.h xdgmimemagic.c xdgmimemagic.h xdgmimealias.c xdgmimealias.h xdgmimeparent.c xdgmimeparent.h xdgmimeglob.c xdgmimeglob.h xdgmimeint.c xdgmimeint.h \
	support.c support.h \
	tracker.c tracker_.h \
	$(AYYI_SRC) \
	$(GQVIEW1_SRC)

samplecat_LDADD = \
	file_manager/libfilemanager.a \
	gimp/libgimpwidgets.a \
	$(AYYI_LIBS) \
	$(DBUS_LDFLAGS) \
	$(TRACKER_LDFLAGS) \
	$(GTK_LIBS) $(DB_LIBS) $(JACK_LIBS) $(SNDFILE_LIBS) $(FLAC_LIBS) $(GTHREAD_LIBS)

