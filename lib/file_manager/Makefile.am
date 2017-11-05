
if ENABLE_VALA
VALA_DIR = vala
endif
SUBDIRS = filetypes $(VALA_DIR)

AM_CFLAGS = \
	-g -Wall \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_LIB_DIR=\"$(pkglibdir)\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I../ \
	-DG_LOG_DOMAIN=\"FileMan\" \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib/xdgmime \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS)

noinst_LIBRARIES = libfilemanager.a

BUILT_SOURCES =

CLEANFILES =

libfilemanager_a_SOURCES = \
	typedefs.h \
	file_manager.c file_manager.h \
	filemanager.c filemanager.h \
	file_view.c file_view.h \
	mimetype.c mimetype.h \
	support.c support.h \
	display.c display.h \
	view_iface.c view_iface.h \
	cell_icon.c cell_icon.h \
	menu.c menu.h \
	diritem.c diritem.h \
	dir.c dir.h \
	minibuffer.c minibuffer.h \
	pixmaps.h \
	fscache.h \
	dnd.h

