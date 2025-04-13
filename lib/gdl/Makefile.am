GDL_MAJOR_VERSION=2
GDL_MINOR_VERSION=30
GDL_MICRO_VERSION=1

GDL_CURRENT=3
GDL_REVISION=0
GDL_AGE=0

if ENABLE_YAML
YAML_LDFLAGS = \
	$(top_srcdir)/lib/yaml/libyaml.a \
	$(YAML_LIBS)
endif

AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Gdl\" \
	-DGDL_UIDIR=\""$(uidir)"\" \
	-I$(includedir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	$(GTK_CFLAGS) \
	$(XML_CFLAGS)

noinst_LTLIBRARIES = libgdl-4.la

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

libgdl_4_headers = \
	gdl.h \
	gdl-dock.h \
	gdl-dock-object.h \
	master.h \
	gdl-dock-item.h \
	gdl-dock-item-grip.h \
	layout.h \
	gdl-dock-bar.h \
	gdl-dock-placeholder.h
	
libgdl_4_la_SOURCES = \
	$(libgdl_4_headers) \
	gdl-dock-object.c \
	master.c \
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
	layout.c \
	gdl-dock-placeholder.c \
	gdl-dock-bar.c \
	switcher.c \
	switcher.h \
	layoutmanager.c \
	layoutmanager.h \
	factory.c \
	factory.h \
	placeholder.c \
	placeholder.h \
	il8n.h \
	registry.h registry.c \
	utils.c utils.h \
	debug.c debug.h \
	libgdltypebuiltins.h \
	libgdltypebuiltins.c \
	libgdlmarshal.h \
	libgdlmarshal.c

if BUILD_TESTS
noinst_PROGRAMS=test-dock

test_dock_sources=test-dock.c
test_dock_LDADD=libgdl-4.la \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/gtk/libgtk.la \
	$(top_srcdir)/lib/utils/behaviour_subject.o \
	$(XML_LIBS) \
	$(YAML_LDFLAGS) \
	$(GTK_LDFLAGS)
endif

noinst_SCRIPTS = test-dock-seed.js test-dock-gjs.js

libgdlincludedir = $(includedir)/libgdl-4.0/gdl
libgdlinclude_HEADERS = \
	$(libgdl_4_headers) \
	libgdltypebuiltins.h

EXTRA_DIST = libgdlmarshal.list \
	gdl-gtk-paned.c \
	gdl-gtk-paned-handle.c \
	tree-expand.xpm \
	tree-contract.xpm \
	program.xpm static.xpm shared.xpm \
	$(noinst_SCRIPTS)

# enumerations code generation

libgdltypebuiltins.c: $(libgdl_4_headers)
	glib-mkenums \
			--fhead "#include <glib-object.h>\n" \
			--fhead "#undef GDL_DISABLE_DEPRECATED\n\n" \
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
	&& sed -i '1s;^;#include "config.h"\n;' $(@F) \
	&& rm -f xgen-gtbec

libgdltypebuiltins.h: $(libgdl_4_headers)
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
	&& sed -i '1s;^;#include "config.h"\n;' $(@F) \
	&& rm -f xgen-gmc

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_sources = $(libgdl_4_la_SOURCES)

Gdl-4.gir: libgdl-4.la
Gdl_4_gir_INCLUDES = GObject-2.0 Gtk-4.0
Gdl_4_gir_CFLAGS = $(INCLUDES) \
		   -UGDL_DISABLE_DEPRECATED
Gdl_4_gir_LIBS = libgdl-4.la
Gdl_4_gir_FILES = $(introspection_sources)
Gdl_4_gir_EXPORT_PACKAGES = gdl-3.0
Gdl_4_gir_SCANNERFLAGS = --c-include gdl/gdl.h
INTROSPECTION_GIRS += Gdl-3.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
