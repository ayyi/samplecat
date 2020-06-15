
SUBDIRS = filetypes

AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"FileMan\" \
	-DPACKAGE_LIB_DIR=\"$(pkglibdir)\" \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS)

noinst_LIBRARIES = libfilemanager.a

BUILT_SOURCES =

CLEANFILES =

GTK_SOURCES = \
	file_manager.c file_manager.h \
	filemanager.c filemanager.h \
	menu.c menu.h \
	minibuffer.c minibuffer.h \
	file_view.c file_view.h

libfilemanager_a_SOURCES = \
	typedefs.h \
	$(GTK_SOURCES) \
	pixmaps.c pixmaps.h \
	mimetype.c mimetype.h \
	support.c support.h \
	display.c display.h \
	view_iface.c view_iface.h \
	cell_icon.c cell_icon.h \
	diritem.c diritem.h \
	dir.c dir.h \
	fscache.c fscache.h

