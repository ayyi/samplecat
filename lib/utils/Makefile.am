
AM_CFLAGS = \
	-g -Wall \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I../ \
	-DG_LOG_DOMAIN=\"Utils\" \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS)

noinst_LIBRARIES = libutils.a

CLEANFILES =

libutils_a_SOURCES = \
	ayyi_utils.c ayyi_utils.h \
	fs.c fs.h \
	fscache.c fscache.h \
	mime_type.c mime_type.h

