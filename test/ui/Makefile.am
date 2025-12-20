
if BUILD_TESTS

if ENABLE_ATSPI
ATSPI_PROGS = atspi
endif
bin_PROGRAMS = ui_test $(ATSPI_PROGS)

AM_CFLAGS = \
	-I../../src \
	-I../../lib \
	-I../../lib/waveform \
	-I../../lib/waveform/lib/agl \
	$(DBUS_CFLAGS) \
	$(GTK_CFLAGS) \
	$(ATSPI_CFLAGS)

ui_test_SOURCES = \
	test.c

if ENABLE_OPENGL
OPENGL_LDFLAGS = \
	$(GL_LDFLAGS)
endif

if ENABLE_YAML
YAML_LDFLAGS = \
	$(top_srcdir)/lib/yaml/libyaml.a \
	$(YAML_LIBS)
endif

ui_test_LDADD = \
	../../src/application.o \
	../../lib/gtk/cellrenderer_hypertext.o \
	../../src/panels/library.o \
	../../src/progress_dialog.o \
	../../src/resources.o \
	../../src/support.o \
	../../src/window.o \
	../../src/types.o \
	../../lib/gtk/icon_theme.o \
	$(top_srcdir)/lib/test/libtest.la \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/gdl/.libs/libgdl-4.a \
	$(XML_LIBS) \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/gtk/libgtk.la \
	$(top_srcdir)/lib/menu/libmenu.la \
	$(top_srcdir)/src/panels/libpanels.la \
	$(top_srcdir)/lib/samplecat/libsamplecat.la \
	$(top_srcdir)/lib/dir-tree/libdirtree.la \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/src/widgets/.libs/libwidgets.a \
	$(DBUS_LIBS) \
	$(DEBUG_LDFLAGS) \
	$(OPENGL_LDFLAGS) \
	$(GMODULE_LIBS) \
	$(top_srcdir)/lib/audio_analysis/libaudioanalysis.la \
	$(top_srcdir)/lib/waveform/.libs/libwaveformcore.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformui.a \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a \
	$(SQLITE_LIBS) \
	$(MYSQL_LDFLAGS) \
	$(JACK_LIBS) \
	$(SNDFILE_LIBS) \
	${SAMPLERATE_LIBS} \
	$(FFTW3_LIBS) \
	$(OPENGL_LDFLAGS) \
	$(GTK_LDFLAGS) \
	$(GRAPHENE_LIBS) \
	$(LIBASS_LDFLAGS) \
	$(X11_LDFLAGS) \
	$(ZLIB_LIBS) \
	$(GMODULE_LIBS) \
	$(FFMPEG_LIBS) \
	$(XRANDR_LDFLAGS) \
	$(MATH_LDFLAGS) \
	$(YAML_LDFLAGS) \
	-lstdc++ \
	-ldl

atspi_LDFLAGS = \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/test/.libs/libtest.a \
	$(ATSPI_LDFLAGS) \
	$(GTK_LDFLAGS)

BUILT_SOURCES = list.c atspi.h

list.c: Makefile test_*.c
	@echo > $@
	@for i in test_*.c; do echo "#include \"$$i\"" >> $@; done
	@echo >> $@
	@echo "gpointer tests[] = {" >> $@
	@for i in test_*.c; do echo "	$${i%.c}," >> $@; done
	@echo "};" >> $@

define build_header =
	echo > $@
	echo '#include "test/runner.h"' >> $@
	echo >> $@
	@echo "TestFn x" >> $@
	@cat $(@:.h=.c) | grep ^test_ | sed 's/\(.*\) .*/  ,\1/' >> $@
	@echo "  ;" >> $@
	@echo >> $@
	@echo "gpointer tests[] = {" >> $@
	@cat $(@:.h=.c) | grep ^test_ | sed 's/\(.*\) .*/  \1,/' >> $@
	@echo "};" >> $@
	@echo >> $@
	@echo "int n_tests () { return "`cat $(@:.h=.c) | grep ^test_ | wc -l`"; }" >> $@
endef

atspi.h: atspi.c Makefile
	@$(build_header)

TESTS = \
	ui_test

EXTRA_DIST = \
	utils.c \
	test_*.c

endif

