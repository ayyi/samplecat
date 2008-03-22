AM_CFLAGS = \
	-g -Wall \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I../ \
	-I../gqview2 \
	-DHAVE_GTK_2_10 \
	-DG_LOG_DOMAIN=\"FileMan\" \
	-I$(top_srcdir)/src \
	$(GTK_CFLAGS) $(DB_CFLAGS) $(FLAC_CFLAGS) $(XML_CFLAGS)

noinst_LIBRARIES = libfilemanager.a

BUILT_SOURCES =

CLEANFILES =

libfilemanager_headers = \
	file_manager.h
	file_view.h

libfilemanager_a_SOURCES = \
	$(libgdl_headers) \
	file_manager.c \
	file_view.c \
	filer.c filer.h rox/display.c rox/display.h rox/view_iface.c rox/view_iface.h rox/rox_support.c rox/rox_support.h rox/cell_icon.c rox/cell_icon.h \
	menu.c menu.h \
	mimetype.c mimetype.h \
	rox/dir.c rox/dir.h

EXTRA_DIST =

