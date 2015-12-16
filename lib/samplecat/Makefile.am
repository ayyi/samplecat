SUBDIRS = db

AM_CFLAGS = \
	-g -Wall \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I../ \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS)

noinst_LIBRARIES = libsamplecat.a

CLEANFILES =

libsamplecat_a_SOURCES = \
	typedefs.h \
	samplecat.c samplecat.h \
	sample.c sample.h \
	worker.c worker.h \
	logger.c logger.h \
	settings.c settings.h \
	support.c support.h

