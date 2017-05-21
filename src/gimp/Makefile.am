AM_CFLAGS = \
	-g -Wall \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I../ \
	-DHAVE_GTK_2_10 \
	-DG_LOG_DOMAIN=\"Gimp\" \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	$(DBUS_CFLAGS) $(GTK_CFLAGS)

noinst_LIBRARIES = libgimpwidgets.a

BUILT_SOURCES =

CLEANFILES =

libgimpwidgets_a_SOURCES = \
	gimpaction.c gimpaction.h \
	gimpactiongroup.c gimpactiongroup.h

EXTRA_DIST =

