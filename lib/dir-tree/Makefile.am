
AM_CFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-DG_LOG_DOMAIN=\"DirTree\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libdirtree.la

libdirtree_la_SOURCES = \
	view_dir_tree.c view_dir_tree.h \
	gqview.h typedefs.h \
	ui_fileops.c ui_fileops.h \
	ui_tree_edit.c ui_tree_edit.h \
	layout_util.c layout_util.h \
	filelist.c filelist.h \
	pixbuf_util.c pixbuf_util.h \
	ui_menu.c ui_menu.h \
	utilops.c utilops.h
