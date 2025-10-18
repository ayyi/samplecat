SUBDIRS = db player

AM_CFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libsamplecat.la

libsamplecat_la_SOURCES = \
	typedefs.h \
	samplecat.c samplecat.h \
	model.c model.h \
	list_store.c list_store.h \
	sample.c sample.h \
	dir_list.c dir_list.h \
	dh_link.c dh_link.h \
	worker.c worker.h \
	work.c work.h \
	logger.c logger.h \
	settings.c settings.h \
	observable.c observable.h \
	throttle.c throttle.h \
	support.c support.h

libsamplecat_la_LIBADD = \
	player/libplayer.la

