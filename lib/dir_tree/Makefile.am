
AM_CFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-DG_LOG_DOMAIN=\"DirTree\" \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS)

noinst_LIBRARIES = libdirtree.a

libdirtree_a_SOURCES = \
	view_dir_tree.c view_dir_tree.h \
	gqview.h typedefs.h \
	ui_fileops.c ui_fileops.h \
	ui_tree_edit.c ui_tree_edit.h \
	layout_util.c layout_util.h \
	filelist.c filelist.h \
	pixbuf_util.c pixbuf_util.h \
	ui_menu.c ui_menu.h \
	utilops.c utilops.h
