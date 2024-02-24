all:
	@./make_layouts
	@./make_ui

#install:

CLEANFILES = \
	layouts.c \
	layout_ui.c

#distclean:

#distdir:

layoutdir = $(sysconfdir)/samplecat/layout

layout_DATA = \
	Default.yaml \
	All.yaml \
	Minimal.yaml \
	Inspector.yaml \
	File_Manager.yaml

EXTRA_DIST = \
	$(layout_DATA) \
	make_layouts \
	make_ui \
	__default__.xml \
	__default__.yaml \
	layout.ui

