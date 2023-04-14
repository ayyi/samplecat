
AM_CFLAGS = \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-DG_LOG_DOMAIN=\"Utils\" \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS)

noinst_LIBRARIES = libutils.a

CLEANFILES =

libutils_a_SOURCES = \
	ayyi_utils.c ayyi_utils.h \
	fs.c fs.h

other_stuff = \
	fscache.c fscache.h \
	mime_type.c mime_type.h

