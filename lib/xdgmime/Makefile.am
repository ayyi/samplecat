AM_CPPFLAGS = \
	-I. \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-DPACKAGE_LIB_DIR=\"$(pkglibdir)\" \
	-DDEBUG \
	$(GTK_CFLAGS)

noinst_LIBRARIES = libxdgmime.a

CLEANFILES =

libxdgmime_a_headers =

libxdgmime_a_SOURCES = \
	xdgmime.c xdgmime.h \
	xdgmimealias.c xdgmimealias.h \
	xdgmimecache.c xdgmimecache.h \
	xdgmimeglob.c xdgmimeglob.h \
	xdgmimeint.c xdgmimeint.h \
	xdgmimemagic.c xdgmimemagic.h \
	xdgmimeparent.c xdgmimeparent.h

EXTRA_DIST =

