if ENABLE_YAML

AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Yaml\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	$(GLIB_CFLAGS)

noinst_LIBRARIES = libyaml.a

libyaml_a_SOURCES = \
	save.c save.h \
	load.c load.h

if BUILD_TESTS

bin_PROGRAMS = test

test_DEPENDENCIES = libyaml.a

test_SOURCES = \
	test.c

test_LDADD = \
	$(top_srcdir)/lib/test/libtest.la \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/yaml/libyaml.a \
	$(GTK_LDFLAGS) \
	$(YAML_LIBS)

suppression_dir = $(top_srcdir)/test/suppressions
valgrind:
	/usr/bin/valgrind \
		--leak-check=full \
		--gen-suppressions=yes \
		--log-fd=2 \
		--suppressions=$(suppression_dir)/suppressions_ \
		--suppressions=$(suppression_dir)/mesa.supp \
		--suppressions=$(suppression_dir)/glib.supp \
		--suppressions=$(suppression_dir)/pango.supp \
		--suppressions=$(suppression_dir)/fm.supp \
		--num-callers=50 \
		./test

endif
endif
