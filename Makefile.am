## Process this file with automake to produce Makefile.in

SUBDIRS = layouts lib src experimental test/unit test/ui

pkgconfigdir = $(libdir)/pkgconfig

# man page is generated by
# help2man -N -l -n "cataloguing and auditioning audio samples" -o samplecat.1 ./src/samplecat
man_MANS = samplecat.1

GLIB_COMPILE_RESOURCES = $(shell $(PKG_CONFIG) --variable=glib_compile_resources gio-2.0)

BUILT_SOURCES = resources.c

green = "\033[1;32m"
normal = "\033[0;39m"

resources.c: resources.xml resources/gtkrc
	@echo -en $(green)
	@echo compiling resources ...
	@echo -en $(normal)
	$(GLIB_COMPILE_RESOURCES) $(top_srcdir)/resources.xml --target=src/$@ --sourcedir=$(top_srcdir) --generate-source
	sed -i '1s;^;#include "config.h"\n;' src/$@

EXTRA_DIST = \
	resources.xml \
	resources/gtkrc

.PHONY: dummy
build_test: dummy
	./build_test

EXTRA_DIST = samplecat.1 \
	icons/samplecat.xpm \
	samples.sql \
	resources.xml \
	resources/gtkrc
