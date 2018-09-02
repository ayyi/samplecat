GDL_MAJOR_VERSION=2
GDL_MINOR_VERSION=30
GDL_MICRO_VERSION=1

GDL_CURRENT=3
GDL_REVISION=0
GDL_AGE=0

AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Gdl\" \
	-DGDL_UIDIR=\""$(uidir)"\" \
	-DGTK_DISABLE_DEPRECATION_WARNINGS \
	-DGLIB_DISABLE_DEPRECATION_WARNINGS \
	-I$(includedir) -I$(top_srcdir)/lib \
	$(GTK_CFLAGS) \
	$(XML_CFLAGS)

noinst_LIBRARIES = libgdl.a

BUILT_SOURCES = \
	libgdlmarshal.c \
	libgdlmarshal.h \
	libgdltypebuiltins.c \
	libgdltypebuiltins.h

CLEANFILES = \
	libgdltypebuiltins.c \
	libgdltypebuiltins.h \
	libgdlmarshal.c \
	libgdlmarshal.h

libgdl_headers = \
	gdl.h \
	gdl-tools.h \
	gdl-dock.h \
	gdl-dock-object.h \
	gdl-dock-master.h \
	gdl-dock-item.h \
	gdl-dock-item-grip.h \
	gdl-dock-layout.h \
	gdl-dock-bar.h \
	gdl-dock-placeholder.h \
	debug.h
	
libgdl_a_SOURCES = \
	$(libgdl_headers) \
	gdl-dock-object.c \
	gdl-dock-master.c \
	gdl-dock.c \
	gdl-dock-item.c \
	gdl-dock-item-button-image.h \
	gdl-dock-item-button-image.c \
	gdl-dock-item-grip.h \
	gdl-dock-item-grip.c \
	gdl-dock-notebook.h \
	gdl-dock-notebook.c \
	gdl-dock-paned.c \
	gdl-dock-paned.h \
	gdl-dock-tablabel.c \
	gdl-dock-tablabel.h \
	gdl-dock-layout.c \
	gdl-dock-placeholder.c \
	gdl-dock-bar.c \
	gdl-switcher.c \
	gdl-switcher.h \
	gdl-i18n.h \
	libgdltypebuiltins.h \
	libgdltypebuiltins.c \
	libgdlmarshal.h \
	libgdlmarshal.c

#libgdl_a_LIBADD = \
#	$(GDL_DEPENDENCIES_LIBS) \
#	$(XML_LIBS)

# libgdl_a_LDFLAGS = $(EXTRA_LDFLAGS) -version-info $(GDL_CURRENT):$(GDL_REVISION):$(GDL_AGE)

libgdlincludedir = $(includedir)/libgdl-1.0/gdl
libgdlinclude_HEADERS = \
	$(libgdl_headers) \
	libgdltypebuiltins.h

uidir = $(datadir)/gdl/ui
ui_DATA = layout.ui

EXTRA_DIST = libgdlmarshal.list \
	tree-expand.xpm \
	tree-contract.xpm \
	layout.ui \
	program.xpm static.xpm shared.xpm

# enumerations code generation

libgdltypebuiltins.c: $(libgdl_headers)
	glib-mkenums \
			--fhead "#include <glib-object.h>\n" \
			--fhead "#include \"libgdltypebuiltins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$^ > xgen-gtbec \
	&& cp xgen-gtbec $(@F) \
	&& rm -f xgen-gtbec

libgdltypebuiltins.h: $(libgdl_headers)
	glib-mkenums \
			--fhead "#ifndef __LIBGDLTYPEBUILTINS_H__\n" \
			--fhead "#define __LIBGDLTYPEBUILTINS_H__ 1\n\n" \
			--fhead "#include <gdl/gdl.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __LIBGDLTYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define GDL_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$^ > xgen-gth \
	&& cp xgen-gth $(@F) \
	&& rm -f xgen-gth

libgdlmarshal.h: libgdlmarshal.list
	glib-genmarshal \
		--prefix=gdl_marshal $(srcdir)/libgdlmarshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

libgdlmarshal.c: libgdlmarshal.h
	echo "#include \"libgdlmarshal.h\"" > xgen-gmc \
	&& glib-genmarshal \
		--prefix=gdl_marshal $(srcdir)/libgdlmarshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

