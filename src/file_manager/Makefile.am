
#SUBDIRS = filetypes

AM_CFLAGS = \
	-g -Wall \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I../ \
	-I../gqview2 \
	-I../xdgmime \
	-DG_LOG_DOMAIN=\"FileMan\" \
	-I$(top_srcdir)/src \
	$(GTK_CFLAGS)

noinst_LIBRARIES = libfilemanager.a

BUILT_SOURCES =

CLEANFILES =

libfilemanager_a_SOURCES = \
	$(libgdl_headers) \
	file_manager.c file_manager.h \
	vala/filemanager.c vala/filemanager.h \
	rox_global.h typedefs.h \
	file_view.c file_view.h \
	filer.c filer.h \
	mimetype.c mimetype.h \
	support.c support.h \
	display.c display.h \
	rox/view_iface.c rox/view_iface.h rox/rox_support.c rox/rox_support.h cell_icon.c cell_icon.h \
	menu.c menu.h \
	diritem.c diritem.h \
	dir.c dir.h

EXTRA_DIST = \
	file_view.h

