NULL =

AM_CFLAGS = -g -Wall $(MX_CFLAGS) $(MX_MAINTAINER_CFLAGS)
LDADD = \
	$(MX_LDFLAGS) $(MX_LIBS) \
	$(top_srcdir)/src/db/libsamplecatdb.a \
	$(MYSQL_LDFLAGS) \
	$(SQLITE_LIBS)

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	-I$(top_builddir) \
	$(GTK_CFLAGS)

noinst_PROGRAMS = \
	mx_test \
	$(NULL)

mx_test_SOURCES = main.c \
	list_model.c list_model.h \
	list_view.c list_view.h \
	list_item.c list_item.h \
	$(top_srcdir)/lib/utils/ayyi_utils.c $(top_srcdir)/lib/utils/ayyi_utils.h

GLIB_GENMARSHAL = glib-genmarshal

marshal.h: stamp-marshal.h
	@true
stamp-marshal.h: Makefile marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) \
		--prefix=_samplecat_marshal \
		--header \
	$(srcdir)/marshal.list > xgen-tmh && \
	(cmp -s xgen-tmh marshal.h || cp -f xgen-tmh marshal.h) && \
	rm -f xgen-tmh && \
	echo timestamp > $(@F)

marshal.c: Makefile marshal.list
	$(AM_V_GEN)(echo "#include \"marshal.h\"" ; \
	 $(GLIB_GENMARSHAL) \
		--prefix=_samplecat_marshal \
		--body \
	 $(srcdir)/marshal.list ) > xgen-tmc && \
	cp -f xgen-tmc marshal.c && \
	rm -f xgen-tmc

EXTRA_DIST =

#-include $(top_srcdir)/git.mk
